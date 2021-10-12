
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
#include "omafvdcontroller.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <iterator>
#include <memory>
#include <tuple>

#include "async/futureops.h"
#include "buildinfo.h"
#include "controllerops.h"
#include "processor/noop.h"
#include "processor/debugsink.h"
#include "view.h"
#include "timedmetadata.h"

namespace VDD
{
    FrameDuration getLongestVideoDuration(const InputVideos& aInputVideos)
    {
        FrameDuration duration;
        for (auto& video : aInputVideos)
        {
            if (video.duration)
            {
                duration = std::max(duration, *video.duration);
            }
        }
        return duration;
    }

    ControllerConfigureForwarderForView::ControllerConfigureForwarderForView(
        ControllerConfigureForwarderForView& aSelf)
        : mController(aSelf.mController), mView(aSelf.mView)
    {
    }

    ControllerConfigureForwarderForView* ControllerConfigureForwarderForView::clone()
    {
        return new ControllerConfigureForwarderForView(*this);
    }

    ControllerConfigureForwarderForView::ControllerConfigureForwarderForView(
        OmafVDController& aController, View& aView)
        : mController(aController), mView(&aView)
    {
    }

    ControllerConfigureForwarderForView::ControllerConfigureForwarderForView(
        OmafVDController& aController)
        : mController(aController), mView(nullptr)
    {
    }

    ControllerConfigureForwarderForView::~ControllerConfigureForwarderForView()
    {
    }

    bool ControllerConfigureForwarderForView::isView() const
    {
        return !!mView;
    }

    /** @brief Generate a new adaptation set id */
    StreamId ControllerConfigureForwarderForView::newAdaptationSetId()
    {
        return mController.newAdaptationSetId();
    }

    TrackId ControllerConfigureForwarderForView::newTrackId()
    {
        return mController.newTrackId();
    }

    /** @brief Set the input sources and streams */
    void ControllerConfigureForwarderForView::setInput(AsyncNode* aInputLeft,
                                                       AsyncNode* aInputRight,
                                                       VideoInputMode aInputVideoMode,
                                                       FrameDuration aFrameDuration,
                                                       FrameDuration aTimeScale, VideoGOP aGopInfo)
    {
        if (mView)
        {
            mView->mInputLeft = aInputLeft;
            mView->mInputRight = aInputRight;
            mView->mInputVideoMode = aInputVideoMode;
            mView->mVideoFrameDuration = aFrameDuration;
            mView->mVideoTimeScale = aTimeScale;
            mView->mGopInfo = aGopInfo;
        }
    }

    /** @brief Is the Controller in mpd-only-mode? */
    bool ControllerConfigureForwarderForView::isMpdOnly() const
    {
        return false;  // this is a legacy functionality, only related to DASH and a minor
                       // external use case of it, not relevant for the mp4controller,
                       // should we get rid of it?
    }

    /** @brief Retrieve frame duration */
    FrameDuration ControllerConfigureForwarderForView::getFrameDuration() const
    {
        assert(mView);
        return mView->mVideoFrameDuration;
    }

    /** @see OmafVDController::getDashDurations */
    SegmentDurations ControllerConfigureForwarderForView::getDashDurations() const
    {
        return mController.getDashDurations();
    }

    /** @brief Builds the audio dash pipeline */
    Optional<AudioDashInfo> ControllerConfigureForwarderForView::makeAudioDashPipeline(
        const ConfigValue& aDashConfig, std::string aAudioName,
        AsyncNode* aAacInput, FrameDuration aTimeScale,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        assert(mView);
        return mController.makeAudioDashPipeline(mView, aDashConfig, aAudioName, aAacInput,
                                                 aTimeScale, aPipelineBuildInfo);
    }

    /** @see OmafVDController::buildPipeline */
    PipelineInfo ControllerConfigureForwarderForView::buildPipeline(
        Optional<DashSegmenterConfig> aDashOutput, PipelineOutput aPipelineOutput,
        PipelineOutputNodeMap aSources, AsyncProcessor* aMP4VRSink,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        // we might have mViwe==null here; it's fine
        return mController.buildPipeline(mView, aDashOutput, aPipelineOutput, aSources,
                                         aMP4VRSink, aPipelineBuildInfo);
    }

    ViewId ControllerConfigureForwarderForView::getViewId() const
    {
        assert(mView);
        return mView->getId();
    }

    Optional<ViewLabel> ControllerConfigureForwarderForView::getViewLabel() const
    {
        assert(mView);
        return mView->getLabel();
    }

    std::list<AsyncProcessor*> ControllerConfigureForwarderForView::getMediaPlaceholders()
    {
        assert(mView);
        if (mController.mMediaPipelineEndPlaceholders.count(mView))
        {
            return mController.mMediaPipelineEndPlaceholders.at(mView);
        }
        else
        {
            return {};
        }
    }

    Optional<Future<Optional<StreamSegmenter::Segmenter::MetaSpec>>> ControllerConfigureForwarderForView::getFileMeta() const
    {
        return mController.mFileMeta;
    }

    void ControllerConfigureForwarderForView::postponeOperation(PostponeTo aToPhase, std::function<void(void)> aOperation)
    {
        mController.postponeOperation(aToPhase, aOperation);
    }

    std::list<EntityIdReference> ControllerConfigureForwarderForView::resolveRefIdLabels(std::list<ParsedValue<RefIdLabel>> aRefIdLabels)
    {
        return mController.resolveRefIdLabels(aRefIdLabels);
    }

    std::set<StreamId> InputVideo::getStreamIds() const
    {
        std::set<StreamId> streamIds;
        for (const OmafTileSetConfiguration& tile : mTileProducerConfig.tileConfig)
        {
            streamIds.insert(tile.streamId);
        }
        return streamIds;
    }

    MP4VROutputs& OmafVDController::getMP4VROutputs()
    {
        return mMP4VROutputs;
    }

    OmafVDController::OmafVDController(Config aConfig)
        : ControllerBase(aConfig)
        , mParseOnly(optionWithDefault(mConfig->root(), "debug.parse_only", readBool, false))
    {
        if ((*mConfig)["mp4"] && (*mConfig)["dash"])
        {
            throw ConfigValueInvalid("Cannot output both MP4 and DASH at the same time; pick one", (*mConfig)["mp4"]);
        }
        if ((*mConfig)["mp4"])
        {
            auto config = MP4VRWriter::loadConfig((*mConfig)["mp4"]);
            config.fragmented = false;
            config.log = mLog;
            mMP4VRConfig = config;
            mMP4VRConfig->fileMeta = mFileMeta;
        }

        // toplevel video-setting can be overridden by individal view settings
        ConfigValue videoConfig = (*mConfig)["video"];

        // Done before loading videos, as they have references to here
        // Part of processing is postponed (via postponeOperation)
        makeTimedMetadata();

        ViewId viewpointId{0u};

        if (mConfig->root()["views"])
        {
            std::set<ViewId> uniqueIds;

            for (auto viewConfig : mConfig->root()["views"].childValues())
            {
                struct ControllerOps::Config config
                {
                };
                auto id = readViewId(viewConfig["id"]);
                config.prefix = std::to_string(id.get());
                if (viewConfig["video"] && videoConfig)
                {
                    // hack! This is because we don't support marking the old
                    // branch visited even if we visit the new keys. instead of
                    // merging, perhaps we could avoid this whole issue by
                    // making use of ConfigValueWithFallback. But ConfigValue
                    // does not work well with slicing, so this is maybe the way
                    // to go for now.
                    videoConfig.markBranchVisited();
                    viewConfig["video"].merge(videoConfig, fillBlanksJsonMergeStrategy);
                }
                mViews.emplace_back(*this, *mGraph, ControllerOps(*mOps, config), viewConfig, mLog,
                                    viewpointId, ViewpointGroupId(0u));
                ++viewpointId;
            }
        }
        else
        {
            mViews.emplace_back(*this, *mGraph, *mOps, mConfig->root(), mLog, ViewId(0u),
                                ViewpointGroupId(0u));
        }

        makeEntityGroups();

        // Phase 3

        // support adding postponed operations inside the loop
        while (!mPostponedOperations.empty()) {
            auto operation = mPostponedOperations.front();
            mPostponedOperations.pop_front();
            operation();
        }

        if (getDash())
        {
            getDash()->setupMpd();
        }

        for (auto& labeledMP4VRWriter : mMP4VROutputs)
        {
            labeledMP4VRWriter.second->finalizePipeline();
        }

        setupDebug();

        writeDot();
    }

    OmafVDController::~OmafVDController()
    {
        // nothing
    }

    void OmafVDController::setupDebug()
    {
    }

    PipelineInfo OmafVDController::buildPipeline(View* aView,
                                                 Optional<DashSegmenterConfig> aDashOutput,
                                                 PipelineOutput aPipelineOutput,
                                                 PipelineOutputNodeMap aSources,
                                                 AsyncProcessor* aMP4VRSink,
                                                 Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        AsyncNode::ScopeColored scopeColored("green");

        PipelineInfo sourcePipelineInfo{};
        sourcePipelineInfo.source = aSources[aPipelineOutput].first;

        StreamFilter sinkStreamFilter = aSources[aPipelineOutput].second;
        AsyncProcessor* sinkInit = nullptr;
        AsyncProcessor* sinkSegment =
            nullptr;  // mOps->makeForGraph<NoOp>("Pipeline sinkSegment", NoOp::Config{});
        AsyncProcessor* sourceSegment = nullptr;
        AsyncProcessor* save = nullptr;

        if (aPipelineOutput.getClass() == PipelineClass::Video ||
            aPipelineOutput.getClass() == PipelineClass::Audio)
        {
            AsyncProcessor* placeholder =
                mOps->makeForGraph<NoOp>("Media config\nplaceholder", NoOp::Config{});
            mMediaPipelineEndPlaceholders[aView].push_back(placeholder);
            connect(*sourcePipelineInfo.source, *placeholder, sinkStreamFilter).ASYNC_HERE;
            sinkStreamFilter = allStreams;
            sourcePipelineInfo.source = placeholder;
        }

        bool omafv2Extractor =
            getDash() && getDash()->getOutputMode() == OutputMode::OMAFV2 && aPipelineOutput.isExtractor();

        if (aMP4VRSink)
        {
            sinkSegment = aMP4VRSink;
            // sourceSegment?!
        }
        else if (aDashOutput)
        {
            if (aDashOutput->segmentInitSaverConfig || aDashOutput->singleFileSaverConfig)
            {
                auto segmenterInitConfig = aDashOutput->segmenterInitConfig;
                sinkInit = mOps->makeForGraph<SegmenterInit>("segmenter init", segmenterInitConfig);

                if (auto& segmentInitSaverConfig = aDashOutput->segmentInitSaverConfig)
                {
                    auto initSave = aDashOutput
                                        ? mOps->makeForGraph<Save>(
                                              "\"" + segmentInitSaverConfig->fileTemplate + "\"",
                                              *segmentInitSaverConfig)
                                        : nullptr;
                    connect(*sinkInit, *initSave).ASYNC_HERE;
                }
                else if (auto& singleFileSaverConfig = aDashOutput->singleFileSaverConfig)
                {
                    save = mOps->makeForGraph<SingleFileSave>(
                        "\"" + singleFileSaverConfig->filename + "\"", *singleFileSaverConfig);
                    connect(*sinkInit, *save).ASYNC_HERE;
                }
                else
                {
                    // ok, so we don't save init segment for this stream
                }
            }

            // Ensure we end up generating either purely mdat-based output or imda-based output;
            // mixing will produce unexpected kind of output from Segmenter.
            assert(sinkInit || getDash()->useImda(aPipelineOutput));

            auto& sasConf = aDashOutput->segmenterAndSaverConfig;
            auto segmenter = mOps->makeForGraph<Segmenter>("segmenter", sasConf.segmenterConfig);
            sourcePipelineInfo.segmentSink = segmenter;

            {
                AsyncNode::ScopeColored scopeColored2("purple");
                DebugSink::Config config{};
                config.log = true;
                auto debugSink = mOps->makeForGraph<DebugSink>("pipeline debug", config);
                connect(*segmenter, *debugSink).ASYNC_HERE;
            }

            // source is later connected to sinkSegment
            sinkSegment = segmenter;
            sourceSegment = segmenter;

            // if (!omafv2Extractor)
            {
                // but actual segment saving part is handler here
                if (save)
                {
                    // if (!omafv2Extractor)
                    {
                        if (getDash()->getOutputMode() == OutputMode::OMAFV2)
                        {
                            auto streamFilter = getDash()->useImda(aPipelineOutput)
                                                    ? StreamFilter({Segmenter::cImdaStreamId,
                                                                    Segmenter::cSidxStreamId})
                                                    : StreamFilter({Segmenter::cSingleStreamId,
                                                                    Segmenter::cSidxStreamId});
                            connect(*segmenter, *save, streamFilter).ASYNC_HERE;
                        }
                        else {
                            connect(*segmenter, *save).ASYNC_HERE;
                        }
                    }
                }
                else if (auto& segmentSaverConfig = sasConf.segmentSaverConfig)
                {
                    save = mOps->makeForGraph<Save>("\"" + segmentSaverConfig->fileTemplate + "\"",
                                                    *segmentSaverConfig);
                    auto streamFilter = getDash()->useImda(aPipelineOutput)
                                            ? StreamFilter({Segmenter::cImdaStreamId})
                                            : StreamFilter({Segmenter::cSingleStreamId});
                    connect(*segmenter, *save, streamFilter).ASYNC_HERE;
                }
                else
                {
                    // Either save should be set or there should be segment saver config
                    assert(false);
                }

                if (save)
                {
                    // We need to connect segmenter to provide segment duration info to
                    // MPD writer
                    getDash()->hookSegmentSaverSignal(*aDashOutput, segmenter);

                    // Saver is nice to hook for video so the generated MPD never refers
                    // to files that haven't yet been written.

                    // getDash()->hookSegmentSaverSignal(*aDashOutput, save);
                }
            }
        }

        if (sinkInit)
        {
            connect(*sourcePipelineInfo.source, *sinkInit, sinkStreamFilter).ASYNC_HERE;
        }
        connect(*sourcePipelineInfo.source, *sinkSegment, sinkStreamFilter).ASYNC_HERE;

        // there was code here to handle omafv2Extractor specially, to call addExtractor and handle
        // stuff in a special way

        if (aView && aPipelineBuildInfo && aPipelineBuildInfo->connectMoof)
        {
            AsyncNode::ScopeColored scopeColored2("hotpink");
            aView->combineMoof(sourcePipelineInfo, *aPipelineBuildInfo);
        }

        return sourcePipelineInfo;
    }

    TileConfigurations::TiledInputVideo OmafVDController::tiledVideoInputOfInputVideo(
        const InputVideo& aInputVideo)
    {
        TileConfigurations::TiledInputVideo video{};
        video.height = aInputVideo.height;
        video.width = aInputVideo.width;
        video.qualityRank = aInputVideo.quality;
        video.tiles = aInputVideo.mTileProducerConfig.tileConfig;
        video.ctuSize = aInputVideo.ctuSize;
        return video;
    }

    void OmafVDController::validateABRVideoConfig(const InputVideo& aReferenceVideo,
                                                  const InputVideo& aCurrentVideo,
                                                  const ConfigValue& aVideoConfig)
    {
        if (aReferenceVideo.width != aCurrentVideo.width)
        {
            throw ConfigValueInvalid("Height for ABR video does not match the first video",
                                     aVideoConfig);
        }
        if (aReferenceVideo.height != aCurrentVideo.height)
        {
            throw ConfigValueInvalid("Width for ABR video does not match the first video",
                                     aVideoConfig);
        }
        if (aReferenceVideo.ctuSize != aCurrentVideo.ctuSize)
        {
            throw ConfigValueInvalid("Ctu size for ABR video does not match the first video",
                                     aVideoConfig);
        }
        if (aReferenceVideo.mTileProducerConfig.tileCount !=
            aCurrentVideo.mTileProducerConfig.tileCount)
        {
            throw ConfigValueInvalid("Tile count for ABR video does not match the first video",
                                     aVideoConfig);
        }
    }

    ConfigureOutputInfo OmafVDController::configureOutputForVideo(
        const ConfigureOutputForVideo& aConfig, View& aView, PipelineOutput aOutput,
        PipelineMode aPipelineMode,
        std::function<std::unique_ptr<TileProxy>(void)> aTileProxyFactory,
        std::pair<size_t, size_t> aTilesXY, TileProxy::Config aTileProxyConfig,
        Optional<StreamAndTrack> aExtractorStreamAndTrackForMP4VR,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        ConfigureOutputInfo configureOutputInfo{};
        ExtractorMode extractorMode = CREATE_EXTRACTOR;

        // create dash/mp4 sinks and connect the tile proxy to them
        if (mDash)
        {
            if (mDash->getConfig()["output_name_base"])
            {
                getDash()->setBaseName(readString(mDash->getConfig()["output_name_base"]));
            }
            else
            {
                // use the video output mode
                getDash()->setBaseName(
                    readString(aView.getConfig()["video"]["common"]["output_mode"]));
            }

            FrameDuration longestVideoDuration;
            for (InputVideo& video : aView.getInputVideosPrimaryFirst())
            {
                aView.associateTileProducer(video,
                                            video.abrIndex == 0u ? extractorMode : NO_EXTRACTOR);

                if (video.duration) {
                    longestVideoDuration = std::max(longestVideoDuration, *video.duration);
                }

                if (aConfig.vdMode == VDMode::MultiQ)
                {
                    // only 1 extractor needed for multi-Q/single-res case
                    extractorMode = NO_EXTRACTOR;
                }

                if (video.abrIndex == 0u)
                {
                    // connect the tile producer to the tile proxy
                    connect(*video.mTileProducer,
                            *aConfig.mTileProxyDash->getSink(video.getStreamIds()),
                            StreamFilter(video.getStreamIds())).ASYNC_HERE;
                }
                else
                {
                    // hooked later to the segmenter
                }
            }

            if (!aConfig.mGopInfo.fixed)
            {
                throw UnsupportedVideoInput("GOP length must be fixed in input when creating DASH");
            }

            mDashDurations = makeSegmentDurations(aConfig.mGopInfo, longestVideoDuration);
            auto dashConfig = getDash()->dashConfigFor("media");

            std::list<StreamId> dashAdSetIds, streamIds;

            StreamSegmenter::Segmenter::SequenceId sequenceStep = {
                std::uint32_t(aTileProxyConfig.extractors.size())};
            StreamSegmenter::Segmenter::SequenceId sequenceBase = {0};

            if (aConfig.vdMode == VDMode::MultiQ)
            {
                aView.setupMultiQSubpictureAdSets(dashConfig, streamIds, dashAdSetIds, *mDashDurations, aOutput,
                                                  aTilesXY, sequenceBase, sequenceStep);
            }
            else if (aConfig.vdMode == VDMode::MultiRes5K || aConfig.vdMode == VDMode::MultiRes6K)
            {
                aView.setupMultiResSubpictureAdSets(dashConfig, streamIds, dashAdSetIds, *mDashDurations, aOutput, sequenceBase, sequenceStep);
            }

            for (auto id : streamIds)
            {
                aView.addVideoAdaptationId(id);
            }

            // then create one or more for the extractor(s)
            std::vector<TileDirectionConfig>::iterator direction =
                aTileProxyConfig.tileMergingConfig.directions.begin();
            for (StreamAndTrack extractorId : aTileProxyConfig.extractors)
            {
                AdaptationConfig adaptationConfig;

                StreamId streamId = extractorId.first;
                adaptationConfig.projectionFormat =
                    mpdProjectionFormatOfOmafProjectionType(aView.getProjection().projection);
                adaptationConfig.adaptationSetId = streamId;
                std::string segmentName = getDash()->getBaseName();
                if (aConfig.vdMode != VDMode::MultiQ)
                {
                    segmentName = getDash()->getBaseName() + "." + direction->label;
                }

                DashSegmenterMetaConfig dashMetaConfig{};
                dashMetaConfig.viewId = aView.getId();
                dashMetaConfig.viewLabel = aView.getLabel();
                dashMetaConfig.dashDurations = *mDashDurations;
                dashMetaConfig.timeScale = aConfig.videoTimeScale;
                dashMetaConfig.trackId = extractorId.second;
                dashMetaConfig.streamId = streamId;
                dashMetaConfig.fileMeta = mFileMeta;
                assert(sequenceBase != sequenceStep);
                FragmentSequenceGenerator fragmentSequenceGenerator{sequenceBase, sequenceStep};
                dashMetaConfig.fragmentSequenceGenerator = fragmentSequenceGenerator;

                ISOBMFF::Optional<VDD::DashSegmenterConfig> dashSegmenterConfig =
                    makeOptional(getDash()->makeDashSegmenterConfig(
                        dashConfig, aOutput.withExtractorSet(), segmentName, dashMetaConfig));

                StreamFilter streamFilter( { streamId } );
                if (aConfig.vdMode == VDMode::MultiQ)
                {
                    // add all tile tracks
                    OmafTileSets allTileSets;
                    for (const InputVideo& inputVideo: aConfig.mInputVideos)
                    {
                        auto& tiles = inputVideo.mTileProducerConfig.tileConfig;
                        std::copy(tiles.begin(), tiles.end(), std::back_inserter(allTileSets));
                    }

                    getDash()->addAdditionalVideoTracksToExtractorInitConfig(
                        extractorId.second, dashSegmenterConfig.get().segmenterInitConfig,
                        streamFilter, allTileSets,
                        aConfig.videoTimeScale, streamIds, aOutput);
                }
                else
                {
                    // add the relevant tile tracks
                    assert(direction != aTileProxyConfig.tileMergingConfig.directions.end());
                    getDash()->addPartialAdaptationSetsToMultiResExtractor(*direction, streamIds, adaptationConfig.adaptationSetId);
                    for (auto& video : aConfig.mInputVideos)
                    {
                        if (video.abrIndex == 0u)
                        {
                            // we need to add all video track ids to all
                            // extractor init segments
                            getDash()->addAdditionalVideoTracksToExtractorInitConfig(
                                extractorId.second, dashSegmenterConfig.get().segmenterInitConfig,
                                streamFilter, video.mTileProducerConfig.tileConfig,
                                aConfig.videoTimeScale, streamIds, aOutput);
                        }
                    }
                    direction++;
                }
                getDash()->addBasePreselectionToAdaptationSets(dashAdSetIds, adaptationConfig.adaptationSetId);

                appendScalTrafIndexMap(
                    dashSegmenterConfig.get()
                        .segmenterAndSaverConfig.segmenterConfig.trackToScalTrafIndexMap,
                    dashSegmenterConfig.get().segmenterInitConfig, extractorId.second);

                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                configureOutputInfo.dashIds.push_back({adaptationConfig.adaptationSetId,
                                                       dashSegmenterConfig->representationId,
                                                       extractorId.second});

                // make the pipeline for the extractor adaptation set
                buildPipeline(&aView, dashSegmenterConfig, aOutput.withExtractorSet(),
                              {{aOutput.withExtractorSet(),
                                {aConfig.mTileProxyDash->getSource(), streamFilter}}},
                              nullptr, aPipelineBuildInfo.map([=](const PipelineBuildInfo& i) {
                                  return i.withStreamId(streamId);
                              }));

                RepresentationConfig representationConfig{
                    {},
                    *dashSegmenterConfig,
                    /*assocationId*/ {},
                    /*associationType*/ {},
                    /*allMediaAssociationViewpoint*/ {},
                    /*storeContentComponent*/
                    {
                        StoreContentComponent::TrackId  // maybe not needed for extractor tracks
                    }};

                adaptationConfig.representations.push_back(
                    Representation{aConfig.mTileProxyDash->getSource(),
                                   representationConfig});

                DashMPDConfig dashMPDConfig{};
                dashMPDConfig.segmenterOutput = aOutput;
                dashMPDConfig.mpdViewIndex = aConfig.newMpdViewId();
                dashMPDConfig.adaptationConfig = adaptationConfig;
                dashMPDConfig.supportingAdSets = dashAdSetIds;
                if (getDash()->getOutputMode() == OutputMode::OMAFV2)
                {
                    dashMPDConfig.supportingAdSets->push_back(aView.getIndexStreamId());
                }

                getDash()->configureMPD(dashMPDConfig);

                // for overlay references
                aView.addMainVideoAdaptationId(adaptationConfig.adaptationSetId);

                // for invo references
                aView.addVideoRepresentationId(dashSegmenterConfig->representationId);

                ++sequenceBase;
            }

            aView.handleMoofCombine(aOutput);
        }
        else if (((*mConfig)["mp4"]))
        {
            VDD::AsyncProcessor* mp4Sink = nullptr;

            auto tileProxyMP4 = std::make_unique<TileProxyConnector>(*mGraph, aTileProxyFactory());

            std::string input;
            if (aConfig.vdMode == VDMode::MultiQ)
            {
                if ((*mConfig)["mp4"]["input"])
                {
                    input = readFilename((*mConfig)["mp4"]["input"]);
                }
                else
                {
                    // must not write more than 1 input; find the best quality
                    uint8_t bestQ = 255;
                    for (auto& inVideo : aConfig.mInputVideos)
                    {
                        if (inVideo.quality < bestQ)
                        {
                            bestQ = inVideo.quality;
                            input = inVideo.label;
                        }
                    }
                }
            }
            std::string tile = "";
            if (((*mConfig)["mp4"]["tile"]))
            {
                tile = readString((*mConfig)["mp4"]["tile"]);
            }
            StreamAndTrackIds ids;
            std::string outputName = readFilename((*mConfig)["mp4"]["filename"]);
            std::string inputLabel = "";

            // go through the inputs in config
            for (InputVideo& inVideo : aView.getInputVideosPrimaryFirst())
            {
                bool inputMatched = false;
                if (!input.empty())
                {
                    if (input.compare(inVideo.label) == 0)
                    {
                        // Also write all MultiQ data to a the same file (in
                        // presence of Views, so multiple distinct videos)
                        inputLabel = "all";
                        inputMatched = true;
                    }
                }
                else
                {
                    // writes all video tracks to MP4, in multires case we need
                    // tracks from more than 1 input video to get anything
                    // decodable
                    inputLabel = "all";
                    inputMatched = true;
                }
                if (inputMatched && inputLabel != "" && inVideo.abrIndex == 0u)
                {
                    // match the label with the tile config
                    for (auto& tileConfig : inVideo.mTileProducerConfig.tileConfig)
                    {
                        if (tile.empty())
                        {
                            ids.push_back({tileConfig.streamId, tileConfig.trackId});
                        }
                        else
                        {
                            if (tile.compare(tileConfig.label) == 0)
                            {
                                // match found
                                ids.push_back({tileConfig.streamId, tileConfig.trackId});
                                break;
                            }
                        }
                    }
                    aView.associateTileProducer(inVideo, extractorMode);
                    if (aConfig.vdMode == VDMode::MultiQ)
                    {
                        // only 1 extractor needed for multi-Q/single-res
                        // case
                        extractorMode = NO_EXTRACTOR;
                    }

                    // connect the tile producer to the tile proxy
                    connect(*inVideo.mTileProducer, *tileProxyMP4->getSink(inVideo.getStreamIds()),
                            StreamFilter(inVideo.getStreamIds())).ASYNC_HERE;
                }
            }
            // configuration error, if no inputName matching
            if (inputLabel.empty())
            {
                throw ConfigValueInvalid(
                    "MP4 aOutput configuration error, input video not matching", (*mConfig)["mp4"]);
            }

            outputName = Utils::replace(outputName, "mp4", inputLabel);
            if (!tile.empty())
            {
                outputName += "." + tile;
            }

            if (auto mp4vrOutput = createMP4Output(aPipelineMode, outputName))
            {
                MP4VRWriter::Sink sink;
                Optional<VRVideoConfig> videoConfig;
                if (tile.empty())
                {
                    // Note: 3rd parameter is bool, and doesn't tie video(0) to
                    // the aOutput in any ways. In multires case we still write
                    // only 1 extractor == 1 direction, and it happens to be the
                    // first one on the list
                    videoConfig = makeOptional(VRVideoConfig(
                        {ids,
                         OutputMode::OMAFV1,
                         (aConfig.mInputVideos.at(0).mTileProducerConfig.tileConfig.size() > 1),
                         aExtractorStreamAndTrackForMP4VR,
                         {} /* overlays */}));
                }
                else
                {
                    // just a single subpicture track, no extractor
                    videoConfig = makeOptional(VRVideoConfig(
                        {ids,
                         OutputMode::OMAFV1,
                         (aConfig.mInputVideos.at(0).mTileProducerConfig.tileConfig.size() > 1),
                         {},
                         {}}));
                }
                sink = mp4vrOutput->createSink(videoConfig, aOutput, aConfig.videoTimeScale);
                mp4Sink = sink.sink;
                for (auto id : sink.trackIds)
                {
                    aView.addVideoTrackId(id);
                }
            }

            buildPipeline(&aView, {}, aOutput,
                          {{aOutput, {tileProxyMP4->getSource(), allStreams}}}, mp4Sink,
                          aPipelineBuildInfo);
        }

        return configureOutputInfo;
    }

    void OmafVDController::addToLabelEntityIdMapping(const LabelEntityIdMapping& aMapping)
    {
        mergeLabelEntityIdMapping(mEntityGroupReadContext.labelEntityIdMapping, aMapping);
    }

    namespace
    {
        StreamSegmenter::MPDTree::Omaf2Viewpoint vipoOfEntity(
            const DashOmaf& aDash, const StreamSegmenter::Segmenter::VipoEntityGroupSpec& spec)
        {
            using namespace StreamSegmenter::MPDTree;
            Omaf2Viewpoint r{};
            r.id = std::to_string(spec.viewpointId);
            auto& vi = r.viewpointInfo;
            vi.label = spec.viewpointLabel;
            vi.position.x = spec.viewpointPos.viewpointPosX;
            vi.position.y = spec.viewpointPos.viewpointPosY;
            vi.position.z = spec.viewpointPos.viewpointPosZ;

            if (auto& gps = spec.viewpointGpsPosition)
            {
                vi.gpsPosition = Omaf2ViewpointGPSPosition();
                vi.gpsPosition->longitude = gps->viewpointGpsposLongitude;
                vi.gpsPosition->latitude = gps->viewpointGpsposLatitude;
                vi.gpsPosition->altitude = gps->viewpointGpsposAltitude;
            }

            if (auto& geo = spec.viewpointGeomagneticInfo)
            {
                vi.geomagneticInfo = Omaf2ViewpointGeomagneticInfo();
                vi.geomagneticInfo->yaw = geo->viewpointGeomagneticYaw;
                vi.geomagneticInfo->pitch = geo->viewpointGeomagneticPitch;
                vi.geomagneticInfo->roll = geo->viewpointGeomagneticRoll;
            }

            auto& gr = spec.viewpointGroup;
            vi.groupInfo.groupId = gr.vwptGroupId;
            if (gr.vwptGroupDescription.numElements)
            {
                vi.groupInfo.groupDescription =
                    std::string(gr.vwptGroupDescription.begin(), gr.vwptGroupDescription.end());
            }

            if (auto& sw = spec.viewpointSwitchingList)
            {
                StreamSegmenter::MPDTree::Omaf2ViewpointSwitchingInfo info;
                for (auto& vws : sw->viewpointSwitching)
                {
                    for (auto& vwsr : vws.viewpointSwitchRegions)
                    {
                        info.switchRegions.push_back({});
                        StreamSegmenter::MPDTree::Omaf2ViewpointSwitchRegion& region =
                            info.switchRegions.back();
                        region.id = std::to_string(vws.destinationViewpointId);
                        region.period = aDash.getDefaultPeriodId();

                        switch (vwsr.region.getKey())
                        {
                        case ISOBMFF::ViewpointRegionType::VIEWPORT_RELATIVE:
                        {
                            region.regionType = 0;
                            auto& x =
                                vwsr.region.at<ISOBMFF::ViewpointRegionType::VIEWPORT_RELATIVE>();
                            StreamSegmenter::MPDTree::Omaf2OneViewpointSwitchRegion oneVp;
                            StreamSegmenter::MPDTree::Omaf2ViewpointRelative vpRelative;
                            vpRelative.rectLeftPct = x.rectLeftPercent;
                            vpRelative.rectTopPct = x.rectTopPercent;
                            vpRelative.rectWidthPct = x.rectWidthtPercent;
                            vpRelative.rectHeightPct = x.rectHeightPercent;
                            oneVp.vpRelative = vpRelative;
                            region.regions.push_back(oneVp);
                            break;
                        }
                        case ISOBMFF::ViewpointRegionType::SPHERE_RELATIVE:
                        {
                            region.regionType = 1;
                            auto& x =
                                vwsr.region.at<ISOBMFF::ViewpointRegionType::SPHERE_RELATIVE>();
                            StreamSegmenter::MPDTree::Omaf2OneViewpointSwitchRegion oneVp;
                            StreamSegmenter::MPDTree::Omaf2SphereRegion sphereRegion;
                            sphereRegion.centreAzimuth = x.sphereRegion.centreAzimuth;
                            sphereRegion.centreElevation = x.sphereRegion.centreElevation;
                            sphereRegion.centreTilt = x.sphereRegion.centreTilt;
                            sphereRegion.azimuthRange = x.sphereRegion.azimuthRange;
                            sphereRegion.elevationRange = x.sphereRegion.elevationRange;
                            sphereRegion.shapeType = static_cast<uint8_t>(x.shapeType);
                            oneVp.sphereRegion = sphereRegion;
                            region.regions.push_back(oneVp);
                            break;
                        }
                        case ISOBMFF::ViewpointRegionType::OVERLAY:
                        {
                            region.regionType = 2;
                            auto& x = vwsr.region.at<ISOBMFF::ViewpointRegionType::OVERLAY>();
                            region.refOverlayId = x;
                            break;
                        }
                        }
                    }
                }
                vi.switchingInfo = info;
            }

            return r;
        }
    }  // namespace

    void OmafVDController::makeEntityGroups()
    {
        StreamSegmenter::Segmenter::MetaSpec groupsListDeclarations =
            readOptional(readEntityGroups(mEntityGroupReadContext))((*mConfig)["entity_groups"])
                .value_or(StreamSegmenter::Segmenter::MetaSpec{});

        auto handleEntityGroup = [&](const StreamSegmenter::Segmenter::EntityToGroupSpec& spec,
                                     std::string fourcc,
                                     const StreamSegmenter::MPDTree::AttributeList attributes) {
            StreamSegmenter::MPDTree::EntityGroup group{};
            group.groupId = spec.groupId;
            group.groupType = fourcc;

            std::list<Future<StreamSegmenter::MPDTree::EntityId>> entityIds;

            std::set<EntityId> visitedEntityIds;
            for (auto id : spec.entityIds)
            {
                const EntityIdReference& refId =
                    mEntityGroupReadContext.labelEntityIdMapping.entityById(id);
                for (auto& reference : refId.references)
                {
                    if (visitedEntityIds.count(reference.entityId) == 0)
                    {
                        visitedEntityIds.insert(reference.entityId);
                        // use futures to handle that pesky refId.representationId
                        entityIds.push_back(mapFuture(
                            reference.representationId,
                            [reference](const RepresentationId& representationId)
                                -> StreamSegmenter::MPDTree::EntityId {
                                StreamSegmenter::MPDTree::EntityId entityId{};
                                entityId.adaptationSetId = reference.adaptationSetId.get();
                                entityId.representationId = representationId;
                                return entityId;
                            }));
                    }
                }
            }

            Optional<StreamSegmenter::MPDTree::Omaf2Viewpoint> vipo;
            if (auto* vipoEntity =
                    dynamic_cast<const StreamSegmenter::Segmenter::VipoEntityGroupSpec*>(&spec))
            {
                vipo = vipoOfEntity(*getDash(), *vipoEntity);
            }

            whenAllOfContainer(entityIds).then(
                [group, attributes, vipo,
                 this](const std::vector<StreamSegmenter::MPDTree::EntityId> entityIds) {
                    StreamSegmenter::MPDTree::EntityGroup group2{group};
                    group2.entities = {entityIds.begin(), entityIds.end()};
                    group2.attributes = attributes;

                    if (mDash)
                    {
                        mDash->addEntityGroup(group2);
                        if (group.groupType == "vipo" && vipo)
                        {
                            for (auto& entityId : entityIds)
                            {
                                auto vi = *vipo;
                                if (makeOptional(group.groupId) == mInitialViewpointId)
                                {
                                    vi.viewpointInfo.initialViewpoint = true;
                                }
                                mDash->updateViewpoint(entityId.adaptationSetId, vi);
                            }
                        }
                    };
                });
        };

        if (mDash)
        {
            for (const auto& ovbg : groupsListDeclarations.ovbgGroups)
            {
                StreamSegmenter::MPDTree::AttributeList attrs;
                handleEntityGroup(ovbg, "ovbg", attrs);
            }
            for (const auto& altr : groupsListDeclarations.altrGroups)
            {
                StreamSegmenter::MPDTree::AttributeList attrs;
                handleEntityGroup(altr, "altr", attrs);
            }
            for (const auto& oval : groupsListDeclarations.ovalGroups)
            {
                StreamSegmenter::MPDTree::AttributeList attrs;
                handleEntityGroup(oval, "oval", attrs);
            }

            for (const auto& vipo : groupsListDeclarations.vipoGroups)
            {
                StreamSegmenter::MPDTree::AttributeList attrs;
                handleEntityGroup(vipo, "vipo", attrs);
            }
        }
        mFileMeta.set(groupsListDeclarations);
    }

    const EntityGroupReadContext& OmafVDController::getEntityGroupReadContext() const
    {
        return mEntityGroupReadContext;
    }

    std::tuple<std::string, Optional<VideoRole>> OmafVDController::readLabelRole(
        const VDD::ConfigValue& aVideoInputConfigurationValue)
    {
        auto label = aVideoInputConfigurationValue.getName();
        Optional<VideoRole> role;
        if (aVideoInputConfigurationValue["role"])
        {
            role = readRole(aVideoInputConfigurationValue["role"]);
        }
        return std::make_tuple(label, role);
    }

    TrackGroupId OmafVDController::newTrackGroupId()
    {
        // TrackIds and TrackGroupIds (and AdaptationSetIds) live in same space
        return TrackGroupId(newTrackId().get());
    }

    MP4VRWriter* OmafVDController::createMP4Output(PipelineMode aMode, std::string aName)
    {
        if (mMP4VRConfig)
        {
            auto key = std::make_pair(aMode, aName);
            if (!mMP4VROutputs.count(key))
            {
                mMP4VROutputs.insert(std::make_pair(
                    key, Utils::make_unique<MP4VRWriter>(*mOps, *mMP4VRConfig, aName, aMode)));
            }

            return mMP4VROutputs[key].get();
        }
        else
        {
            return nullptr;
        }
    }

    void OmafVDController::postponeOperation(PostponeTo, std::function<void(void)> aOperation)
    {
        mPostponedOperations.push_back(std::move(aOperation));
    }

    void OmafVDController::makeTimedMetadata()
    {
        std::list<TimedMetadataDeclaration> timedMetadataDeclarations =
            readOptional(readList("Timed metadata declarations", readTimedMetadataDeclaration))(
                (*mConfig)["timed_metadata"])
                .value_or(std::list<TimedMetadataDeclaration>{});

        for (auto& decl : timedMetadataDeclarations)
        {
            // store initial viewpoint for storing into dash entity groups
            if (auto* invp = decl.atPtr<TimedMetadataType::Invp>())
            {
                if (mInitialViewpointId)
                {
                    throw ConfigValueReadError("At most one viewpoints can be initial");
                }
                mInitialViewpointId = invp->config.initialViewpointSampleEntry.idOfInitialViewpoint;
            }


            TmdTrackContext tmdTrackContext {};
            auto source = createSourceForTimedMetadataTrack(tmdTrackContext, *mOps, decl);
            FrameDuration timescale =
                timescaleForSamples(createSamplesForTimedMetadataTrack(tmdTrackContext, decl), {});


            if (mDash)
            {
                auto adaptationSetId = newAdaptationSetId();
                EntityIdReference refId;
                refId.label = decl.base().refId;
                SingleEntityIdReference reference;
                reference.adaptationSetId = adaptationSetId;
                reference.entityId = {reference.adaptationSetId.get()};
                Promise<RepresentationId> representationId;
#if defined(__GNUC__) || defined(__clang__)
                representationId.setId(__PRETTY_FUNCTION__);
#endif
                reference.representationId = representationId;
                refId.references.push_back(reference);
                mEntityGroupReadContext.labelEntityIdMapping.addLabel(refId);
                // Needs to be postponed because dash segment durations are figured out when loading
                // media files. Also entity group information is used via
                // hookDashTimedMetadata->buildPipeline->makeDashSegmenterConfig->segmenterInitConfig
                // (lol)
                auto trackReferences = decl.base().trackReferences;
                postponeOperation(PostponeTo::Phase3, [=]() {
                    ControllerConfigureForwarderForView configure(*this);
                    HookDashInfo info =
                        hookDashTimedMetadata(source, *mDash, adaptationSetId, configure, *mOps,
                                              {} /*Optional<PipelineBuildInfo>*/, timescale, trackReferences);
                    representationId.listen(info.representationId);
                });
            }

            if (MP4VRWriter* writer =
                    createMP4Output(PipelineModeTimedMetadata::TimedMetadata, "tmd"))
            {
                ControllerConfigureForwarderForView configure(*this);

                TimedMetadataTrackInfo trackInfo =
                    hookMP4TimedMetadata(source, configure, *mOps, {}, *writer,
                                         {} /*Optional<PipelineBuildInfo>*/, timescale);

                // note: mEntityGroupReadContext is not output-specific, so this _really_
                // only works for just one output
                auto& trackIds = trackInfo.mp4vrSink.trackIds;
                assert(trackIds.size() == 1);
                EntityIdReference refId;
                refId.label = decl.base().refId;
                SingleEntityIdReference reference;
                reference.trackId = *trackIds.begin();
                reference.entityId = {reference.trackId.get()};
                reference.representationId = makePromise(std::string("what to put here2"));
                refId.references.push_back(reference);
                mEntityGroupReadContext.labelEntityIdMapping.addLabel(refId);
            }
        }
    }

    void OmafVDController::writeDot()
    {
        if (mDotFile)
        {
            mLog->log(Info) << "Writing " << *mDotFile << std::endl;
            std::ofstream graph(*mDotFile);
            mGraph->graphviz(graph);
        }
    }

    void OmafVDController::run()
    {
        GraphStopGuard graphStopGuard(*mGraph);
        // All done, we can transitively eliminate the vr config nodes
        for (auto& configProcessor : mMediaPipelineEndPlaceholders)
        {
            for (AsyncProcessor* processor : configProcessor.second)
            {
                eliminate(*processor);
            }
        }

        writeDot();

        if (mParseOnly)
        {
            std::cerr << "Info: Not starting graph in parse only mode" << std::endl;
        }
        else
        {
            bool keepWorking = true;

            std::chrono::time_point<std::chrono::steady_clock> prevDotDumpTime =
                std::chrono::steady_clock::now();

            while (keepWorking)
            {
                if (mErrors.size())
                {
                    mGraph->abort();
                }

                keepWorking = mGraph->step(mErrors);

                if (mDumpDotInterval)
                {
                    auto now = std::chrono::steady_clock::now();
                    std::chrono::duration<double> delta = now - prevDotDumpTime;
                    if (delta > std::chrono::duration<double>(*mDumpDotInterval))
                    {
                        writeDot();
                        prevDotDumpTime = now;
                    }
                }
            }

            if (getDash())
            {
                getDash()->writeMpd();
            }

            writeDot();

            for (const auto& view : mViews)
            {
                view.report();
            }
        }

        mLog->log(Info) << "Done!" << std::endl;
    }

    GraphErrors OmafVDController::moveErrors()
    {
        auto errors = std::move(mErrors);
        mErrors.clear();
        return errors;
    }
}  // namespace VDD
