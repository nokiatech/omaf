
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "view.h"

#include "async/futureops.h"
#include "async/mediasteplock.h"
#include "audio.h"
#include "controllerops.h"
#include "controllerparsers.h"
#include "omaf/tileproxymultires.h"
#include "overlays.h"
#include "processor/debugsave.h"
#include "processor/debugsink.h"
#include "segmenter/moofcombine.h"
#include "segmenter/save.h"
#include "segmenter/sidxadjuster.h"
#include "segmenter/tagstreamidprocessor.h"
#include "segmenter/tagtrackidprocessor.h"
#include "timedmetadata.h"
#include "viewpoints.h"

namespace VDD
{
    namespace
    {
        template <typename Id>
        class ReuseIds
        {
        public:
            using IdProvider = std::function<Id()>;

            ReuseIds(IdProvider aIdProvider) : mBaseIdProvider(aIdProvider)
            {
                // nothing
            }

            IdProvider getProvider()
            {
                return [&] {
                    if (mReuseAt)
                    {
                        assert(*mReuseAt != mCache.end());
                        auto id = **mReuseAt;
                        ++*mReuseAt;
                        return id;
                    }
                    else
                    {
                        auto id = mBaseIdProvider();
                        mCache.push_back(id);
                        return id;
                    }
                };
            }

            void startReusing()
            {
                mReuseAt = mCache.cbegin();
            }

        private:
            IdProvider mBaseIdProvider;
            Id mId;
            std::list<Id> mCache;
            Optional<typename std::list<Id>::const_iterator> mReuseAt;
        };
    }  // namespace

    View::View(OmafVDController& aController, GraphBase& aGraph, const ControllerOps& aOps,
               const VDD::ConfigValue& aConfig, std::shared_ptr<Log> aLog,
               ViewId aViewpointId,
               ViewpointGroupId aViewpointGroupId)
        : mController(aController)
        , mGraph(aGraph)
        , mOps(aOps)
        , mId(aViewpointId)
        , mViewpointGroupId(aViewpointGroupId)
        , mConfig(aConfig)
        , mLog(aLog)
        , mVideoStepLock(Utils::make_unique<MediaStepLock>(aGraph))
        , mVDMode(VDMode::Invalid)
        , mVideoInputMode(VideoInputMode::Mono)
        , mBeginTime(clock())
        , mIndexAdaptationSetId([&] { return mController.newExtractorAdaptationSetIt(); })
    {
        mId = readOptional(readViewId)(mConfig["id"]).value_or({});
        mLabel = readOptional(readViewLabel)(mConfig["label"]);

        makeVideo();
        if (mInputVideos.size() && mLog->getLogLevel() == LogLevel::Info)
        {
            for (size_t videoIndex = 0u; videoIndex < mInputVideos.size(); ++videoIndex)
            {
                auto& video = mInputVideos[videoIndex];
                if (video.mTileProducerConfig.tileConfig.empty())
                {
                    continue;
                }
                // Connect the frame counter to be evaluated after each imported image
                if (video.mMediaSource != nullptr)
                {
                    std::string filename = video.config->filename;
                    connect(*video.mMediaSource,
                            "Input frame counter for " + filename,
                            [this, filename, videoIndex](const Streams& aStreams) {
                                if (!aStreams.isEndOfStream())
                                {
                                    std::unique_lock<std::mutex> framesLock(mFramesMutex);
                                    mLog->progress(static_cast<size_t>(mFrames[videoIndex]),
                                                   filename);
                                    ++mFrames[videoIndex];
                                    mEndTime = clock();
                                }
                            });
                }
            }
        }

        auto audioInput = mConfig["audio"];
        if (audioInput)
        {
            if (mController.getDash())
            {
                makeAudioDash(mOps, *getConfigure(), *mController.getDash(), audioInput, {} /*aPipelineBuildInfo*/);
            }
            if (mController.getMP4VROutputs().size())
            {
                makeAudioMp4(mOps, *getConfigure(), mController.getMP4VROutputs(), audioInput, {} /*aPipelineBuildInfo*/);
            }
        }

        if (mConfig["overlays"])
        {
            makeOverlays();
        }

        if (mConfig["viewpoint"])
        {
            makeViewpointMeta(mConfig["viewpoint"]);
        }

        if (mInputVideos.size() && (mController.getDash() || mController.getMP4VROutputs().size()))
        {
            auto syncFrameSource = getSyncFrameSource();
            if (auto initialViewingOrientationSample = readOptional(
                    readInitialViewingOrientationSample)(mConfig["initial_viewing_orientation"]))
            {
                AsyncProcessor* invoInput = nullptr;
                auto segmentDuration =
                    mController.getDash()
                        ? getConfigure()->getDashDurations().segmentDuration
                        : mController
                              .makeSegmentDurations(mGopInfo, getLongestVideoDuration(mInputVideos))
                              .segmentDuration;
                invoInput = makeInvoInput(mOps, segmentDuration, *initialViewingOrientationSample);
                connect(*syncFrameSource, *invoInput);
                if (mController.getDash())
                {
                    hookDashTimedMetadata(invoInput, *mController.getDash(),
                                          mController.newAdaptationSetId(), *getConfigure(), mOps,
                                          {} /*aPipelineBuildInfo*/);
                }
                for (auto& labeledMP4VRWriter : mController.getMP4VROutputs())
                {
                    auto& videoTrackIds = getVideoTrackIds();
                    hookMP4TimedMetadata(invoInput, *getConfigure(), mOps,
                                         {videoTrackIds.begin(), videoTrackIds.end()},
                                         *labeledMP4VRWriter.second, {} /*aPipelineBuildInfo*/,
                                         segmentDuration);
                }
            }
        }
    }

    View::~View() = default;

    ViewId View::getId() const
    {
        return mId;
    }

    Optional<ViewLabel> View::getLabel() const
    {
        return mLabel;
    }

    ViewpointGroupId View::getViewpointGroupId() const
    {
        return mViewpointGroupId;
    }

    const ConfigValue& View::getConfig() const
    {
        return mConfig;
    }

    /* Lower level configuration operations (ie. access to mInputLeft). */
    std::unique_ptr<ControllerConfigure> View::getConfigure()
    {
        return std::make_unique<ControllerConfigureForwarderForView>(mController, *this);
    }

    void View::report() const
    {
        if (mFrames.size())
        {
            double sec = (double)(mEndTime - mBeginTime) / CLOCKS_PER_SEC;

            mLog->log(Info) << mId << ": Wall clock time to process one frame is "
                            << sec / mFrames.begin()->second * 1000 << " miliseconds" << std::endl;
        }
    }

    const std::set<TrackId>& View::getVideoTrackIds() const
    {
        return mVideoTrackIds;
    }

    const std::set<TrackId> View::getMediaTrackIds() const
    {
        auto trackIds = mVideoTrackIds;
        std::copy(mAudioTrackIds.begin(), mAudioTrackIds.end(),
                  std::inserter(trackIds, trackIds.begin()));
        return trackIds;
    }

    void View::addVideoTrackId(TrackId aTrackId)
    {
        mVideoTrackIds.insert(aTrackId);
    }

    void View::addAudioTrackId(TrackId aTrackId)
    {
        mAudioTrackIds.insert(aTrackId);
    }

    const std::list<RepresentationId> View::getMediaRepresentationIds() const
    {
        std::list<RepresentationId> ids = mVideoRepresentationIds;
        std::copy(mAudioRepresentationIds.begin(),
                  mAudioRepresentationIds.end(),
                  std::back_inserter(ids));
        return ids;
    }

    void View::addVideoRepresentationId(RepresentationId aRepresentationId)
    {
        mVideoRepresentationIds.push_back(aRepresentationId);
    }

    void View::addAudioRepresentationId(RepresentationId aRepresentationId)
    {
        mAudioRepresentationIds.push_back(aRepresentationId);
    }

    void View::resetVideoRepresentationIds(const std::list<RepresentationId>& aRepresentationIds)
    {
        mVideoRepresentationIds = aRepresentationIds;
    }

    void View::addMainVideoAdaptationId(StreamId aAdaptationId)
    {
        mMainVideoAdaptationIds.push_back(aAdaptationId);
        mVideoAdaptationIds.push_back(aAdaptationId);
    }

    void View::addVideoAdaptationId(StreamId aAdaptationId)
    {
        mVideoAdaptationIds.push_back(aAdaptationId);
    }

    void View::addAudioAdaptationId(StreamId aAdaptationId)
    {
        mAudioAdaptationIds.push_back(aAdaptationId);
    }

    const std::list<StreamId>& View::getVideoAdaptationIds() const
    {
        return mVideoAdaptationIds;
    }

    const std::list<StreamId>& View::getMainVideoAdaptationIds() const
    {
        return mMainVideoAdaptationIds;
    }

    const std::list<StreamId> View::getMediaAdaptationIds() const
    {
        auto streamIds = mVideoAdaptationIds;
        std::copy(mAudioAdaptationIds.begin(), mAudioAdaptationIds.end(),
                  std::back_inserter(streamIds));
        return streamIds;
    }

    Projection View::getProjection() const
    {
        return mProjection;
    }

    void View::assign5KVideoRoles(const ConfigValue& aInputVideoConfigs)
    {
        bool hasRoles = true;
        for (auto& video : mInputVideos)
        {
            hasRoles = hasRoles && video.role;
        }

        if (hasRoles)
        {
            // early return: no automatic role assignment required
            return;
        }

        // currently MultiRes5K supports only two video streams, so let's ensure that here
        if (mInputVideos.size() != 2)
        {
            throw ConfigValueInvalid(
                "Multi res 5K auto configuration requires exactly two videos; assign explicit "
                "\"role\" to videos",
                aInputVideoConfigs);
        }

        InputVideo* highResVideo = nullptr;
        InputVideo* lowResVideo = nullptr;
        for (auto& video : mInputVideos)
        {
            if (!highResVideo || video.quality < highResVideo->quality)
            {
                highResVideo = &video;
            }
            if (!lowResVideo || video.quality > lowResVideo->quality)
            {
                lowResVideo = &video;
            }
        }

        if (highResVideo == lowResVideo)
        {
            throw ConfigValueInvalid("Video cannot be both high and low res", aInputVideoConfigs);
        }

        for (auto& video : mInputVideos)
        {
            if (!video.role)
            {
                if (&video == highResVideo)
                {
                    video.role = VideoRole::FullRes;
                }
                else if (&video == lowResVideo)
                {
                    video.role = VideoRole::LowRes;
                }
            }
        }
    }

    std::vector<std::reference_wrapper<InputVideo>> View::findVideosForRole(const VideoRole& aRole)
    {
        std::vector<std::reference_wrapper<InputVideo>> inputVideos;
        for (auto& input : mInputVideos)
        {
            if (*input.role == aRole)
            {
                inputVideos.push_back(input);
            }
        }
        return inputVideos;
    }

    InputVideo& View::findPrimaryVideoForRole(const VideoRole& aRole)
    {
        std::list<std::reference_wrapper<InputVideo>> inputVideos;
        for (auto& input : mInputVideos)
        {
            if (*input.role == aRole)
            {
                inputVideos.push_back(input);
            }
        }
        if (inputVideos.size() == 0)
        {
            throw ConfigValueReadError("Role " + stringOfVideoRole(aRole) +
                                       " was not defined, yet expected");
        }
        else if (inputVideos.size() > 1)
        {
            throw ConfigValueReadError("Role " + stringOfVideoRole(aRole) +
                                       " was available in multiple files, expected exactly 1");
        }
        else
        {
            return inputVideos.begin()->get();
        }
    }

    std::vector<std::reference_wrapper<InputVideo>> View::getInputVideosPrimaryFirst()
    {
        std::vector<std::reference_wrapper<InputVideo>> inputVideos;
        for (auto& input : mInputVideos)
        {
            if (input.abrIndex == 0u)
            {
                inputVideos.push_back(input);
            }
        }
        for (auto& input : mInputVideos)
        {
            if (input.abrIndex != 0u)
            {
                inputVideos.push_back(input);
            }
        }
        return inputVideos;
    }

    std::map<VideoRole, TileConfigurations::TiledInputVideo> View::pickTileConfigurations(
        const std::set<VideoRole>& aRequiredRoles,
        const std::map<VideoRole, OmafTileSets (*)(OmafTileSets& aTileConfig,
                                                   VideoInputMode aInputVideoMode)>& aCrop)
    {
        // basically computer-readable documentation:
        assert(mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K);

        std::map<VideoRole, TileConfigurations::TiledInputVideo> tileInputVideos;

        // handle ABR videos later for aesthetic and debugging reasons: ABR streams come last
        std::list<std::function<void()>> postponedABRVideos;
        for (auto role : aRequiredRoles)
        {
            for (auto video : findVideosForRole(role))
            {
                auto operation = [this, role, video, &aCrop, &tileInputVideos] {
                    if (aCrop.count(role))
                    {
                        OmafTileSets removedTiles = aCrop.at(role)(
                            video.get().mTileProducerConfig.tileConfig, mVideoInputMode);

                        // if there is cropping then we may need to rewrite tile ids
                        rewriteIdsForTiles(video.get(), removedTiles);
                    }
                    if (video.get().abrIndex == 0u)
                    {
                        tileInputVideos[role] =
                            OmafVDController::tiledVideoInputOfInputVideo(video.get());
                    }
                };
                if (video.get().abrIndex == 0u)
                {
                    operation();
                }
                else
                {
                    postponedABRVideos.push_back(std::move(operation));
                }
            }
        }

#if defined(_DEBUG)
        std::set<TrackId> usedTracks;
        for (auto role : aRequiredRoles)
        {
            for (auto& video : findVideosForRole(role))
            {
                for (const auto& tile : video.get().mTileProducerConfig.tileConfig)
                {
                    assert(!usedTracks.count(tile.trackId));
                    usedTracks.insert(tile.trackId);
                }
            }
        }
#endif

        for (auto& postponedOperation : postponedABRVideos)
        {
            postponedOperation();
        }

        return tileInputVideos;
    }

    namespace
    {
        struct RoleTrackGroupIdProvider
        {
            OmafVDController& mController;
            TileIndexTrackGroupIdMap& mTileIndexTrackGroupMap;

            RoleTrackGroupIdProvider(OmafVDController& aController, TileIndexTrackGroupIdMap& aMap)
                : mController(aController), mTileIndexTrackGroupMap(aMap)
            {
                // nothing
            }

            TrackGroupIdProvider operator()()
            {
                return [&](TileIndex aTileIndex) {
                    if (!mTileIndexTrackGroupMap.count(aTileIndex))
                    {
                        mTileIndexTrackGroupMap[aTileIndex] = mController.newTrackGroupId();
                    }
                    return mTileIndexTrackGroupMap.at(aTileIndex);
                };
            }
        };
    }  // namespace

    void View::makeVideo()
    {
        // create a placeholder config for tile proxy where the tile configs are read
        TileProxy::Config tileProxyConfig;

        // go through the inputs in config
        auto inputVideoConfigs = mConfig["video"];
        TileXY tilesXY;
        Optional<size_t> frameCount;

        // so the first is used for viewopints, the second for "non-viewpoints"..
        // a bit hacky but looks nice; but the ref_id can also be in the top level
        // in non-viewpoint case.
        mVideoRefIdLabel = readOptional(readRefIdLabel)(mConfig["ref_id"]);
        if (!mVideoRefIdLabel)
        {
            mVideoRefIdLabel = readOptional(readRefIdLabel)(inputVideoConfigs["ref_id"]);
        }

        // as the json configuration lines get ordered alphabetically, we need to make sure we first
        // read common, and only then read the individual video parameters
        for (const VDD::ConfigValue& value : inputVideoConfigs.childValues())
        {
            if (value.getName() == "common")
            {
                // common parameters
                readCommonInputVideoParams(value, frameCount, mOverrideFrameDuration);
            }
        }
        // the common-section is mandatory
        if (mVDMode == VDMode::Invalid)
        {
            throw ConfigValueInvalid(
                "Video input configuration error, not enough common parameters given",
                inputVideoConfigs);
        }

        // ReuseIds is used for reusing track ids for multiresoluotion cases
        ReuseIds<TrackId> reuseTrackId([&] { return mController.newTrackId(); });
        PipelineOutput output = PipelineOutputVideo::Mono;


        std::map<Optional<VideoRole>, TileIndexTrackGroupIdMap> tileIndexTrackGroupMap;
        ReuseIds<StreamId> reuseStreamId([&] { return mController.newAdaptationSetId(); });

        for (const VDD::ConfigValue& value : inputVideoConfigs.childValues())
        {
            if (value.getName() != "common" && value.getName() != "ref_id")
            {
                std::string label;
                Optional<VideoRole> role;
                std::tie(label, role) = OmafVDController::readLabelRole(value);
                auto abr = value["abr"];

                if (abr && !role)
                {
                    throw ConfigValueInvalid(
                        "abr video input requires role to be filled in explicitly", value);
                }

                RoleTrackGroupIdProvider trackGroupIdProvider(
                    mController,
                    // In MultiQ-case we use the same track groups for the same tile indices
                    // regardless of the role
                    tileIndexTrackGroupMap[mVDMode == VDMode::MultiQ ? Optional<VideoRole>()
                                                                     : role]);

                if (abr)
                {
                    size_t abrIndex = 0u;
                    size_t abrIndex0VideoIndex = mInputVideos.size();
                    size_t bestQualityVideoIndex = 0u;
                    std::vector<size_t> videoIndices;

                    // for mild efficiency boost
                    size_t numVideos = abr.childValues().size();
                    mInputVideos.reserve(mInputVideos.size() + numVideos);
                    videoIndices.reserve(numVideos);

                    TileXY firstTilesXY;

                    for (auto& videoConfig : abr.childValues())
                    {
                        if (videoConfig["ref_id"]) {
                            throw ConfigValueInvalid(
                                "Individual video inputs may not have ref_id, it must be in the "
                                "\"video\" level",
                                videoConfig["ref_id"]);
                        }

                        videoIndices.push_back(mInputVideos.size());
                        auto config_ = readInputVideoConfig(videoConfig, frameCount, tilesXY, label,
                                                            role, reuseTrackId.getProvider(),
                                                            trackGroupIdProvider(),
                                                            reuseStreamId.getProvider());
                        config_->abrIndex = abrIndex;

                        if (tilesXY.first * tilesXY.second < 1)
                        {
                            throw UnsupportedVideoInput(config_->mInputConfig->filename +
                                                        " is not a tiled video stream (required).");
                        }

                        if (abrIndex > 0u)
                        {
                            if (tilesXY != firstTilesXY)
                            {
                                throw UnsupportedVideoInput(
                                    config_->mInputConfig->filename +
                                    " does not share the same tile configuration with others.");
                            }
                            OmafVDController::validateABRVideoConfig(
                                mInputVideos[abrIndex0VideoIndex], *config_, videoConfig);
                        }
                        else
                        {
                            firstTilesXY = tilesXY;
                        }

                        if (abrIndex == 0u ||
                            config_->quality < mInputVideos[bestQualityVideoIndex].quality)
                        {
                            bestQualityVideoIndex =
                                static_cast<size_t>(config_ - mInputVideos.begin());
                        }
                        ++abrIndex;
                    }
                    // arrange abrIndex = 0 to the stream with best quality
                    std::swap(mInputVideos[bestQualityVideoIndex].abrIndex,
                              mInputVideos[abrIndex0VideoIndex].abrIndex);

                    // rewrite primaryInputVideoIndex
                    for (auto index : videoIndices)
                    {
                        mInputVideos[index].primaryInputVideoIndex = bestQualityVideoIndex;
                        assert(mInputVideos[index].abrIndex == 0u ||
                               index != mInputVideos[index].primaryInputVideoIndex);
                    }
                }
                else
                {
                    auto config_ = readInputVideoConfig(
                        value, frameCount, tilesXY, label, role, reuseTrackId.getProvider(),
                        trackGroupIdProvider(), reuseStreamId.getProvider());

                    if (tilesXY.first * tilesXY.second < 1)
                    {
                        throw UnsupportedVideoInput(config_->mInputConfig->filename +
                                                    " is not a tiled video stream (required).");
                    }
                }
                if (mVDMode == VDMode::MultiQ)
                {
                    //reuseTrackId.startReusing();
                }
            }
        }

        PipelineMode pipelineMode = PipelineModeVideo::Mono;
        if (mInputVideoMode == VideoInputMode::TopBottom)
        {
            output = PipelineOutputVideo::TopBottom;
            pipelineMode = PipelineModeVideo::TopBottom;
        }
        else if (mInputVideoMode == VideoInputMode::LeftRight)
        {
            output = PipelineOutputVideo::SideBySide;
            pipelineMode = PipelineModeVideo::SideBySide;
        }
        else if (mInputVideoMode == VideoInputMode::TemporalInterleaving)
        {
            output = PipelineOutputVideo::TemporalInterleaving;
            pipelineMode = PipelineModeVideo::Mono;
        }
        tileProxyConfig.outputMode = output;

        // assign roles to high and low res videos for MultiRes5K
        if (mVDMode == VDMode::MultiRes5K)
        {
            assign5KVideoRoles(inputVideoConfigs);
        }

        if (mVDMode == VDMode::MultiQ)
        {
            tileProxyConfig.tileCount = static_cast<size_t>(tilesXY.first * tilesXY.second);

            StreamAndTrack extractorId;
            extractorId.first = mController.newExtractorAdaptationSetIt();
            extractorId.second = mController.newTrackId();
            tileProxyConfig.extractors.push_back(extractorId);
        }
        else if (mVDMode == VDMode::MultiRes5K)
        {
            std::set<VideoRole> roles{VideoRole::FullRes, VideoRole::LowRes};
            std::map<VideoRole, TileConfigurations::TiledInputVideo> tileInputVideos =
                pickTileConfigurations(roles, {});

            if (tileInputVideos.size() != 2)
            {
                throw ConfigValueInvalid("Input video count not matching the output_mode",
                                         mConfig["video"]);
            }

            tileProxyConfig.tileCount = TileConfigurations::createERP5KConfig(
                tileProxyConfig.tileMergingConfig, tileInputVideos.at(VideoRole::FullRes),
                tileInputVideos.at(VideoRole::LowRes), tileProxyConfig.extractors);
        }
        else if (mVDMode == VDMode::MultiRes6K)
        {
            std::set<VideoRole> roles{VideoRole::FullRes, VideoRole::MediumRes,
                                      VideoRole::MediumResPolar, VideoRole::LowResPolar};

            std::map<VideoRole,
                     OmafTileSets (*)(OmafTileSets & aTileConfig, VideoInputMode aInputVideoMode)>
                crop{{VideoRole::FullRes, TileConfigurations::crop6KMainVideo},
                     {VideoRole::MediumRes, TileConfigurations::crop6KMainVideo},
                     {VideoRole::MediumResPolar, TileConfigurations::crop6KPolarVideo},
                     {VideoRole::LowResPolar, TileConfigurations::crop6KPolarVideo}};

            std::map<VideoRole, TileConfigurations::TiledInputVideo> tileInputVideos =
                pickTileConfigurations(roles, crop);

            if (tileInputVideos.size() != 4)
            {
                throw ConfigValueInvalid("Input video count not matching the output_mode",
                                         mConfig["video"]);
            }

            tileProxyConfig.tileCount = TileConfigurations::createERP6KConfig(
                tileProxyConfig.tileMergingConfig, tileInputVideos.at(VideoRole::FullRes),
                tileInputVideos.at(VideoRole::MediumRes),
                tileInputVideos.at(VideoRole::MediumResPolar),
                tileInputVideos.at(VideoRole::LowResPolar), tileProxyConfig.extractors,
                [&] { return mController.newTrackId(); },
                [&] { return mController.newExtractorAdaptationSetIt(); });
        }

        // We choose the most recently inserted extractor for this MP4 (we may not use it, ie. if we don't make MP4VR)
        StreamAndTrack extractorStreamAndTrackForMP4VR = tileProxyConfig.extractors.back();

        for (InputVideo& video : getInputVideosPrimaryFirst())
        {
            video.mTileProducerConfig.resetExtractorLevelIDCTo51 =
                mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K;
        }

        copyTrackIdsForABR();

        std::function<std::unique_ptr<TileProxy>(void)> tileProxyFactory;

        // create a single tile proxy
        if (mVDMode == VDMode::MultiQ)
        {
            tileProxyFactory = [tileProxyConfig]() {
                return Utils::make_unique<TileProxy>(tileProxyConfig);
            };
        }
        else
        {
            tileProxyFactory = [tileProxyConfig]() {
                // we use a more advanced proxy, that can also modify the extractors
                return Utils::make_unique<TileProxyMultiRes>(tileProxyConfig);
            };
        }

        for (size_t videoIndex = 0u; videoIndex < mInputVideos.size(); ++videoIndex)
        {
            mFrames[videoIndex] = 1;
        }

        ConfigureOutputForVideo cfg{};
        cfg.vdMode = mVDMode;
        cfg.videoTimeScale = mVideoTimeScale;
        cfg.mGopInfo = mGopInfo;
        cfg.mInputVideos = mInputVideos;
        cfg.newMpdViewId = [this]() { return mMpdViewCount++; };
        if (mController.getDash())
        {
            mTileProxyDash.reset(new TileProxyConnector(mGraph, tileProxyFactory()));
        }
        cfg.mTileProxyDash = mTileProxyDash;

        PipelineBuildInfo pipelineBuildInfo{};
        pipelineBuildInfo.output = output;
        pipelineBuildInfo.isExtractor = true;
        ConfigureOutputInfo info = mController.configureOutputForVideo(
            cfg, *this, output, pipelineMode, tileProxyFactory, tilesXY, tileProxyConfig,
            extractorStreamAndTrackForMP4VR, pipelineBuildInfo);
        if (mVideoRefIdLabel)
        {
            LabelEntityIdMapping mapping;
            EntityIdReference refId;
            refId.label = *mVideoRefIdLabel;
            if (info.dashIds.size())
            {
                for (auto dashInfo: info.dashIds)
                {
                    SingleEntityIdReference reference;
                    reference.adaptationSetId = dashInfo.adaptationSetId;
                    reference.representationId = makePromise(dashInfo.representationId);
                    reference.entityId = {dashInfo.trackId.get()};
                    refId.references.push_back(reference);
                }
            }
            else
            {
                SingleEntityIdReference reference;
                reference.trackId = extractorStreamAndTrackForMP4VR.second;
                reference.entityId = {reference.trackId.get()};
                refId.references.push_back(reference);
            }
            mapping.addLabel(refId);
            mController.addToLabelEntityIdMapping(mapping);
        }
    }

    void View::makeViewpointMeta(const ConfigValue& aConfigValue)
    {
        auto dyvpDecl = readViewpointDeclaration(aConfigValue);

        Optional<ViewpointDashInfo> dashInfo{};
        ViewpointMediaTrackInfo mediaTrackInfo{};

        if (mController.getDash())
        {
            ViewpointDashInfo info{};
            info.dash = mController.getDash();
            info.dashDurations = mController.getDashDurations();
            info.suffix = "dyvp";
            mediaTrackInfo.mediaAssociationIds = getMediaRepresentationIds();
            dashInfo = info;
        }

        mediaTrackInfo.gopInfo = mGopInfo;
        mediaTrackInfo.mediaTrackIds = getMediaTrackIds();
        mediaTrackInfo.mediaAdaptationSetIds = getMediaAdaptationIds();

        createViewpointMetadataTrack(*this, *getConfigure(), mOps, dyvpDecl,
                                     &mController.getMP4VROutputs(), dashInfo, mediaTrackInfo,
                                     {} /*aPipelineBuildInfo*/);
    }

    void View::makeOverlays()
    {
        auto ovlyDeclarations =
            readList("Overlay declarations",
                     readOverlayDeclaration(mController.getEntityGroupReadContext()))(mConfig["overlays"]);

        for (auto& ovlyDecl : ovlyDeclarations)
        {
            Optional<OverlayDashInfo> dashInfo{};
            if (mController.getDash())
            {
                OverlayDashInfo info{};
                info.dash = mController.getDash();
                info.dashDurations = mController.getDashDurations();
                info.backgroundAssocationIds = getMainVideoAdaptationIds();
                info.allocateAdaptationsetId = [&] { return mController.newAdaptationSetId(); };
                dashInfo = info;
            }

            auto overlayIdStr = std::to_string(ovlyDecl->ovly.overlays.at(0u).overlayId);

            OverlayVideoTrackInfo overlayInfo = createOverlayMediaTracks(
                *getConfigure(), mOps, *ovlyDecl, &mController.getMP4VROutputs(),
                dashInfo ? dashInfo->addSuffix("overlay." + overlayIdStr) : dashInfo,
                {} /*aPipelineBuildInfo*/);
            mController.addToLabelEntityIdMapping(overlayInfo.labelEntityIdMapping);

            // remember these so we create viewpoints in the mpd for these adaptations
            for (auto adaptationSetId : overlayInfo.overlayMediaAdaptationSetIds)
            {
                addVideoAdaptationId(adaptationSetId);
            }

            if (dashInfo)
            {
                dashInfo->metadataAssocationIds = overlayInfo.overlayMediaAssociationIds;
            }
            createTimedOverlayMetadataTracks(*getConfigure(), mOps, *ovlyDecl,
                                             &mController.getMP4VROutputs(), dashInfo, overlayInfo,
                                             {} /*aPipelineBuildInfo*/);
        }
    }

    void View::combineMoof(const PipelineInfo& aPipelineInfo,
                           const PipelineBuildInfo& aPipelineBuildInfo)
    {
        if (mController.getDash() &&
            mController.getDash()->getOutputMode() == OutputMode::OMAFV2 &&
            mController.getDash()->useImda(aPipelineBuildInfo.output))
        {
            assert(aPipelineBuildInfo.streamId);
            TagStreamIdProcessor::Config tagStreamIdConfig{*aPipelineBuildInfo.streamId};
            auto tagStream = mOps.makeForGraph<TagStreamIdProcessor>(
                "stream id:=" + Utils::to_string(*aPipelineBuildInfo.streamId), tagStreamIdConfig);

            connect(*aPipelineInfo.segmentSink, *tagStream, StreamFilter({Segmenter::cTrackRunStreamId})).ASYNC_HERE;
            connect(*tagStream, *getMoofOutput(aPipelineBuildInfo)).ASYNC_HERE;
        }
    }

    void View::setupMultiQSubpictureAdSets(const ConfigValue& aDashConfig,
                                           std::list<StreamId>& aStreamSetIds,
                                           std::list<StreamId>& aDashAdSetIds,
                                           SegmentDurations& aDashDurations, PipelineOutput aOutput,
                                           TileXY aTilesXY,
                                           StreamSegmenter::Segmenter::SequenceId& aSequenceBase,
                                           StreamSegmenter::Segmenter::SequenceId& aSequenceStep)
    {
        aSequenceStep = {aSequenceStep.get() +
                         uint32_t(mInputVideos.at(0).mTileProducerConfig.tileConfig.size() *
                                  mInputVideos.size())};
        for (size_t tileConfigIndex = 0;
             tileConfigIndex < mInputVideos.at(0).mTileProducerConfig.tileConfig.size();
             tileConfigIndex++)  // create adaptation sets - the tile configs are the same, only the
                                 // stream ids are different
        {
            // traditional loop is used since we need the tileConfigIndex in two places
            auto& tileConfig =
                mInputVideos.at(0).mTileProducerConfig.tileConfig.at(tileConfigIndex);

            // for representation variants
            std::map<PipelineOutput, AdaptationConfig> outputAdaptationConfigs;
            AdaptationConfig& adaptationConfig = outputAdaptationConfigs[aOutput];
            adaptationConfig.adaptationSetId = tileConfig.streamId;
            adaptationConfig.projectionFormat = mpdProjectionFormatOfOmafProjectionType(getProjection().projection);

            aDashAdSetIds.push_back(tileConfig.streamId);

            bool firstInput = true;
            for (auto& input : mInputVideos)  // create representations
            {
                const auto& curTileConfig = input.mTileProducerConfig.tileConfig.at(tileConfigIndex);
                using std::to_string;
                auto label = input.label + "." + "tile" + to_string(tileConfig.streamId);
                std::string baseName = mController.getDash()->getBaseName() + '.' + label;

                DashSegmenterMetaConfig dashMetaConfig{};
                dashMetaConfig.viewId = getId();
                dashMetaConfig.dashDurations = aDashDurations;
                dashMetaConfig.timeScale = mVideoTimeScale;
                dashMetaConfig.trackId = curTileConfig.trackId;
                dashMetaConfig.streamId = curTileConfig.streamId;
                dashMetaConfig.abrIndex = input.abrIndex;
                assert(aSequenceBase != aSequenceStep);
                FragmentSequenceGenerator fragmentSequenceGenerator{aSequenceBase, aSequenceStep};
                ++aSequenceBase;
                dashMetaConfig.fragmentSequenceGenerator = fragmentSequenceGenerator;

                auto dashSegmenterConfig =
                    makeOptional(mController.getDash()->makeDashSegmenterConfig(
                        aDashConfig, aOutput, baseName, dashMetaConfig));

                aStreamSetIds.push_back(curTileConfig.streamId);

                VDD::SegmenterInit::TrackConfig& trackConfig =
                    dashSegmenterConfig->segmenterInitConfig.tracks[dashMetaConfig.trackId];
                assert(tileConfig.trackGroupId == curTileConfig.trackGroupId);
                trackConfig.alte = tileConfig.trackGroupId;

                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                if (mController.getDash()->useImda(aOutput))
                {
                    dashSegmenterConfig.get().segmentInitSaverConfig = {};
                }
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                StreamFilter mask({curTileConfig.streamId});
                AsyncNode* source = getInputVideoOutputSource(input);
                PipelineBuildInfo pipelineBuildInfo{};
                pipelineBuildInfo.streamId = curTileConfig.streamId;
                pipelineBuildInfo.output = aOutput;
                pipelineBuildInfo.isExtractor = false;
                //pipelineBuildInfo.connectMoof = firstInput;
                mController.buildPipeline(
                    this, dashSegmenterConfig, aOutput, {{aOutput, {source, mask}}}, nullptr,
                    pipelineBuildInfo);

                if (input.abrIndex == 0u)
                {
                    RepresentationConfig representationConfig{};
                    representationConfig.output = *dashSegmenterConfig;
                    if (mController.getDash()->getOutputMode() == OutputMode::OMAFV2)
                    {
                        representationConfig.storeContentComponent = StoreContentComponent::TrackId;
                    }
                    adaptationConfig.representations.push_back(
                        Representation{input.mTileProducer, representationConfig});

                    // Add a video presentation here. If we are to use extractors, this list is
                    // cleared before setting them up.
                    mVideoRepresentationIds.push_back(dashSegmenterConfig->representationId);
                }
                firstInput = false;
            }

            // subpictures are always considered single eye, assuming the tiling splits them to
            // separate channels (Note: this is for MultiQ case where tiling in all videos must
            // match)
            PipelineOutput output = aOutput;
            if (aOutput == PipelineOutputVideo::TopBottom)
            {
                if (tileConfig.tileIndex.get() <
                    mInputVideos.front().mTileProducerConfig.tileCount / 2)
                {
                    // left
                    std::ostringstream sstream;
                    sstream << "l" << tileConfigIndex;
                    adaptationConfig.stereoId = sstream.str();
                    output = PipelineOutputVideo::Left;
                }
                else
                {
                    // right
                    std::ostringstream sstream;
                    sstream << "r"
                            << (tileConfigIndex -
                                mInputVideos.front().mTileProducerConfig.tileCount / 2);
                    adaptationConfig.stereoId = sstream.str();
                    output = PipelineOutputVideo::Right;
                }
            }
            else if (aOutput == PipelineOutputVideo::SideBySide)
            {
                // viewIndex = line + column(half)
                size_t viewIndex = (tileConfigIndex / aTilesXY.first) * (aTilesXY.first / 2) +
                                   (tileConfigIndex % (aTilesXY.first / 2));
                if ((tileConfig.tileIndex.get() % aTilesXY.first) < aTilesXY.first / 2)
                {
                    // left
                    std::ostringstream sstream;
                    sstream << "l" << viewIndex;
                    adaptationConfig.stereoId = sstream.str();
                    output = PipelineOutputVideo::Left;
                }
                else
                {
                    // right
                    std::ostringstream sstream;
                    sstream << "r" << viewIndex;
                    adaptationConfig.stereoId = sstream.str();
                    output = PipelineOutputVideo::Right;
                }
            }
            for (auto& outputAdaptation : outputAdaptationConfigs)
            {
                // don't shadow adaptationConfig
                auto& adaptationConfig_ = outputAdaptation.second;

                DashMPDConfig dashMPDConfig{};
                dashMPDConfig.segmenterOutput = output;
                dashMPDConfig.mpdViewIndex = mMpdViewCount;
                dashMPDConfig.adaptationConfig = adaptationConfig_;
                // baseAdaptationSetIds is filled in by the call to
                // addBasePreselectionToAdaptationSets when constructing extractor adaptations ets
                // in omafvdcontroller.cpp
                mController.getDash()->configureMPD(dashMPDConfig);
            }
            ++mMpdViewCount;
        }
    }

    AsyncNode* View::getInputVideoOutputSource(const InputVideo& aInputVideo)
    {
        AsyncNode* source = nullptr;
        if (aInputVideo.abrIndex == 0u)
        {
            source = mTileProxyDash->getSource();
        }
        else
        {
            source = aInputVideo.mTileProducer;
        }
        return source;
    }

    void View::setupMultiResSubpictureAdSets(const ConfigValue& aDashConfig,
                                             std::list<StreamId>& aStreamSetIds,
                                             std::list<StreamId>& aDashAdSetIds,
                                             SegmentDurations& aDashDurations,
                                             PipelineOutput aOutput,
                                             StreamSegmenter::Segmenter::SequenceId& aSequenceBase,
                                             StreamSegmenter::Segmenter::SequenceId& aSequenceStep)
    {
        // adaptationConfigs is indexed by (primary) inputIndex, tileConfigIndex.
        // The idea here is that because we handle primary video inputs first (due to using
        // getInputVideoOutputSource) and the abr frames the second, it always happens so that
        // adaptConfig[{primaryInputIndex, tileConfigIndex}] is already populated when handling ABR
        std::map<TileXY, AdaptationConfig> adaptationConfigs;
        auto inputs = getInputVideosPrimaryFirst();
        for (size_t inputIndex = 0u; inputIndex < inputs.size(); ++inputIndex)
        {
            auto& input = inputs[inputIndex].get();
            if (inputIndex == 0)
            {
                aSequenceStep = {
                    aSequenceStep.get() +
                    uint32_t(inputs.size() * input.mTileProducerConfig.tileConfig.size())};
            }
            bool primary = input.abrIndex == 0u;
            int count = 0;
            for (size_t tileConfigIndex = 0u;
                 tileConfigIndex < input.mTileProducerConfig.tileConfig.size(); ++tileConfigIndex)
            {
                const auto& tileConfig = input.mTileProducerConfig.tileConfig.at(tileConfigIndex);
                AdaptationConfig& adaptationConfig =
                    adaptationConfigs[{input.primaryInputVideoIndex, tileConfigIndex}];
                if (primary)
                {
                    adaptationConfig.adaptationSetId = tileConfig.streamId;
                }
                // extractor track needs to consider all qualities, so push all the tile streams to
                // both aStreamSetIds and aDashAdSetIds
                aStreamSetIds.push_back(adaptationConfig.adaptationSetId);
                aDashAdSetIds.push_back(adaptationConfig.adaptationSetId);
                adaptationConfig.projectionFormat = mpdProjectionFormatOfOmafProjectionType(getProjection().projection);

                auto label = input.label + "." + "tile" + std::to_string(count++);
                std::string baseName = mController.getDash()->getBaseName() + '.' + label;

                DashSegmenterMetaConfig dashMetaConfig{};
                dashMetaConfig.viewId = getId();
                dashMetaConfig.dashDurations = aDashDurations;
                dashMetaConfig.timeScale = mVideoTimeScale;
                dashMetaConfig.trackId = tileConfig.trackId;
                dashMetaConfig.streamId = tileConfig.streamId;
                dashMetaConfig.abrIndex = input.abrIndex;
                assert(aSequenceBase != aSequenceStep);
                FragmentSequenceGenerator fragmentSequenceGenerator{aSequenceBase, aSequenceStep};
                ++aSequenceBase;
                dashMetaConfig.fragmentSequenceGenerator = fragmentSequenceGenerator;

                Optional<DashSegmenterConfig> dashSegmenterConfig =
                    makeOptional(mController.getDash()->makeDashSegmenterConfig(
                        aDashConfig, aOutput, baseName, dashMetaConfig));
                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                if (mController.getDash()->getOutputMode() == OutputMode::OMAFV2)
                {
                    dashSegmenterConfig.get().segmentInitSaverConfig = {};
                }
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                StreamFilter mask({tileConfig.streamId});
                AsyncNode* source = getInputVideoOutputSource(input);
                PipelineBuildInfo pipelineBuildInfo{};
                pipelineBuildInfo.streamId = tileConfig.streamId;
                pipelineBuildInfo.output = aOutput;
                pipelineBuildInfo.isExtractor = false;
                //pipelineBuildInfo.connectMoof = primary;
                mController.buildPipeline(this, dashSegmenterConfig, aOutput,
                                          {{aOutput, {source, mask}}}, nullptr, pipelineBuildInfo);

                RepresentationConfig representationConfig{};
                representationConfig.output = *dashSegmenterConfig;
                if (mController.getDash()->getOutputMode() == OutputMode::OMAFV2)
                {
                    representationConfig.storeContentComponent = StoreContentComponent::TrackId;
                }

                adaptationConfig.representations.push_back(
                    Representation{input.mTileProducer, representationConfig});

                // Add a video presentation here. If we are to use extractors, this list is
                // cleared before setting them up.
                mVideoRepresentationIds.push_back(dashSegmenterConfig->representationId);
            }
        }

        for (auto indexAdaptationConfig : adaptationConfigs)
        {
            // size_t primaryVideoIndex = indexAdaptationConfig.first.first;
            // size_t tileConfigIndex = indexAdaptationConfig.first.second;
            AdaptationConfig& adaptationConfig = indexAdaptationConfig.second;

            DashMPDConfig dashMPDConfig{};
            dashMPDConfig.segmenterOutput = aOutput;
            dashMPDConfig.mpdViewIndex = mMpdViewCount;
            dashMPDConfig.adaptationConfig = adaptationConfig;
            // baseAdaptationSetIds is filled in by the call to addPartialAdaptationSetsToMultiResExtractor
            mController.getDash()->configureMPD(dashMPDConfig);
            ++mMpdViewCount;
        }
    }

    void View::readCommonInputVideoParams(const VDD::ConfigValue& aValue,
                                          Optional<size_t>& aFrameLimit,
                                          Optional<FrameDuration>& aOverrideFrameDuration)
    {
        std::string projection = readString(aValue["projection"]);
        if (projection.empty() || projection.find("equirect") != std::string::npos)
        {
            // OK, use the default
            mProjection.projection = OmafProjectionType::Equirectangular;
        }
        else if (projection.find("cubemap") != std::string::npos)
        {
            // in future could read the face order & rotation parameters
            mProjection.projection = OmafProjectionType::Cubemap;
        };
        if (aValue["output_mode"])
        {
            std::string mode = readString(aValue["output_mode"]);
            if (mode.find("5K") != std::string::npos)
            {
                mVDMode = VDMode::MultiRes5K;
            }
            else if (mode.find("6K") != std::string::npos)
            {
                mVDMode = VDMode::MultiRes6K;
            }
            else if (mode.find("MultiQ") != std::string::npos)
            {
                mVDMode = VDMode::MultiQ;
            }
        }
        if (aValue["input_mode"])
        {
            mVideoInputMode = readVideoInputMode(aValue["input_mode"]);
            if ((mVideoInputMode == VideoInputMode::LeftRight ||
                 mVideoInputMode == VideoInputMode::TopBottom) &&
                (mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K))
            {
                throw UnsupportedVideoInput(
                    "Stereo for 5K or 6K not yet supported. Stay tuned for updates");
            }
        }
        if (aValue["frame_count"])
        {
            aFrameLimit = readUint32(aValue["frame_count"]);
        }
        if (aValue["frame_rate"])
        {
            auto frameRate = readGeneric<FrameDuration>("frame_rate")(aValue["frame_rate"]);
            aOverrideFrameDuration = frameRate.per1();
        }
    }

    InputVideos::iterator View::readInputVideoConfig(
        const VDD::ConfigValue& aVideoInputConfigurationValue, Optional<size_t> aFrameLimit,
        TileXY& aTiles, std::string aLabel, Optional<VideoRole> aRole,
        TrackIdProvider aTrackIdProvider, TrackGroupIdProvider aTrackGroupIdProvider,
        StreamIdProvider aStreamIdProvider)
    {
        InputVideo input;
        input.label = aLabel;
        input.config.set(aVideoInputConfigurationValue, readMediaInputConfig);
        input.quality =
            std::max((uint8_t)1, (uint8_t)(readUint8(aVideoInputConfigurationValue["quality"])));
        input.role = aRole;

        // create a placeholder where the configs are read
        input.mInputConfig = input.config;
        input.mTileProducerConfig.quality = input.quality;
        // the rest is filled later, if needed

        VideoInput videoInput;
        VideoFileProperties prop =
            videoInput.getVideoFileProperties(input.config, mOverrideFrameDuration);
        if (!input.role)
        {
            input.role = detectVideoRole(prop, mVDMode);
            // input role may still be None; fixed hopefully by assign5KVideoRoles
        }

        if (mVideoTimeScale == FrameDuration(0, 1))
        {
            // left and right inputs are not used here, frame duration and timescale are relevant
            getConfigure()->setInput(nullptr, nullptr, mVideoInputMode, prop.fps.per1(),
                                     prop.timeScale, prop.gopDuration);
        }

        if (mVDMode != VDMode::MultiQ)
        {
            // Handled by aTrackGroupIdProvider in the makeVideo
        }
        aTiles.first = prop.tilesX;
        aTiles.second = prop.tilesY;

        // putting all tile rows here, may be filtered later on in some configurations
        for (size_t y = 0; y < prop.tilesY; y++)
        {
            for (size_t x = 0; x < prop.tilesX; x++)
            {
                OmafTileSetConfiguration tile;
                TileIndex tileIndex = y * prop.tilesX + x;
                tile.trackId = aTrackIdProvider();
                tile.trackGroupId = aTrackGroupIdProvider(tileIndex);
                tile.label = to_string(tile.trackId);
                tile.streamId = aStreamIdProvider();
                tile.tileIndex = tileIndex;

                input.mTileProducerConfig.tileConfig.push_back(tile);
            }
        }
        if (mVDMode != VDMode::MultiRes6K)
        {
            // Handled by aTrackGroupIdProvider in the makeVideo
        }
        // total tile count in the input, including skipped tile rows; used
        input.mTileProducerConfig.tileCount = static_cast<size_t>(prop.tilesX * prop.tilesY);
        input.width = prop.width;
        input.height = prop.height;
        input.ctuSize = prop.ctuSize;
        input.frameLimit = aFrameLimit;
        input.primaryInputVideoIndex = mInputVideos.size();

        mInputVideos.push_back(input);

        auto last = mInputVideos.end();
        --last;
        return last;
    }

    void View::rewriteIdsForTiles(InputVideo& aVideo, const OmafTileSets& aRemovedTiles)
    {
        assert(mVDMode == VDMode::MultiRes6K);

        std::list<TrackId> freeTrackIds;
        for (const auto& removedTile : aRemovedTiles)
        {
            freeTrackIds.push_back(removedTile.trackId);
        }

        for (auto& tile : aVideo.mTileProducerConfig.tileConfig)
        {
            assert(!freeTrackIds.empty());
            freeTrackIds.push_back(tile.trackId);

            // WHY assign new stream ids (aka adaptation set id..)? StreamIds are reused in the 6k
            // case, so we can't reuse those from tiles.. Instead we must renumber the streams.
            // This seems a bit complicated way to go about it; why not just use unique indentifiers
            // from the get-go? But it doesn't work.
            tile.streamId = mController.newAdaptationSetId();

            tile.trackId = freeTrackIds.front();
            tile.label = to_string(tile.trackId);
            freeTrackIds.pop_front();
        }

        assert(freeTrackIds.size() == aRemovedTiles.size());
    }

    void View::copyTrackIdsForABR()
    {
        size_t index = 0u;
        for (auto& input : mInputVideos)
        {
            if (input.abrIndex != 0u)
            {
                auto& src =
                    mInputVideos[input.primaryInputVideoIndex].mTileProducerConfig.tileConfig;
                auto& dst = input.mTileProducerConfig.tileConfig;
                for (size_t tileIndex = 0u; tileIndex < src.size(); ++tileIndex)
                {
                    dst.at(tileIndex).trackId = src.at(tileIndex).trackId;
                }
            }
            ++index;
        }
    }

    void View::associateTileProducer(InputVideo& aInput, ExtractorMode aExtractorMode)
    {
        // first fill the rest of common parameters
        aInput.mTileProducerConfig.extractorMode = aExtractorMode;
        aInput.mTileProducerConfig.projection = mProjection;
        aInput.mTileProducerConfig.videoMode = mVideoInputMode;

        // as the mp4 is expected to be a plain mp4, it doesn't contain any info about e.g.
        // framepacking. That should come via json, and we should be able to trust on it currently
        // only mono input (and output) is supported, framepacked is not supported

        MediaInputConfig mediaInputConfig = *aInput.mInputConfig;
        auto getSource = [&] {
            auto mediaLoader = mediaInputConfig.makeMediaLoader(
                true);  // bytestream format, as h265parser expects it
            auto videoTracks = mediaLoader->getTracksByFormat({CodedFormat::H265});
            if (!videoTracks.size())
            {
                throw UnsupportedVideoInput(aInput.mInputConfig->filename +
                                            " does not contain H.265 video.");
            }
            MediaLoader::SourceConfig sourceConfig{};
            if (aInput.frameLimit)
            {
                sourceConfig.frameLimit = size_t(*aInput.frameLimit);
            }
            sourceConfig.overrideFrameDuration = mOverrideFrameDuration;
            return mediaLoader->sourceForTrack(*videoTracks.begin(), sourceConfig);
        };

        {
            // this is consumed by the call getMediaAdaptationIds;
            auto durationSource = getSource();
            aInput.duration = getMediaDurationConsumingSource(*durationSource);
        }

        auto mediaSource = mOps.wrapForGraph(
            std::string("\"") + aInput.mInputConfig->filename + "\"", getSource());

        auto limited = mOps.configureForGraph(mVideoStepLock->get());
        connect(*mediaSource, *limited);
        aInput.mMediaSource = limited;

        // create a tile filter (which is also a source - to minimize code rewrite)
        auto tileProducer = Utils::make_unique<TileProducer>(aInput.mTileProducerConfig);

        aInput.mTileProducer = mOps.wrapForGraph("TileProducer", std::move(tileProducer));
        connect(*aInput.mMediaSource, *aInput.mTileProducer);
    }

    AsyncProcessor* View::getMoofOutput(const PipelineBuildInfo& aPipelineBuildInfo)
    {
        if (!mMoofOutput.count(aPipelineBuildInfo.output))
        {
            CombineNode::Config config{};
            config.labelPrefix = "MoofCombine";
            MoofOutput moofOutput{CombineNode(mGraph, config), 0u, {}, {}, aPipelineBuildInfo.segmentSink};
            mMoofOutput.insert(std::make_pair(aPipelineBuildInfo.output, std::move(moofOutput)));
        }
        MoofOutput& output = mMoofOutput.at(aPipelineBuildInfo.output);

        size_t index = output.count;
        ++output.count;

        if (aPipelineBuildInfo.isExtractor)
        {
            output.extractor.push_back(*aPipelineBuildInfo.streamId);
        }
        else
        {
            output.media.push_back(*aPipelineBuildInfo.streamId);
        }

        return mOps.configureForGraph(output.node.getSink(index));
    }

    AsyncSource* View::addExtractor(AsyncProcessor* aExtractorSegmenter)
    {
        Extractor extractor;
        extractor.extractor = aExtractorSegmenter;

        AsyncNode::pushDefaultColor(std::string("red"));
        std::shared_ptr<CombineSourceNode> moofExtractorCombineSource;

        {
            CombineNode::Config combineNodeConfig{};
            combineNodeConfig.labelPrefix = "MoofExtractorCombine";
            combineNodeConfig.renumberStreams = true;
            combineNodeConfig.requireSingleStream = true;
            extractor.outputCombiner = std::make_shared<CombineNode>(mGraph, combineNodeConfig);
        }

        extractor.extractorMoofSink = mOps.configureForGraph(extractor.outputCombiner->getSink(cExtractorMoofStreamId.get(), "ExtractorMoof"));
        extractor.tilesMoofSink = mOps.configureForGraph(extractor.outputCombiner->getSink(cTilesMoofStreamId.get(), "ExtractorMoof"));
        if (mController.getDash()->getProfile() == DashProfile::OnDemand)
        {
            extractor.combinedSidxSink = mOps.configureForGraph(extractor.outputCombiner->getSink(cCombinedSidxStreamId.get(), "CombinedSidx"));
            std::shared_ptr<CombineSourceNode> sidxCombineSource;
            {
                CombineNode::Config combineNodeConfig{};
                combineNodeConfig.labelPrefix = "SidxCombine";
                combineNodeConfig.renumberStreams = true;
                combineNodeConfig.requireSingleStream = true;
                extractor.forSidxBuilder = std::make_shared<CombineNode>(mGraph, combineNodeConfig);
            }
            extractor.forSidxBuilderSource = extractor.forSidxBuilder->getSource().get();
            extractor.extractorForSidxSink = mOps.configureForGraph(extractor.forSidxBuilder->getSink(
                SidxAdjuster::cExtractorSidxInputStreamId.get(), "ExtractorSidxForSidx"));
            extractor.tilesMoofForSidxSink = mOps.configureForGraph(extractor.forSidxBuilder->getSink(
                SidxAdjuster::cTilesMoofInputStreamId.get(), "TilesMoofForSidx"));
            {
                SidxAdjuster::Config config;
                auto adjuster = mOps.makeForGraph<SidxAdjuster>("SidxAdjuster", config);
                connect(*extractor.forSidxBuilderSource, *adjuster).ASYNC_HERE;
                connect(*adjuster, *extractor.combinedSidxSink).ASYNC_HERE;
            };
        }
        else
        {
            extractor.tilesMoofForSidxSink = nullptr;
            extractor.extractorForSidxSink = nullptr;
            extractor.combinedSidxSink = nullptr;
        }

        moofExtractorCombineSource = extractor.outputCombiner->getSource();

        extractor.combinedSource = moofExtractorCombineSource.get();

        if (extractor.extractorForSidxSink)
        {
            connect(*aExtractorSegmenter, *extractor.extractorForSidxSink, StreamFilter({Segmenter::cSidxStreamId})).ASYNC_HERE; // segment indices (if available separately)
        }
        connect(*aExtractorSegmenter, *extractor.extractorMoofSink, StreamFilter({Segmenter::cTrackRunStreamId})).ASYNC_HERE;

        // sourcePipelineInfo.segmentSink = moofSink;
        mExtractors.push_back(std::move(extractor));
        AsyncNode::popDefaultColor();

        return extractor.combinedSource;
    }

    void View::handleMoofCombine(PipelineOutput aOutput)
    {
        if (!mMoofOutput.count(aOutput))
        {
            return;
        }

        MoofOutput& moofs = mMoofOutput.at(aOutput);

        MoofCombine::Config moofCombineConfig{};
        moofCombineConfig.generateSidx = true;
        std::copy(moofs.media.begin(), moofs.media.end(),
                  std::back_inserter(moofCombineConfig.order));
        std::copy(moofs.extractor.begin(), moofs.extractor.end(),
                  std::back_inserter(moofCombineConfig.order));

        DashSegmenterMetaConfig dashMetaConfig{};
        dashMetaConfig.viewId = getId();
        dashMetaConfig.timeScale = mVideoTimeScale;
        dashMetaConfig.dashDurations = mController.getDashDurations();

        auto& dash = *mController.getDash();
        DashSegmenterConfig moofDashConfig = dash.makeDashSegmenterConfig(
            dash.dashConfigFor("media"), PipelineOutputIndexSegment::IndexSegment,
            dash.getBaseName(), dashMetaConfig);
        moofDashConfig.segmentInitSaverConfig = {}; // no init segments for index segments
        SegmenterAndSaverConfig& baseConfig = moofDashConfig.segmenterAndSaverConfig;

        if (mController.getDash()->getProfile() == DashProfile::OnDemand)
        {
            moofCombineConfig.writeSegmentHeader = MoofCombine::WriteSegmentHeader::Never;
        }
        else if (baseConfig.segmentSaverConfig)
        {
            moofCombineConfig.xtyp = Xtyp::Styp;
            moofCombineConfig.writeSegmentHeader = MoofCombine::WriteSegmentHeader::Always;
        }
        else if (moofDashConfig.singleFileSaverConfig)
        {
            moofCombineConfig.xtyp = Xtyp::Ftyp;
            moofCombineConfig.writeSegmentHeader = MoofCombine::WriteSegmentHeader::First;
        }
        auto combiner = mOps.makeForGraph<MoofCombine>("MoofCombine", moofCombineConfig);
        connect(*moofs.node.getSource(), *combiner).ASYNC_HERE;

        AsyncNode::ScopeColored scopeColored("blue");
        for (auto& extractor : mExtractors)
        {
            connect(*combiner, *extractor.tilesMoofSink).ASYNC_HERE;
            if (extractor.tilesMoofForSidxSink)
            {
                connect(*combiner, *extractor.tilesMoofForSidxSink).ASYNC_HERE;
            }
        }

        DashMPDConfig dashMPDConfig{};
        dashMPDConfig.segmenterOutput = aOutput;
        dashMPDConfig.mpdViewIndex = getId().get(); 
        AdaptationConfig adaptationConfig;
        adaptationConfig.adaptationSetId = *mIndexAdaptationSetId;
        adaptationConfig.representations.push_back(
            Representation{combiner, RepresentationConfig{{},
                                                          moofDashConfig,
                                                          /*assocationId*/ {},
                                                          /*associationType*/ {},
                                                          /*allMediaAssociationViewpoint*/ {},
                                                          /*storeContentComponent*/ {}}});
        dashMPDConfig.adaptationConfig = adaptationConfig;

        //dashMPDConfig.supportingAdSets = dashAdSetIds;
        mController.getDash()->configureMPD(dashMPDConfig);

        {
            AsyncNode::ScopeColored scopeColored2("purple");
            auto debugSink = mOps.makeForGraph<DebugSink>("tilesmoof debug", DebugSink::Config{});
            connect(*combiner, *debugSink).ASYNC_HERE;
        }

        if (auto& segmentInitSaverConfig = baseConfig.segmentSaverConfig)
        {
            auto initSave = mOps.makeForGraph<Save>(
                "\"" + segmentInitSaverConfig->fileTemplate + "\"", *segmentInitSaverConfig);
            connect(*combiner, *initSave).ASYNC_HERE;
        }
        else if (auto& singleFileSaverConfig = moofDashConfig.singleFileSaverConfig)
        {
            auto save = mOps.makeForGraph<SingleFileSave>(
                "\"" + singleFileSaverConfig->filename + "\"", *singleFileSaverConfig);
            connect(*combiner, *save).ASYNC_HERE;
        }
        else
        {
            assert(false);
        }
    }

    StreamId View::getIndexStreamId() const
    {
        return *mIndexAdaptationSetId;
    }

    AsyncNode* View::getSyncFrameSource()
    {
        // The first input video might not have media provided, as the MP4 case doesn't make use
        // of them all
        for (auto& inputVideo: mInputVideos) {
            if (inputVideo.mMediaSource)
            {
                return inputVideo.mMediaSource;
            }
        }
        assert(false);
        return nullptr;
    }
}  // namespace VDD