
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
#include "omafvicontroller.h"
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
#include "omaf/omafconfiguratornode.h"

namespace VDD
{
    OmafVIController::OmafVIController(Config aConfig)
        : ControllerBase()
        , mLog(aConfig.log ? aConfig.log : std::shared_ptr<Log>(new ConsoleLog()))
        , mConfig(aConfig.config)
        , mParallel(optionWithDefault(mConfig->root(), "parallel", "use parallel graph evaluation", readBool, true))
        , mGraph(mParallel
                 ? static_cast<GraphBase*>(new ParallelGraph({ optionWithDefault(mConfig->root(), "debug.parallel.perf", "performance logging enabled", readBool, false) }))
                 : static_cast<GraphBase*>(new SequentialGraph()))
        , mWorkPool(ThreadedWorkPool::Config { 2, mParallel ? std::max(1u, std::thread::hardware_concurrency()) : 0u })
        , mDotFile(VDD::readOptional("OmafVIController graph file", readString)((*mConfig).tryTraverse(mConfig->root(), configPathOfString("debug.dot"))))
        , mOps(new ControllerOps(*mGraph, mLog))
        , mDash(mLog, (*mConfig)["dash"], *mOps)
        , mConfigure(new ControllerConfigure(*this))
    {
        if ((*mConfig)["mp4"].valid())
        {
            auto config = MP4VRWriter::loadConfig((*mConfig)["mp4"]);
            config.fragmented = false;
            config.log = mLog;
            mMP4VRConfig = config;
        }

        VideoInput input;
        if (input.setupMP4VideoInput(*mOps, *mConfigure, (*mConfig)["video"], VideoProcessingMode::Passthrough))
        {
            makeVideo();
        }
        if (mInputLeft)
        {
            // Connect the frame counter to be evaluated after each imported image
            connect(*mInputLeft, "Input frame counter", [&](const Views& aViews){
                    if (!aViews[0].isEndOfStream())
                    {
                        mLog->progress(static_cast<size_t>(mFrame));
                        ++mFrame;
                    }
                });
        }

        auto audioInput = (*mConfig)["audio"];
        if (((*mConfig)["dash"]).valid())
        {
            makeAudioDash(*mOps, *mConfigure, mDash, audioInput);
        }
        else
        {
            makeAudioMp4(*mOps, *mConfigure, mMP4VROutputs, audioInput);
        }

        for (auto& labeledMP4VRWriter : mMP4VROutputs)
        {
            labeledMP4VRWriter.second->finalizePipeline();
        }

        mDash.setupMpd("");

        setupDebug();

        writeDot();
    }

    OmafVIController::~OmafVIController()
    {
        // nothing
    }

    void OmafVIController::setupDebug()
    {
    }

    void OmafVIController::makeAudioDashPipeline(const ConfigValue& aDashConfig,
        std::string aAudioName,
        AsyncNode* aAacInput,
        FrameDuration aTimeScale)
    {
        TrackId id = TrackId(1);
        auto audioDashConfig = makeOptional(mDash.makeDashSegmenterConfig(aDashConfig,
            PipelineOutput::Audio,
            aAudioName,
            mDashDurations,
            false,
            aTimeScale, id, {}));

        {
            auto source = buildPipeline(aAudioName, 
                audioDashConfig,
                PipelineOutput::Audio,
                { { PipelineOutput::Audio,{ aAacInput, allViews } } },
                nullptr);

            AdaptationConfig adaptationConfig{};
            adaptationConfig.adaptationSetId = 1;
            adaptationConfig.representations =
            {
                Representation
                {
                    source,
                    RepresentationConfig{ {}, *audioDashConfig }
                }
            };

            mDash.configureMPD(PipelineOutput::Audio, 1,
                adaptationConfig,
                {},
                {});
        }
    }

    AsyncNode* OmafVIController::buildPipeline(std::string aName,
        Optional<DashSegmenterConfig> aDashOutput,
        PipelineOutput aPipelineOutput,
        PipelineOutputNodeMap aSources,
        AsyncProcessor* aMP4VRSink)
    {
        AsyncNode* source = aSources[aPipelineOutput].first;
        
        if (aPipelineOutput == PipelineOutput::Audio)
        {
            // aSources.first is a good choice

            if (aMP4VRSink)
            {
                connect(*source, *aMP4VRSink);
            }
        }
        else
        {
            // for video we need to add OMAF SEI NAL units
            OmafConfiguratorNode::Config config;
            config.counter = nullptr;
            config.videoOutput = aPipelineOutput;
            AsyncProcessor* configNode = mOps->makeForGraph<OmafConfiguratorNode>("Omaf Configurator", config);
            connect(*source, *configNode);
            if (aMP4VRSink)
            {
                connect(*configNode, *aMP4VRSink);
            }
            source = configNode;
        }
        if (aDashOutput)
        {
            AsyncProcessor* segmenterInit = nullptr;
            segmenterInit = mOps->makeForGraph<SegmenterInit>("segmenter init", aDashOutput->segmenterInitConfig);
            auto initSave = aDashOutput ? mOps->makeForGraph<Save>("\"" + aDashOutput->segmentInitSaverConfig.fileTemplate + "\"", aDashOutput->segmentInitSaverConfig) : nullptr;

            connect(*source, *segmenterInit);
            connect(*segmenterInit, *initSave);

            auto segmenter = mOps->makeForGraph<Segmenter>("segmenter", aDashOutput->segmenterAndSaverConfig.segmenterConfig);
            auto save = mOps->makeForGraph<Save>("\"" + aDashOutput->segmenterAndSaverConfig.segmentSaverConfig.fileTemplate + "\"", aDashOutput->segmenterAndSaverConfig.segmentSaverConfig);
            connect(*source, *segmenter, allViews);
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

    void OmafVIController::makeVideo()
    {
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

        if (((*mConfig)["dash"]).valid())
        {
            if (!mGopInfo.fixed)
            {
                throw UnsupportedVideoInput("GOP length must be fixed in input when creating DASH");
            }

            auto dashConfig = mDash.dashConfigFor("media");
            mDashDurations.subsegmentsPerSegment = 1;
            mDashDurations.segmentDuration = StreamSegmenter::Segmenter::Duration(mGopInfo.duration.num, mGopInfo.duration.den);
            AdaptationConfig adaptationConfig;
            adaptationConfig.adaptationSetId = 0;
            std::string mpdName = readString((*mConfig)["dash"]["mpd"]["filename"]);
            std::string baseName = Utils::replace(mpdName, ".mpd", "");
            TrackId id = TrackId(1);
            auto dashSegmenterConfig = makeOptional(mDash.makeDashSegmenterConfig(dashConfig,
                output,
                baseName,
                mDashDurations,
                false,
                mVideoTimeScale, id, {}));
            dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = false;

            auto source = buildPipeline(baseName,
                dashSegmenterConfig,
                output,
                { { output,{ mInputLeft, allViews } } },
                nullptr);


            adaptationConfig.representations.push_back(
                Representation
                {
                    source,
                    RepresentationConfig{ {}, *dashSegmenterConfig }
                });

            mDash.configureMPD(output, 1,
                adaptationConfig,
                mVideoFrameDuration,
                {});

        }
        else // create mp4 output
        {
            std::string outputName = readString((*mConfig)["mp4"]["filename"]);

            VDD::AsyncProcessor* mp4vrSink = nullptr;

            AsyncNode* mp4input = mInputLeft;

            if (mInputRight)
            {
                // 2 tracks
                PipelineMode mode = PipelineMode::VideoSeparate;
                if (auto mp4vrOutput = createMP4VROutputForVideo(mode, outputName))
                {
                    auto videoConfig = makeOptional(VRVideoConfig({ {}, OperatingMode::OMAF, false, {} }));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, PipelineOutput::VideoRight, mVideoTimeScale, mGopInfo.duration);
                }

                auto source = buildPipeline(outputName,
                {},
                    PipelineOutput::VideoLeft,
                    { { PipelineOutput::VideoLeft,{ mp4input, allViews } } },
                    mp4vrSink);

                mp4input = mInputRight;
                if (auto mp4vrOutput = createMP4VROutputForVideo(mode, outputName))
                {
                    auto videoConfig = makeOptional(VRVideoConfig({ {}, OperatingMode::OMAF, false, {} }));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, PipelineOutput::VideoLeft, mVideoTimeScale, mGopInfo.duration);
                }

                source = buildPipeline(outputName,
                {},
                    PipelineOutput::VideoRight,
                    { { PipelineOutput::VideoRight,{ mp4input, allViews } } },
                    mp4vrSink);
            }
            else
            {
                // mono / framepacked
                if (auto mp4vrOutput = createMP4VROutputForVideo(pipelineMode, outputName))
                {
                    auto videoConfig = makeOptional(VRVideoConfig({ {}, OperatingMode::OMAF, false, {} }));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, output, mVideoTimeScale, mGopInfo.duration);
                }

                auto source = buildPipeline(outputName,
                {},
                    output,
                    { { output, { mp4input, allViews } } },
                    mp4vrSink);

            }
        }
    }


    MP4VRWriter* OmafVIController::createMP4VROutputForVideo(PipelineMode aMode,
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

    void OmafVIController::writeDot()
    {
        if (mDotFile)
        {
            mLog->log(Info) << "Writing " << *mDotFile << std::endl;
            std::ofstream graph(*mDotFile);
            mGraph->graphviz(graph);
        }
    }

    void OmafVIController::run()
    {
        clock_t begin = clock();
        bool keepWorking = true;
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
        mLog->log(Info) << "Time to process one frame is "   << sec/mFrame * 1000 << " miliseconds" << std::endl;

        mLog->log(Info) << "Done!" << std::endl;
    }

   GraphErrors OmafVIController::moveErrors()
    {
        auto errors = std::move(mErrors);
        mErrors.clear();
        return errors;
    }
}
