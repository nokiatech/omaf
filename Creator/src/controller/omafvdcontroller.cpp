
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>
#include <ctime>
#include <tuple>
#include <cmath>

#include "buildinfo.h"
#include "omafvdcontroller.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "videoinput.h"
#include "configreader.h"
#include "audio.h"
#include "mp4vrwriter.h"
#include "async/combinenode.h"
#include "async/futureops.h"
#include "async/sequentialgraph.h"
#include "async/throttlenode.h"
#include "async/parallelgraph.h"
#include "common/utils.h"
#include "common/optional.h"
#include "config/config.h"
#include "log/log.h"
#include "log/consolelog.h"
#include "mp4loader/mp4loader.h"
#include "processor/metacapturesink.h"
#include "processor/metagateprocessor.h"
#include "segmenter/save.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/tagtrackidprocessor.h"
#include "omaf/tileproxy.h"
#include "omaf/tileproxymultires.h"
#include "omaf/tileconfigurations.h"


namespace VDD
{
    namespace
    {
        VideoInputMode readVideoInputMode(const VDD::ConfigValue& aValue)
        {
            std::string modeStr = Utils::lowercaseString(readString(aValue));
            if (modeStr == "topbottom")
            {
                return VideoInputMode::TopBottom;
            }
            else if (modeStr == "sidebyside")
            {
                return VideoInputMode::LeftRight;
            }
            else if (modeStr == "mono")
            {
                return VideoInputMode::Mono;
            }
            else
            {
                throw ConfigValueInvalid("Invalid input_mode", aValue);
            }
        }

        TrackToScalTrafIndexMap buildScalTrafIndexMap(const SegmenterInit::Config& aSegmenterInitConfig, TrackId aExtractorTrackId)
        {
            TrackToScalTrafIndexMap m;

            int8_t index = 1;
            for (auto trackId : aSegmenterInitConfig.tracks.at(aExtractorTrackId).trackReferences.at("scal"))
            {
                m[aExtractorTrackId][trackId] = index;
                ++index;
            }

            assert(m.size());
            return m;
        }
    }

    OmafVDController::OmafVDController(Config aConfig)
        : ControllerBase()
        , mLog(aConfig.log ? aConfig.log : std::shared_ptr<Log>(new ConsoleLog()))
        , mConfig(aConfig.config)
        , mParallel(optionWithDefault(mConfig->root(), "parallel", "use parallel graph evaluation", readBool, true))
        , mGraph(mParallel
                 ? static_cast<GraphBase*>(new ParallelGraph({
                                               optionWithDefault(mConfig->root(), "debug.parallel.perf", "performance logging enabled", readBool, false),
                                               optionWithDefault(mConfig->root(), "debug.dump", "OmafVDController debug dump", readBool, false),
                                           }))
                 : static_cast<GraphBase*>(new SequentialGraph()))
        , mWorkPool(ThreadedWorkPool::Config { 2, mParallel ? std::max(1u, std::thread::hardware_concurrency()) : 0u })
        , mDotFile(VDD::readOptional("OmafVDController graph file", readString)((*mConfig).tryTraverse(mConfig->root(), configPathOfString("debug.dot"))))
        , mOps(new ControllerOps(*mGraph, mLog))
        , mDash(mLog, (*mConfig)["dash"], *mOps)
        , mConfigure(new ControllerConfigure(*this))
        , mVDMode(VDMode::Invalid)
    {

        if ((*mConfig)["mp4"].valid())
        {
            auto config = MP4VRWriter::loadConfig((*mConfig)["mp4"]);
            config.fragmented = false;
            config.log = mLog;
            mMP4VRConfig = config;
        }

        makeVideo();
        if (mInputVideos.size() && mLog->getLogLevel() == LogLevel::Info)
        {
            for (auto& video : mInputVideos)
            {
                if (video.mTileProducerConfig.tileConfig.empty())
                {
                    continue;
                }
                // Connect the frame counter to be evaluated after each imported image
                StreamId streamId = video.mTileProducerConfig.tileConfig.at(0).streamId;
                if (video.mTileProducer != nullptr)
                {
                    connect(*video.mTileProducer, "Input frame counter for " + video.filename, [&, video, streamId](const Views& aViews) {
                        if (!aViews[0].isEndOfStream() && aViews[0].getStreamId() == streamId && aViews[0].getExtractors().empty())
                        {
                            std::unique_lock<std::mutex> framesLock(mFramesMutex);
                            mLog->progress(static_cast<size_t>(mFrames[video.label]), video.filename);
                            ++mFrames[video.label];
                        }
                    });
                }
            }
        }

        for (auto& labeledMP4VRWriter : mMP4VROutputs)
        {
            labeledMP4VRWriter.second->finalizePipeline();
        }

        if ((*mConfig)["dash"].valid())
        {
            mDash.setupMpd(mDashBaseName, mProjection);
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

    AsyncNode* OmafVDController::buildPipeline(std::string aName,
        Optional<DashSegmenterConfig> aDashOutput,
        PipelineOutput aPipelineOutput,
        PipelineOutputNodeMap aSources,
        AsyncProcessor* aMP4VRSink)
    {
        AsyncNode* source = nullptr;
        source = aSources[aPipelineOutput].first;
        ViewMask viewMask = aSources[aPipelineOutput].second;

        if (aMP4VRSink)
        {
            connect(*source, *aMP4VRSink);
        }
        else if (aDashOutput)
        {
            AsyncProcessor* segmenterInit = nullptr;
            if (aDashOutput)
            {
                auto segmenterInitConfig = aDashOutput->segmenterInitConfig;
                segmenterInit = mOps->makeForGraph<SegmenterInit>("segmenter init", segmenterInitConfig);
            }
            auto initSave = aDashOutput ? mOps->makeForGraph<Save>("\"" + aDashOutput->segmentInitSaverConfig.fileTemplate + "\"", aDashOutput->segmentInitSaverConfig) : nullptr;

            connect(*source, *segmenterInit, viewMask);
            connect(*segmenterInit, *initSave);

            auto& sasConf = aDashOutput->segmenterAndSaverConfig;
            auto segmenter = mOps->makeForGraph<Segmenter>("segmenter", sasConf.segmenterConfig);
            auto save = mOps->makeForGraph<Save>("\"" + sasConf.segmentSaverConfig.fileTemplate + "\"", sasConf.segmentSaverConfig);
            connect(*source, *segmenter, viewMask);
            connect(*segmenter, *save);

            if (aPipelineOutput != PipelineOutput::Audio)
            {
                // Perhaps we could do with just hooking only the saver, is there a point in
                // connecting also the segmenter?
                mDash.hookSegmentSaverSignal(*aDashOutput, segmenter);
                mDash.hookSegmentSaverSignal(*aDashOutput, save);
            }
        }

        return source;
    }

    void OmafVDController::makeVideo()
    {
        // create a placeholder config for tile proxy where the tile configs are read
        TileProxy::Config tileProxyConfig;

        // go through the inputs in config
        auto inputVideoConfigs = (*mConfig)["video"];
        std::pair<int, int> tilesXY;
        uint32_t frameCount = 0;

        // as the json configuration lines get ordered alphabetically, we need to make sure we first read common, and only then read the individual video parameters
        for (const VDD::ConfigValue& value : inputVideoConfigs.childValues())
        {
            if (value.getName().find("common") != std::string::npos)
            {
                // common parameters
                readCommonInputVideoParams(value, frameCount);
            }
        }
        // the common-section is mandatory
        if (mVDMode == VDMode::Invalid)
        {
            throw ConfigValueInvalid("Video input configuration error, not enough common parameters given", inputVideoConfigs);
        }
        for (const VDD::ConfigValue& value : inputVideoConfigs.childValues())
        {
            // read other than "common"
            if (value.getName().find("common") == std::string::npos)
            {
                readInputVideoConfig(value, frameCount, tilesXY);
            }
        }

        PipelineOutput output = PipelineOutput::VideoMono;
        PipelineMode pipelineMode = PipelineMode::VideoMono;
        if (mInputVideoMode == VideoInputMode::TopBottom)
        {
            output = PipelineOutput::VideoTopBottom;
            pipelineMode = PipelineMode::VideoTopBottom;
        }
        else if (mInputVideoMode == VideoInputMode::LeftRight)
        {
            output = PipelineOutput::VideoSideBySide;
            pipelineMode = PipelineMode::VideoSideBySide;
        }
        tileProxyConfig.outputMode = output;

        if (mVDMode == VDMode::MultiQ)
        {
            tileProxyConfig.tileCount = tilesXY.first * tilesXY.second;

            StreamAndTrack extractorId;
            extractorId.first = mNextStreamId++;
            extractorId.second = TrackId(tileProxyConfig.tileCount + 1);
            tileProxyConfig.extractors.push_back(extractorId);
        }
        else if (mVDMode == VDMode::MultiRes5K)
        {
            uint8_t bestQ = 255, lowQ = 0;
            TileConfigurations::TiledInputVideo fgVideo;
            TileConfigurations::TiledInputVideo bgVideo;
            for (auto& video : mInputVideos)
            {
                if (video.quality < bestQ)
                {
                    bestQ = video.quality;
                    fgVideo.height = video.height;
                    fgVideo.width = video.width;
                    fgVideo.qualityRank = video.quality;
                    fgVideo.tiles = video.mTileProducerConfig.tileConfig;
                    fgVideo.ctuSize = video.ctuSize;
                }
                if (video.quality > lowQ)
                {
                    lowQ = video.quality;
                    bgVideo.height = video.height;
                    bgVideo.width = video.width;
                    fgVideo.qualityRank = video.quality;
                    bgVideo.tiles = video.mTileProducerConfig.tileConfig;
                    bgVideo.ctuSize = video.ctuSize;
                }
            }
            tileProxyConfig.tileCount = TileConfigurations::createERP5KConfig(tileProxyConfig.tileMergingConfig, fgVideo, bgVideo, tileProxyConfig.extractors);

        }
        else if (mVDMode == VDMode::MultiRes6K)
        {
            if (mInputVideos.size() != 4)
            {
                throw ConfigValueInvalid("Input video count not matching the output_mode", (*mConfig)["video"]);
            }
            uint8_t bestQ = 255, lowQ = 0;
            TileConfigurations::TiledInputVideo fullResVideo;
            TileConfigurations::TiledInputVideo mediumResVideo;
            TileConfigurations::TiledInputVideo mediumResPolarVideo;
            TileConfigurations::TiledInputVideo lowResPolarVideo;
            for (auto& video : mInputVideos)
            {
                if (video.width == 6144)
                {
                    TileConfigurations::crop6KForegroundVideo(video.mTileProducerConfig.tileConfig);
                    rewriteIdsForTiles(video);
                    fullResVideo.tiles = video.mTileProducerConfig.tileConfig;
                    fullResVideo.height = video.height;
                    fullResVideo.width = video.width;
                    fullResVideo.qualityRank = video.quality;
                    fullResVideo.ctuSize = video.ctuSize;
                }
                else if (video.width == 3072)
                {
                    if (video.mTileProducerConfig.tileConfig.size() == 24)
                    {
                        TileConfigurations::crop6KBackgroundVideo(video.mTileProducerConfig.tileConfig);
                        rewriteIdsForTiles(video);
                        mediumResVideo.tiles = video.mTileProducerConfig.tileConfig;
                        mediumResVideo.height = video.height;
                        mediumResVideo.width = video.width;
                        mediumResVideo.qualityRank = video.quality;
                        mediumResVideo.ctuSize = video.ctuSize;
                    }
                    else
                    {
                        TileConfigurations::crop6KPolarVideo(video.mTileProducerConfig.tileConfig);
                        rewriteIdsForTiles(video);
                        mediumResPolarVideo.tiles = video.mTileProducerConfig.tileConfig;
                        mediumResPolarVideo.height = video.height;
                        mediumResPolarVideo.width = video.width;
                        mediumResPolarVideo.qualityRank = video.quality;
                        mediumResPolarVideo.ctuSize = video.ctuSize;
                    }
                }
                else
                {
                    TileConfigurations::crop6KPolarBackgroundVideo(video.mTileProducerConfig.tileConfig);
                    rewriteIdsForTiles(video);
                    lowResPolarVideo.tiles = video.mTileProducerConfig.tileConfig;
                    lowResPolarVideo.height = video.height;
                    lowResPolarVideo.width = video.width;
                    lowResPolarVideo.qualityRank = video.quality;
                    lowResPolarVideo.ctuSize = video.ctuSize;
                }
            }
            tileProxyConfig.tileCount = TileConfigurations::createERP6KConfig(tileProxyConfig.tileMergingConfig, fullResVideo, mediumResVideo, mediumResPolarVideo, lowResPolarVideo, tileProxyConfig.extractors);
        }

        for (InputVideo& video : mInputVideos)
        {
            video.mTileProducerConfig.resetExtractorLevelIDCTo51 =
                mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K;
        }

        std::function<AsyncProcessorWrapper*(std::string aLabel)> tileProxyFactory;

        // create a single tile proxy
        if (mVDMode == VDMode::MultiQ)
        {
            tileProxyFactory = [this, tileProxyConfig](std::string aLabel) {
                auto tileProxy = Utils::make_unique<TileProxy>(tileProxyConfig);
                return mOps->wrapForGraph(aLabel, std::move(tileProxy));
            };
        }
        else
        {
            tileProxyFactory = [this, tileProxyConfig](std::string aLabel) {
                // we use a more advanced proxy, that can also modify the extractors
                auto tileProxy = Utils::make_unique<TileProxyMultiRes>(tileProxyConfig);
                return mOps->wrapForGraph(aLabel, std::move(tileProxy));
            };
        }
        ExtractorMode extractorMode;
        if (mVDMode == VDMode::MultiQ)
        {
            extractorMode = COMMON_EXTRACTOR;
        }
        else
        {
            extractorMode = DEDICATED_EXTRACTOR;
        }

        // create dash/mp4 sinks and connect the tile proxy to them
        if (((*mConfig)["dash"]).valid())
        {
            mTileProxyDash = tileProxyFactory("TileFactory\nfor Dash");
            if ((*mConfig)["dash"]["output_name_base"].valid())
            {
                mDashBaseName = readString((*mConfig)["dash"]["output_name_base"]);
            }
            else
            {
                // use the video output mode
                mDashBaseName = readString((*mConfig)["video"]["common"]["output_mode"]);
            }

            for (InputVideo& video : mInputVideos)
            {
                associateTileProducer(video, extractorMode);

                if (mVDMode == VDMode::MultiQ)
                {
                    // only 1 extractor needed for multi-Q/single-res case
                    extractorMode = NO_EXTRACTOR;
                }

                // connect the tile producer to the tile proxy
                connect(*video.mTileProducer, *mTileProxyDash, allViews);
            }

            if (!mGopInfo.fixed)
            {
                throw UnsupportedVideoInput("GOP length must be fixed in input when creating DASH");
            }
            auto dashConfig = mDash.dashConfigFor("media");
            SegmentDurations dashDurations;
            dashDurations.subsegmentsPerSegment = optionWithDefault((*mConfig)["dash"]["media"], "subsegments_per_segment", "number of subsegments per segment", readUInt, 1u);
            dashDurations.segmentDuration = StreamSegmenter::Segmenter::Duration(mGopInfo.duration.num * dashDurations.subsegmentsPerSegment, mGopInfo.duration.den);


            std::list<StreamId> adSetIds;

            if (mVDMode == VDMode::MultiQ)
            {
                setupMultiQSubpictureAdSets(dashConfig, adSetIds, dashDurations, output, tilesXY);
            }
            else if (mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K)
            {
                setupMultiResSubpictureAdSets(dashConfig, adSetIds, dashDurations, output);
            }

            for (InputVideo& video : mInputVideos)
            {
                mFrames[video.label] = 1;
            }

            // then create one or more for the extractor(s)
            std::vector<TileDirectionConfig>::iterator direction = tileProxyConfig.tileMergingConfig.directions.begin();
            for (StreamAndTrack extractorId : tileProxyConfig.extractors)
            {
                AdaptationConfig adaptationConfig;
                adaptationConfig.adaptationSetId = extractorId.first;
                std::string segmentName = mDashBaseName;
                if (mVDMode != VDMode::MultiQ)
                {
                    segmentName = mDashBaseName + "." + direction->label;
                }

                auto dashSegmenterConfig = makeOptional(mDash.makeDashSegmenterConfig(dashConfig,
                    PipelineOutput::VideoExtractor,
                    segmentName,
                    dashDurations,
                    false,
                    mVideoTimeScale, extractorId.second, extractorId.first));

                if (mVDMode == VDMode::MultiQ)
                {
                    // add all tile tracks
                    mDash.addAdditionalVideoTracksToExtractorInitConfig(
                        extractorId.second, dashSegmenterConfig.get().segmenterInitConfig,
                        mInputVideos.at(0).mTileProducerConfig.tileConfig,
                        mVideoTimeScale, adSetIds);
                }
                else
                {
                    // add the relevant tile tracks
                    assert(direction != tileProxyConfig.tileMergingConfig.directions.end());
                    mDash.addPartialAdaptationSetsToMultiResExtractor(*direction, adSetIds);
                    for (auto& video : mInputVideos)
                    {
                        // we need to add all video track ids to all extractor init segments
                        mDash.addAdditionalVideoTracksToExtractorInitConfig(
                            extractorId.second, dashSegmenterConfig.get().segmenterInitConfig,
                            video.mTileProducerConfig.tileConfig, mVideoTimeScale, adSetIds);
                    }
                    direction++;
                }

                // a mouthful
                dashSegmenterConfig.get()
                    .segmenterAndSaverConfig.segmenterConfig.trackToScalTrafIndexMap =
                    buildScalTrafIndexMap(dashSegmenterConfig.get().segmenterInitConfig,
                                          extractorId.second);

                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                ViewMask mask = allViews;
                auto source = buildPipeline(segmentName,
                    dashSegmenterConfig,
                    PipelineOutput::VideoExtractor,
                    { { PipelineOutput::VideoExtractor, { mTileProxyDash, mask } } },
                    nullptr);


                adaptationConfig.representations.push_back(
                    Representation
                {
                    mTileProxyDash,
                    RepresentationConfig{ {}, *dashSegmenterConfig }
                });

                mDash.configureMPD(output, mMpdViewCount,
                    adaptationConfig,
                    mVideoFrameDuration,
                    adSetIds);

                ++mMpdViewCount;

            }
        }
        else if (((*mConfig)["mp4"]).valid())
        {
            
            VDD::AsyncProcessor* mp4Sink = nullptr;

            mTileProxyMP4 = tileProxyFactory("TileFactory\nfor MP4");

            std::string input;
            if (mVDMode == VDMode::MultiQ)
            {
                if ((*mConfig)["mp4"]["input"].valid())
                {
                    input = readString((*mConfig)["mp4"]["input"]);
                }
                else
                {
                    // must not write more than 1 input; find the best quality
                    uint8_t bestQ = 255;
                    for (auto& inVideo : mInputVideos)
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
            if (((*mConfig)["mp4"]["tile"]).valid())
            {
                tile = readString((*mConfig)["mp4"]["tile"]);
            }
            StreamAndTrackIds ids;
            std::string outputName = readString((*mConfig)["mp4"]["filename"]);
            std::string inputLabel = "";

            // go through the inputs in config
            for (auto& inVideo : mInputVideos)
            {
                if (!input.empty())
                {
                    if (input.compare(inVideo.label) == 0)
                    {
                        inputLabel = inVideo.label;
                    }
                    else
                    {
                        inputLabel = "";
                    }
                }
                else
                {
                    // writes all video tracks to MP4, in multires case we need tracks from more than 1 input video to get anything decodable
                    inputLabel = "all";
                }
                if (inputLabel != "")
                {
                    // match the label with the tile config
                    for (auto& tileConfig : inVideo.mTileProducerConfig.tileConfig)
                    {
                        if (tile.empty())
                        {
                            ids.push_back({ tileConfig.streamId, tileConfig.trackId });
                        }
                        else
                        {
                            if (tile.compare(tileConfig.label) == 0)
                            {
                                // match found
                                ids.push_back({ tileConfig.streamId, tileConfig.trackId });
                                break;
                            }
                        }
                    }
                    associateTileProducer(inVideo, extractorMode);
                    if (mVDMode == VDMode::MultiQ)
                    {
                        // only 1 extractor needed for multi-Q/single-res case
                        extractorMode = NO_EXTRACTOR;
                    }
                    // connect the tile producer to the tile proxy
                    connect(*inVideo.mTileProducer, *mTileProxyMP4, allViews);
                }
            }
            // configuration error, if no inputName matching
            if (inputLabel.empty())
            {
                throw ConfigValueInvalid("MP4 output configuration error, input video not matching", (*mConfig)["mp4"]);
            }
            
            outputName = Utils::replace(outputName, "mp4", inputLabel);
            if (!tile.empty())
            {
                outputName += "." + tile;
            }

            if (auto mp4vrOutput = createMP4Output(pipelineMode, outputName))
            {
                if (tile.empty())
                {
                    // Note: 3rd parameter is bool, and doesn't tie video(0) to the output in any ways. In multires case we still write only 1 extractor == 1 direction, and it happens to be the first one on the list
                    auto videoConfig = makeOptional(VRVideoConfig({ ids, OperatingMode::OMAF, (mInputVideos.at(0).mTileProducerConfig.tileConfig.size() > 1), tileProxyConfig.extractors.front() }));
                    mp4Sink = mp4vrOutput->createSink(videoConfig, output, mVideoTimeScale, mGopInfo.duration);
                }
                else
                {
                    // just a single subpicture track, no extractor
                    auto videoConfig = makeOptional(VRVideoConfig({ ids, OperatingMode::OMAF, (mInputVideos.at(0).mTileProducerConfig.tileConfig.size() > 1), {} }));
                    mp4Sink = mp4vrOutput->createSink(videoConfig, output, mVideoTimeScale, mGopInfo.duration);
                }
            }
            
            auto source = buildPipeline(outputName, {}, 
                output,
                { { output, { mTileProxyDash, allViews } } },
                mp4Sink);
            
        }
    }

    void OmafVDController::setupMultiQSubpictureAdSets(const ConfigValue& aDashConfig, std::list<StreamId>& aAdSetIds, SegmentDurations& aDashDurations, PipelineOutput aOutput, std::pair<int, int> aTilesXY)
    {
        for (size_t tileConfigIndex = 0; tileConfigIndex < mInputVideos.at(0).mTileProducerConfig.tileConfig.size(); tileConfigIndex++) // create adaptation sets - the tile configs are the same, only the stream ids are different
        {
            // traditional loop is used since we need the tileConfigIndex in two places
            auto& tileConfig = mInputVideos.at(0).mTileProducerConfig.tileConfig.at(tileConfigIndex);

            // for representation variants
            std::map<PipelineOutput, AdaptationConfig> outputAdaptationConfigs;
            AdaptationConfig& adaptationConfig = outputAdaptationConfigs[aOutput];
            adaptationConfig.adaptationSetId = tileConfig.streamId;
            aAdSetIds.push_back(adaptationConfig.adaptationSetId);
            for (auto& input : mInputVideos) // create representations
            {
                auto label = input.label + "." + "tile" + std::to_string(tileConfig.streamId);
                std::string baseName = mDashBaseName + '.' + label;

                auto dashSegmenterConfig = makeOptional(mDash.makeDashSegmenterConfig(aDashConfig,
                    aOutput,
                    baseName,
                    aDashDurations,
                    false,
                    mVideoTimeScale, tileConfig.trackId, input.mTileProducerConfig.tileConfig.at(tileConfigIndex).streamId));
                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                ViewMask mask = allViews;
                auto source = buildPipeline(baseName,
                    dashSegmenterConfig,
                    aOutput,
                    { { aOutput,{ mTileProxyDash, mask } } },
                    nullptr);


                adaptationConfig.representations.push_back(
                    Representation
                {
                    input.mTileProducer,
                    RepresentationConfig{ {}, *dashSegmenterConfig }
                });
            }

            // subpictures are always considered single eye, assuming the tiling splits them to separate channels (Note: this is for MultiQ case where tiling in all videos must match)
            PipelineOutput output = aOutput;
            if (aOutput == PipelineOutput::VideoTopBottom)
            {
                if (tileConfig.tileIndex < mInputVideos.front().mTileProducerConfig.tileCount / 2)
                {
                    output = PipelineOutput::VideoLeft;
                }
                else
                {
                    output = PipelineOutput::VideoRight;
                }
            }
            else if (aOutput == PipelineOutput::VideoSideBySide)
            {
                if (tileConfig.tileIndex % aTilesXY.second < aTilesXY.first)
                {
                    output = PipelineOutput::VideoLeft;
                }
                else
                {
                    output = PipelineOutput::VideoRight;
                }
            }
            for (auto& outputAdaptation : outputAdaptationConfigs)
            {
                auto& adaptConfig = outputAdaptation.second;
                mDash.configureMPD(output, mMpdViewCount,
                    adaptConfig,
                    mVideoFrameDuration,
                    {});
            }
            ++mMpdViewCount;
        }

    }

    void OmafVDController::setupMultiResSubpictureAdSets(const ConfigValue& aDashConfig, std::list<StreamId>& aAdSetIds, SegmentDurations& aDashDurations, PipelineOutput aOutput)
    {
        for (auto& input : mInputVideos) 
        {
            int count = 0;
            for (auto& tileConfig : input.mTileProducerConfig.tileConfig)
            {
                AdaptationConfig adaptationConfig;// = outputAdaptationConfigs[aOutput];
                adaptationConfig.adaptationSetId = tileConfig.streamId;
                aAdSetIds.push_back(adaptationConfig.adaptationSetId);

                auto label = input.label + "." + "tile" + std::to_string(count++);
                std::string baseName = mDashBaseName + '.' + label;

                auto dashSegmenterConfig = makeOptional(mDash.makeDashSegmenterConfig(aDashConfig,
                    aOutput,
                    baseName,
                    aDashDurations,
                    false,
                    mVideoTimeScale, tileConfig.trackId, tileConfig.streamId));
                dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = true;
                dashSegmenterConfig->segmenterAndSaverConfig.segmenterConfig.log = mLog;

                ViewMask mask = allViews;
                auto source = buildPipeline(baseName,
                    dashSegmenterConfig,
                    aOutput,
                    { { aOutput,{ mTileProxyDash, mask } } },
                    nullptr);


                adaptationConfig.representations.push_back(
                    Representation
                {
                    input.mTileProducer,
                    RepresentationConfig{ {}, *dashSegmenterConfig }
                });

                mDash.configureMPD(aOutput, mMpdViewCount,
                    adaptationConfig,
                    mVideoFrameDuration,
                    {});
            }
            ++mMpdViewCount;
        }

    }

    void OmafVDController::readCommonInputVideoParams(const VDD::ConfigValue& aValue, uint32_t& aFrameCount)
    {
        std::string projection = readString(aValue["projection"]);
        if (projection.empty() || projection.find("equirect") != std::string::npos)
        {
            // OK, use the default
            mProjection.projection = OmafProjectionType::EQUIRECTANGULAR;
        }
        else if (projection.find("cubemap") != std::string::npos)
        {
            // in future could read the face order & rotation parameters
            mProjection.projection = OmafProjectionType::CUBEMAP;
        };
        if (aValue["output_mode"].valid())
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
        if (aValue["input_mode"].valid())
        {
            std::string frames = readString(aValue["input_mode"]);
            if (frames.find("topbottom") != std::string::npos || frames.find("TopBottom") != std::string::npos)
            {
                mVideoInputMode = VideoInputMode::TopBottom;
            }
            else if (frames.find("sidebyside") != std::string::npos || frames.find("SideBySide") != std::string::npos)
            {
                if (mVDMode == VDMode::MultiRes5K || mVDMode == VDMode::MultiRes6K)
                {
                    throw UnsupportedVideoInput("Stereo for 5K or 6K not yet supported. Stay tuned for updates");
                }
                mVideoInputMode = VideoInputMode::LeftRight;
            }
            else
            {
                mVideoInputMode = VideoInputMode::Mono;
            }
        }
        if (aValue["frame_count"].valid())
        {
            aFrameCount = readUint32(aValue["frame_count"]);
        }
    }

    OmafVDController::InputVideo& OmafVDController::readInputVideoConfig(const VDD::ConfigValue& aVideoInputConfigurationValue, uint32_t aFrameCount, std::pair<int, int>& aTiles)
    {
        InputVideo input;
        input.label = aVideoInputConfigurationValue.getName();
        input.filename = readString(aVideoInputConfigurationValue["filename"]);
        input.quality = std::max((uint8_t)1, (uint8_t)(readUint8(aVideoInputConfigurationValue["quality"])));

        // create a placeholder where the configs are read
        input.mTileProducerConfig.inputFileName = input.filename;
        input.mTileProducerConfig.quality = input.quality;
        // the rest is filled later, if needed

        VideoInput videoInput;
        VideoFileProperties prop = videoInput.getVideoFileProperties(input.filename);

        if (mVideoTimeScale == FrameDuration(0, 1))
        {
            // left and right inputs are not used here, frame duration and timescale are relevant
            mConfigure->setInput(nullptr, 0, nullptr, 0, mVideoInputMode, prop.fps.per1(), prop.timeScale, prop.gopDuration, false);
        }

        uint32_t trackId = 1;
        if (mVDMode != VDMode::MultiQ)
        {
            trackId = mNextStreamId;
        }
        StreamId streamId = mNextStreamId;
        aTiles.first = prop.tilesX;
        aTiles.second = prop.tilesY;

        // putting all tile rows here, may be filtered later on in some configurations
        for (int y = 0; y < prop.tilesY; y++)
        {
            for (int x = 0; x < prop.tilesX; x++)
            {
                OmafTileSetConfiguration tile;
                tile.label = trackId;               
                tile.streamId = streamId++;
                tile.trackId = trackId++;
                tile.tileIndex = y*prop.tilesX + x;

                input.mTileProducerConfig.tileConfig.push_back(tile);
            }
        }
		if (mVDMode != VDMode::MultiRes6K)
		{
            // in 6K, we will revisit the IDs soon
            mNextStreamId = streamId;
		}
        // total tile count in the input, including skipped tile rows; used 
        input.mTileProducerConfig.tileCount = prop.tilesX*prop.tilesY;
        input.width = prop.width;
        input.height = prop.height;
        input.ctuSize = prop.ctuSize;
        input.mTileProducerConfig.frameCount = aFrameCount;

        mInputVideos.push_back(input);

        return mInputVideos.back();
    }

    void OmafVDController::rewriteIdsForTiles(InputVideo& aVideo)
    {
        assert(mVDMode == VDMode::MultiRes6K);
        uint32_t trackId = mNextStreamId;
        for (auto& tile : aVideo.mTileProducerConfig.tileConfig)
        {
            tile.label = trackId;
            tile.streamId = mNextStreamId++;
            tile.trackId = trackId++;
        }
    }

    void OmafVDController::associateTileProducer(InputVideo& aInput, ExtractorMode aExtractorMode)
    {
        // first fill the rest of common parameters
        aInput.mTileProducerConfig.extractorMode = aExtractorMode;
        aInput.mTileProducerConfig.projection = mProjection;

        // create a tile filter (which is also a source - to minimize code rewrite) 
        auto tileProducer = Utils::make_unique<TileProducer>(aInput.mTileProducerConfig);
        aInput.mTileProducer = mOps->wrapForGraph("tileProducer", std::move(tileProducer));
    }

    MP4VRWriter* OmafVDController::createMP4Output(PipelineMode aMode,
                                                           std::string aName)
    {
        if (mMP4VRConfig)
        {
            auto key = std::make_pair(aMode, aName);
            if (!mMP4VROutputs.count(key))
            {
                mMP4VROutputs.insert(std::make_pair(key, Utils::make_unique<MP4VRWriter>(*mOps, *mMP4VRConfig, aName, aMode)));
            }

            return mMP4VROutputs[key].get();
        }
        else
        {
            return nullptr;
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
        clock_t begin = clock();
        bool keepWorking = true;

        writeDot();

        while (keepWorking)
        {
            if (mErrors.size())
            {
                mGraph->abort();
            }

            keepWorking = mGraph->step(mErrors);
        }

        writeDot();

        clock_t end = clock();
        double sec = (double) ( end-begin) / CLOCKS_PER_SEC;
        //TODO not 1 frame

        if (mFrames.size())
        {
            mLog->log(Info) << "Time to process one frame is "   << sec/mFrames.begin()->second * 1000 << " miliseconds" << std::endl;
        }

        mLog->log(Info) << "Done!" << std::endl;
    }

   GraphErrors OmafVDController::moveErrors()
    {
        auto errors = std::move(mErrors);
        mErrors.clear();
        return errors;
    }
}
