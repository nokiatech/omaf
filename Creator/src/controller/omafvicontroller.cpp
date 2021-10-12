
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
#include "omafvicontroller.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>
#include <iterator>
#include <memory>
#include <tuple>
#include <vector>

#include "async/combinenode.h"
#include "async/futureops.h"
#include "async/parallelgraph.h"
#include "async/sequentialgraph.h"
#include "async/throttlenode.h"
#include "audio.h"
#include "buildinfo.h"
#include "common/optional.h"
#include "common/utils.h"
#include "config/config.h"
#include "configreader.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "dash.h"
#include "log/consolelog.h"
#include "log/log.h"
#include "medialoader/mp4loader.h"
#include "mp4vromafinterpolate.h"
#include "mp4vromafreaders.h"
#include "mp4vrwriter.h"
#include "omaf/omafconfiguratornode.h"
#include "overlays.h"
#include "processor/metacapturesink.h"
#include "processor/metagateprocessor.h"
#include "processor/metamodifyprocessor.h"
#include "segmenter/save.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/tagtrackidprocessor.h"
#include "timedmetadata.h"
#include "videoinput.h"

namespace VDD
{
    class OmafVIController::ControllerConfigureForwarder : public ControllerConfigure
    {
    public:
        ControllerConfigureForwarder(OmafVIController& aController);
        ControllerConfigureForwarder(ControllerConfigureForwarder& aSelf);
        ~ControllerConfigureForwarder() override;

        /** @brief Generate a new adaptation set id */
        StreamId newAdaptationSetId() override;

        /** @brief Generate a new adaptation set id */
        TrackId newTrackId() override;

        /** @brief Set the input sources and streams */
        void setInput(AsyncNode* aInputLeft, AsyncNode* aInputRight, VideoInputMode aInputVideoMode,
                      FrameDuration aFrameDuration, FrameDuration aTimeScale,
                      VideoGOP aGopInfo) override;

        /** @brief Is the Controller in mpd-only-mode? */
        bool isMpdOnly() const override;

        /** @brief Retrieve frame duration */
        FrameDuration getFrameDuration() const override;

        /** @see OmafVIController::getDashDurations */
        SegmentDurations getDashDurations() const override;

        /** @brief Builds the audio dash pipeline */
        Optional<AudioDashInfo> makeAudioDashPipeline(const ConfigValue& aDashConfig,
                                                      std::string aAudioName, AsyncNode* aAacInput,
                                                      FrameDuration aTimeScale,
                                                      Optional<PipelineBuildInfo> aPipelineBuildInfo) override;

        /** @see OmafVIController::buildPipeline */
        PipelineInfo buildPipeline(Optional<DashSegmenterConfig> aDashOutput,
                                   PipelineOutput aPipelineOutput, PipelineOutputNodeMap aSources,
                                   AsyncProcessor* aMP4VRSink,
                                   Optional<PipelineBuildInfo> aPipelineBuildInfo) override;

        ControllerConfigureForwarder* clone() override;

    private:
        OmafVIController& mController;
    };

    OmafVIController::ControllerConfigureForwarder::ControllerConfigureForwarder(
        OmafVIController& aController)
        : mController(aController)
    {
    }

    OmafVIController::ControllerConfigureForwarder::ControllerConfigureForwarder(
        OmafVIController::ControllerConfigureForwarder& aSelf)
        : mController(aSelf.mController)
    {
    }

    OmafVIController::ControllerConfigureForwarder::~ControllerConfigureForwarder() = default;

    OmafVIController::ControllerConfigureForwarder*
    OmafVIController::ControllerConfigureForwarder::clone()
    {
        return new OmafVIController::ControllerConfigureForwarder(*this);
    }

    StreamId OmafVIController::ControllerConfigureForwarder::newAdaptationSetId()
    {
        return mController.newAdaptationSetId();
    }

    TrackId OmafVIController::ControllerConfigureForwarder::newTrackId()
    {
        return mController.newTrackId();
    }

    FrameDuration OmafVIController::ControllerConfigureForwarder::getFrameDuration() const
    {
        return mController.mVideoFrameDuration;
    }

    void OmafVIController::ControllerConfigureForwarder::setInput(
        AsyncNode* aInputLeft, AsyncNode* aInputRight, VideoInputMode aInputVideoMode,
        FrameDuration aFrameDuration, FrameDuration aTimeScale, VideoGOP aGopInfo)
    {
        mController.mInputLeft = aInputLeft;
        mController.mInputRight = aInputRight;
        mController.mInputVideoMode = aInputVideoMode;
        mController.mVideoFrameDuration = aFrameDuration;
        mController.mVideoTimeScale = aTimeScale;
        mController.mGopInfo = aGopInfo;
    }

    bool OmafVIController::ControllerConfigureForwarder::isMpdOnly() const
    {
        return false;  // this is a legacy functionality, only related to DASH and a minor external
                       // use case of it, not relevant for the mp4controller, should we get rid of
                       // it?
    }

    Optional<AudioDashInfo> OmafVIController::ControllerConfigureForwarder::makeAudioDashPipeline(
        const ConfigValue& aDashConfig, std::string aAudioName, AsyncNode* aAacInput,
        FrameDuration aTimeScale, Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        return mController.makeAudioDashPipeline(nullptr, aDashConfig, aAudioName, aAacInput,
                                                 aTimeScale, aPipelineBuildInfo);
    }

    PipelineInfo OmafVIController::ControllerConfigureForwarder::buildPipeline(
        Optional<DashSegmenterConfig> aDashOutput, PipelineOutput aPipelineOutput,
        PipelineOutputNodeMap aSources, AsyncProcessor* aMP4VRSink,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        return mController.buildPipeline(nullptr, aDashOutput, aPipelineOutput, aSources,
                                         aMP4VRSink, aPipelineBuildInfo);
    }

    SegmentDurations OmafVIController::ControllerConfigureForwarder::getDashDurations() const
    {
        return mController.getDashDurations();
    }

    OmafVIController::OmafVIController(Config aConfig)
        : ControllerBase(aConfig)
        , mConfigure(std::make_unique<ControllerConfigureForwarder>(*this))
        , mInputVideoMode(VideoInputMode::Mono)
    {
        if ((*mConfig)["mp4"])
        {
            auto config = MP4VRWriter::loadConfig((*mConfig)["mp4"]);
            config.fragmented = false;
            config.log = mLog;
            mMP4VRConfig = config;
            mMP4VRConfig->fileMeta = mFileMeta;
        }

        VideoInput input;
        if (input.setupMP4VideoInput(*mOps, *mConfigure, (*mConfig)["video"],
                                     VideoProcessingMode::Passthrough,
                                     mVideoRefIdLabel))
        {
            makeVideo();
        }

        if (mInputLeft)
        {
            // Connect the frame counter to be evaluated after each imported image
            connect(*mInputLeft, "Input frame counter", [&](const Streams& aStreams) {
                if (!aStreams.isEndOfStream())
                {
                    mLog->progress(static_cast<size_t>(mFrame));
                    ++mFrame;
                }
            });
        }

        if (mInputLeft && readBool((*mConfig)["enable_dummy_metadata"]))
        {
            auto syncFrameSource = mInputLeft;
            VDD::InitialViewingOrientationSample initialViewingOrientationSample{};

            AsyncProcessor* invoInput = nullptr;
            invoInput = makeInvoInput(*mOps, mGopInfo.duration, initialViewingOrientationSample);
            connect(*syncFrameSource, *invoInput);

            AsyncProcessor* overlayInput = nullptr;

            MP4VR::OverlayConfigSample sample1;
            sample1.activeOverlayIds = MP4VR::DynArray<uint16_t>(1);
            sample1.activeOverlayIds[0] = 1;

            MP4VR::OverlayConfigSample sample2;
            sample2.activeOverlayIds = MP4VR::DynArray<uint16_t>(1);
            sample2.activeOverlayIds[0] = 2;

            // switch overlay 1 and 2 all over again
            std::vector<MP4VR::OverlayConfigSample> overlaySampleSequence{sample1, sample2};

            overlayInput = makeOverlayInput(*mOps, FrameDuration(5, 1), overlaySampleSequence);
            connect(*syncFrameSource, *overlayInput);

            for (auto& labeledMP4VRWriter : mMP4VROutputs)
            {
                hookMP4TimedMetadata(invoInput, *mConfigure, *mOps,
                                     {mVideoTrackIds.begin(), mVideoTrackIds.end()},
                                     *labeledMP4VRWriter.second,
                                     {} /*aPipelineBuildInfo*/,
                                     {} /* aTimeScale */);

                // create single overlay metadata track, which refers to every video track
                hookMP4TimedMetadata(overlayInput, *mConfigure, *mOps,
                                     {mVideoTrackIds.begin(), mVideoTrackIds.end()},
                                     *labeledMP4VRWriter.second,
                                     {} /*aPipelineBuildInfo*/,
                                     {} /* aTimeScale */);
            }
        }

        auto audioInput = (*mConfig)["audio"];
        if (audioInput.childValues().size())
        {
            if (mDash)
            {
                makeAudioDash(*mOps, *mConfigure, *mDash, audioInput, {} /*aPipelineBuildInfo*/);
            }
            else
            {
                makeAudioMp4(*mOps, *mConfigure, mMP4VROutputs, audioInput, {} /*aPipelineBuildInfo*/);
            }
        }

        //
        // Read timed metadata tracks configuration from json config and create corresponding
        // sources
        //
        std::list<TimedMetadataDeclaration> timedMetadataDeclarations =
            readOptional(readList("Timed metadata declarations", readTimedMetadataDeclaration))(
                (*mConfig)["timed_metadata"])
                .value_or(std::list<TimedMetadataDeclaration>{});

        for (auto& decl : timedMetadataDeclarations)
        {
            TmdTrackContext tmdTrackContext {};
            auto source = createSourceForTimedMetadataTrack(tmdTrackContext, *mOps, decl);

            for (auto& labeledMP4VRWriter : mMP4VROutputs)
            {
                TimedMetadataTrackInfo trackInfo =
                    hookMP4TimedMetadata(source, *mConfigure, *mOps, {}, *labeledMP4VRWriter.second,
                                         {} /* aPipelineBuildInfo */, {} /* aTimeScale */);

                // note: mEntityGroupReadContext is not output-specific, so this _really_ only works
                // for just one output
                auto& trackIds = trackInfo.mp4vrSink.trackIds;
                assert(trackIds.size() == 1);
                EntityIdReference refId{};
                refId.label = decl.base().refId;
                SingleEntityIdReference reference;
                reference.trackId = *trackIds.begin();
                reference.entityId = {reference.trackId.get()};
                //refId.representationId = makePromise();;
                refId.references.push_back(reference);
                mEntityGroupReadContext.labelEntityIdMapping.addLabel(refId);
            }
        }

        //
        // Read overlay configuration from json config and create corresponding sources
        //

        auto ovlyDeclarations =
            readList("Overlay declarations", readOverlayDeclaration(mEntityGroupReadContext))((*mConfig)["overlays"]);

        StreamId associationId = 300;
        for (auto& ovlyDecl : ovlyDeclarations)
        {
            if (mDash)
            {
                OverlayDashInfo dashInfo{};
                dashInfo.dash = mDash.get();
                dashInfo.dashDurations = *mDashDurations;
                dashInfo.backgroundAssocationIds = mVideoAdaptationIds;
                dashInfo.allocateAdaptationsetId = [&] { return associationId++; };

                auto overlayIdStr = std::to_string(ovlyDecl->ovly.overlays.at(0u).overlayId);

                OverlayVideoTrackInfo overlayInfo =
                    createOverlayMediaTracks(*mConfigure, *mOps, *ovlyDecl, nullptr,
                                             dashInfo.addSuffix("overlay." + overlayIdStr),
                                             {} /*aPipelineBuildInfo*/);

                dashInfo.metadataAssocationIds = overlayInfo.overlayMediaAssociationIds;
                createTimedOverlayMetadataTracks(*mConfigure, *mOps, *ovlyDecl, nullptr, dashInfo,
                                                 overlayInfo, {} /*aPipelineBuildInfo*/);
            }
            else
            {
                OverlayVideoTrackInfo info =
                    createOverlayMediaTracks(*mConfigure, *mOps, *ovlyDecl, &mMP4VROutputs, {}, {} /*aPipelineBuildInfo*/);
                mergeLabelEntityIdMapping(mEntityGroupReadContext.labelEntityIdMapping, info.labelEntityIdMapping);
                OutputOverlayMultiMetaTrackInfo outputMetaInfo = createTimedOverlayMetadataTracks(
                                                                                                  *mConfigure, *mOps, *ovlyDecl, &mMP4VROutputs, {}, info, {} /*aPipelineBuildInfo*/);
                // for (auto& [modeName, metaInfo]: outputMetaInfo)
                // {
                //     if (auto sink = metaInfo.dyol.mp4vrSink)
                //     {
                //         assert(sink->trackIds.size() == 1);
                //         mEntityGroupReadContext.labelEntityIdMapping[modeName][metaInfo.refIdLabel] =
                //         sink->trackIds.size();
                //     }
                //     if (metaInfo.overlayMediaTrackIds.size())
                //     {
                //         // can otherwise happen? what should be do with the references then?

                //     }
                // }
            }
        }

        //
        // Read entity groups from json config and add them to segmenter
        //
        EntityGroupReadContext entityReadContext {};
        EntityGroupId nextEntityGroupId(1500);
        entityReadContext.allocateEntityGroupId = [&](void) {
            return nextEntityGroupId++;
        };
        entityReadContext.labelEntityIdMapping = mEntityGroupReadContext.labelEntityIdMapping;

        StreamSegmenter::Segmenter::MetaSpec groupsListDeclarations =
            readOptional(readEntityGroups(entityReadContext))((*mConfig)["entity_groups"])
                .value_or(StreamSegmenter::Segmenter::MetaSpec{});

        mFileMeta.set(groupsListDeclarations);

        //
        // Finalize
        //

        for (auto& labeledMP4VRWriter : mMP4VROutputs)
        {
            labeledMP4VRWriter.second->finalizePipeline();
        }

        VDD::Projection projection;
        // VI is always equirectangular
        projection.projection = VDD::OmafProjectionType::Equirectangular;
        if (mDash)
        {
            mDash->setupMpd(projection);
        }

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

    PipelineInfo OmafVIController::buildPipeline(View* aView,
                                               Optional<DashSegmenterConfig> aDashOutput,
                                               PipelineOutput aPipelineOutput,
                                               PipelineOutputNodeMap aSources,
                                               AsyncProcessor* aMP4VRSink,
                                               Optional<PipelineBuildInfo> /*aPipelineBuildInfo*/)
    {
        PipelineInfo pipelineInfo {};
        pipelineInfo.source = aSources[aPipelineOutput].first;

        assert(!aView);

        if (aPipelineOutput.getClass() == PipelineClass::Video)
        {
            // for video we need to add OMAF SEI NAL units
            OmafConfiguratorNode::Config config;
            config.counter = nullptr;
            config.videoOutput = aPipelineOutput;
            AsyncProcessor* configNode =
                mOps->makeForGraph<OmafConfiguratorNode>("Omaf Configurator", config);
            connect(*pipelineInfo.source, *configNode);
            pipelineInfo.source = configNode;
        }
        if (aMP4VRSink)
        {
            connect(*pipelineInfo.source, *aMP4VRSink);
        }
        if (aDashOutput)
        {
            AsyncProcessor* segmenterInit = nullptr;
            auto segmenterInitConfig = aDashOutput->segmenterInitConfig;
            segmenterInit = mOps->makeForGraph<SegmenterInit>("segmenter init",
                                                              aDashOutput->segmenterInitConfig);
            AsyncProcessor* save = nullptr;

            connect(*pipelineInfo.source, *segmenterInit);
            if (auto& segmenterInitSaverConfig = aDashOutput->segmentInitSaverConfig)
            {
                auto initSave =
                    mOps->makeForGraph<Save>("\"" + segmenterInitSaverConfig->fileTemplate + "\"",
                                             *segmenterInitSaverConfig);
                connect(*segmenterInit, *initSave);
            }
            else if (auto& singleFileSaverConfig = aDashOutput->singleFileSaverConfig)
            {
                save = mOps->makeForGraph<SingleFileSave>(
                    "\"" + singleFileSaverConfig->filename + "\"", *singleFileSaverConfig);
                connect(*segmenterInit, *save);
            }
            else
            {
                assert(false);
            }

            auto segmenter = mOps->makeForGraph<Segmenter>(
                "segmenter", aDashOutput->segmenterAndSaverConfig.segmenterConfig);

            pipelineInfo.segmentSink = segmenter;

            connect(*pipelineInfo.source, *segmenter, allStreams);
            if (auto& config = aDashOutput->segmenterAndSaverConfig.segmentSaverConfig)
            {
                save = mOps->makeForGraph<Save>("\"" + config->fileTemplate + "\"", *config);
            }
            connect(*segmenter, *save);

            if (aPipelineOutput != PipelineOutputAudio::Audio)
            {
                mDash->hookSegmentSaverSignal(*aDashOutput, segmenter);
                mDash->hookSegmentSaverSignal(*aDashOutput, save);
            }
        }

        return pipelineInfo;
    }

    void OmafVIController::makeVideo()
    {
        PipelineOutput output = PipelineOutputVideo::Mono;
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

        if (((*mConfig)["dash"]))
        {
            if (!mGopInfo.fixed)
            {
                throw UnsupportedVideoInput("GOP length must be fixed in input when creating DASH");
            }

            auto dashConfig = mDash->dashConfigFor("media");
            mDashDurations = SegmentDurations();
            mDashDurations->subsegmentsPerSegment = 1;
            mDashDurations->segmentDuration =
                StreamSegmenter::Segmenter::Duration(mGopInfo.duration.num, mGopInfo.duration.den);
            AdaptationConfig adaptationConfig;
            adaptationConfig.adaptationSetId = 1;
            mVideoAdaptationIds.push_back(adaptationConfig.adaptationSetId);
            std::string mpdName = readString((*mConfig)["dash"]["mpd"]["filename"]);
            std::string baseName = Utils::replace(mpdName, ".mpd", "");
            mDash->setBaseName(baseName);
            TrackId id = newTrackId();

            DashSegmenterMetaConfig dashMetaConfig{};
            dashMetaConfig.dashDurations = *mDashDurations;
            dashMetaConfig.timeScale = mVideoTimeScale;
            dashMetaConfig.trackId = id;

            auto dashSegmenterConfig = makeOptional(
                mDash->makeDashSegmenterConfig(dashConfig, output, baseName, dashMetaConfig));
            dashSegmenterConfig.get().segmenterInitConfig.packedSubPictures = false;

            PipelineInfo info = buildPipeline(nullptr, dashSegmenterConfig, output,
                                              {{output, {mInputLeft, allStreams}}}, nullptr,
                                              {} /*aPipelineBuildInfo*/);

            adaptationConfig.representations.push_back(Representation{
                info.source, RepresentationConfig{{}, *dashSegmenterConfig, {}, {}, {}, {}}});

            DashMPDConfig dashMPDConfig{};
            dashMPDConfig.segmenterOutput = output;
            dashMPDConfig.adaptationConfig = adaptationConfig;

            mDash->configureMPD(dashMPDConfig);
        }
        else  // create mp4 output
        {
            std::string outputName = readString((*mConfig)["mp4"]["filename"]);

            MP4VRWriter::Sink mp4vrSink{};

            AsyncNode* mp4input = mInputLeft;

            if (mInputRight)
            {
                // 2 tracks
                PipelineMode mode = PipelineModeVideo::Separate;
                if (auto mp4vrOutput = createMP4VROutputForVideo(mode, outputName))
                {
                    auto videoConfig =
                        makeOptional(VRVideoConfig({{}, OutputMode::OMAFV1, false, {}, {}}));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, PipelineOutputVideo::Right,
                                                        mVideoTimeScale);
                }
                std::copy(mp4vrSink.trackIds.begin(), mp4vrSink.trackIds.end(),
                          std::inserter(mVideoTrackIds, mVideoTrackIds.begin()));

                buildPipeline(nullptr, {}, PipelineOutputVideo::Left,
                              {{PipelineOutputVideo::Left, {mp4input, allStreams}}},
                              mp4vrSink.sink, {} /*aPipelineBuildInfo*/);

                mp4input = mInputRight;
                if (auto mp4vrOutput = createMP4VROutputForVideo(mode, outputName))
                {
                    auto videoConfig =
                        makeOptional(VRVideoConfig({{}, OutputMode::OMAFV1, false, {}, {}}));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, PipelineOutputVideo::Left,
                                                        mVideoTimeScale);
                }
                std::copy(mp4vrSink.trackIds.begin(), mp4vrSink.trackIds.end(),
                          std::inserter(mVideoTrackIds, mVideoTrackIds.begin()));

                buildPipeline(nullptr, {}, PipelineOutputVideo::Right,
                              {{PipelineOutputVideo::Right, {mp4input, allStreams}}},
                              mp4vrSink.sink, {} /*aPipelineBuildInfo*/);
            }
            else
            {
                // mono / framepacked
                if (auto mp4vrOutput = createMP4VROutputForVideo(pipelineMode, outputName))
                {
                    auto videoConfig =
                        makeOptional(VRVideoConfig({{}, OutputMode::OMAFV1, false, {}, {}}));
                    mp4vrSink = mp4vrOutput->createSink(videoConfig, output, mVideoTimeScale);
                }
                std::copy(mp4vrSink.trackIds.begin(), mp4vrSink.trackIds.end(),
                          std::inserter(mVideoTrackIds, mVideoTrackIds.begin()));

                if (mp4vrSink.trackIds.size() && mVideoRefIdLabel)
                {
                    EntityIdReference refId;
                    refId.label = *mVideoRefIdLabel;
                    SingleEntityIdReference reference;
                    reference.trackId = *mp4vrSink.trackIds.begin();
                    reference.entityId = {reference.trackId.get()};
                    refId.references.push_back(reference);
                    mEntityGroupReadContext.labelEntityIdMapping.addLabel(refId);
                }

                buildPipeline(nullptr, {}, output, {{output, {mp4input, allStreams}}},
                              mp4vrSink.sink, {} /*aPipelineBuildInfo*/);
            }
        }
    }


    MP4VRWriter* OmafVIController::createMP4VROutputForVideo(PipelineMode aMode, std::string aName)
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
        GraphStopGuard graphStopGuard(*mGraph);
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

        mGraph->stop();
        if (mDash)
        {
            mDash->writeMpd();
        }

        writeDot();

        clock_t end = clock();
        double sec = (double)(end - begin) / CLOCKS_PER_SEC;
        mLog->log(Info) << "Time to process one frame is " << sec / mFrame * 1000 << " miliseconds"
                        << std::endl;

        mLog->log(Info) << "Done!" << std::endl;
    }

    GraphErrors OmafVIController::moveErrors()
    {
        auto errors = std::move(mErrors);
        mErrors.clear();
        return errors;
    }
}  // namespace VDD
