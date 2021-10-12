
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
#include "controllerbase.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "dash.h"
#include "view.h"

#include "async/parallelgraph.h"
#include "async/sequentialgraph.h"
#include "log/consolelog.h"
#include "segmenter/tagtrackidprocessor.h"

namespace VDD
{
    namespace PipelineModeDefs
    {
        namespace
        {
            const std::map<std::string, PipelineMode> kPipelineModeNameToType{
                {"mono"        , PipelineModeVideo::Mono},
                {"framepacked" , PipelineModeVideo::FramePacked},
                {"topbottom"   , PipelineModeVideo::TopBottom},
                {"sidebyside"  , PipelineModeVideo::SideBySide},
                {"separate"    , PipelineModeVideo::Separate},
                {"audio"       , PipelineModeAudio::Audio},
                {"meta"        , PipelineModeTimedMetadata::TimedMetadata},
                {"overlay"     , PipelineModeVideo::Overlay}};

            // Note: this now merges all videos to "video", hence eliminating possibility to use
            // two-channel stereo. This is done for simplicity, as OMAF doesn't really consider
            // multi-track videos
            const std::map<PipelineOutputTypeKey, PipelineOutputTypeInfo> kOutputTypeInfo{
                {{PipelineClass::Video         , false}, {"video", "video"}},
                {{PipelineClass::Audio         , false}, {"audio", "audio"}},
                {{PipelineClass::Video         , true }, {"base", "base"}},
                {{PipelineClass::TimedMetadata , false}, {"meta", "meta"}},
                {{PipelineClass::IndexSegment  , false}, {"indexsegment", "index"}}};

            const auto kPipelineModeTypeToName = Utils::reverseMapping(kPipelineModeNameToType);
        }  // namespace
    }  // namespace PipelineModeDefs

    std::string nameOfPipelineMode(PipelineMode aOutput)
    {
        return PipelineModeDefs::kPipelineModeTypeToName.at(aOutput);
    }

    std::string filenameComponentForPipelineOutput(PipelineOutput aOutput)
    {
        return PipelineModeDefs::kOutputTypeInfo.at({aOutput.getClass(), aOutput.isExtractor()})
            .filenameComponent;
    }

    std::string outputNameForPipelineOutput(PipelineOutput aOutput)
    {
        return PipelineModeDefs::kOutputTypeInfo.at({aOutput.getClass(), aOutput.isExtractor()})
            .outputName;
    }

    ControllerBase::ControllerBase(const Config& aConfig)
        : mConfig(aConfig.config)
        , mLog(aConfig.log ? aConfig.log : std::shared_ptr<Log>(new ConsoleLog()))
        , mParallel(optionWithDefault(mConfig->root(), "parallel", readBool, true))
        , mGraph(mParallel
                 ? static_cast<GraphBase*>(new ParallelGraph({
                                               optionWithDefault(mConfig->root(), "debug.parallel.perf", readBool, false),
                                               optionWithDefault(mConfig->root(), "debug.dump", readBool, false),
                                               optionWithDefault(mConfig->root(), "debug.parallel.singlethread", readBool, false),
                                           }))
                 : static_cast<GraphBase*>(new SequentialGraph()))
        , mWorkPool(ThreadedWorkPool::Config { 2, mParallel ? std::max(1u, std::thread::hardware_concurrency()) : 0u })
        , mDotFile(VDD::readOptional(readString)((*mConfig).tryTraverse(mConfig->root(), configPathOfString("debug.dot"))))
        , mOps(new ControllerOps(*mGraph, mLog))
        , mDash((*mConfig)["dash"] ? std::make_unique<DashOmaf>(mLog, (*mConfig)["dash"], *mOps) : std::unique_ptr<DashOmaf>())
        , mDumpDotInterval(VDD::readOptional(readDouble)((*mConfig).tryTraverse(mConfig->root(), configPathOfString("debug.dot_interval"))))
    {
        mEntityGroupReadContext.allocateEntityGroupId = [&](void) {
            // Is this a correct set to pull entity ids from?
            return EntityGroupId(newAdaptationSetId().get());
        };
    }

    ControllerBase::~ControllerBase()
    {

    }

    SegmentDurations ControllerBase::getDashDurations() const
    {
        return *mDashDurations;
    }

    StreamId ControllerBase::newAdaptationSetId()
    {
        if (mDash) {
            return mDash->newId(IdKind::NotExtractor);
        } else {
            return mFakeAdaptationSetIdForMP4++;
        }
    }

    StreamId ControllerBase::newExtractorAdaptationSetIt()
    {
        if (mDash) {
            return mDash->newId(IdKind::Extractor);
        } else {
            return mFakeAdaptationSetIdForMP4++;
        }
    }

    SegmentDurations ControllerBase::makeSegmentDurations(const VideoGOP& aGopInfo, Optional<FrameDuration> aCompleteDuration) const
    {
        SegmentDurations dashDurations;
        dashDurations.completeDuration = aCompleteDuration;
        dashDurations.subsegmentsPerSegment =
            mDash
            ? optionWithDefault(mDash->getConfig()["media"], "subsegments_per_segment", readUInt, 1u)
            : 1u;

        dashDurations.segmentDuration = StreamSegmenter::Segmenter::Duration(
            aGopInfo.duration.num * dashDurations.subsegmentsPerSegment,
            aGopInfo.duration.den);

        // Ensure a minimal segment duration length to avoid large amounts of
        // segments for some videos
        auto defaultMinimumSegmentDuration = StreamSegmenter::Segmenter::Duration(3, 1);
        auto minimumSegmentDuration =
            mDash
            ? optionWithDefault(mDash->getConfig()["media"], "minimum_segment_duration",
                                readGeneric<StreamSegmenter::Segmenter::Duration>("rational"),
                                defaultMinimumSegmentDuration)
            : defaultMinimumSegmentDuration;
        if (dashDurations.segmentDuration < minimumSegmentDuration)
        {
            auto scaleBy = minimumSegmentDuration / dashDurations.segmentDuration;
            if (scaleBy.num % scaleBy.den != 0)
            {
                // Round scaleBy to next integer
                scaleBy.num = scaleBy.num + (scaleBy.den - scaleBy.num % scaleBy.den);
            }
            dashDurations.segmentDuration = dashDurations.segmentDuration * scaleBy;
        }
        return dashDurations;
    }

    Optional<AudioDashInfo> ControllerBase::makeAudioDashPipeline(View* aView,
                                                                  const ConfigValue& aDashConfig,
                                                                  std::string aAudioName,
                                                                  AsyncNode* aAacInput,
                                                                  FrameDuration aTimeScale,
                                                                  Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        AudioDashInfo dashInfo;
        DashSegmenterMetaConfig dashMetaConfig{};
        dashMetaConfig.viewId = aView ? makeOptional(aView->getId()) : ViewId{};
        dashMetaConfig.viewLabel = aView ? aView->getLabel() : Optional<ViewLabel>{};
        dashMetaConfig.dashDurations = *mDashDurations;
        dashMetaConfig.timeScale = aTimeScale;
        dashMetaConfig.trackId = newTrackId();

        TagTrackIdProcessor::Config trackTaggerConfig{dashMetaConfig.trackId};
        auto trackTagger = mOps->makeForGraph<TagTrackIdProcessor>(
            "track id:=" + std::to_string(dashMetaConfig.trackId.get()), trackTaggerConfig);

        connect(*aAacInput, *trackTagger);
        auto source = trackTagger;

        Optional<DashSegmenterConfig> audioDashConfig = makeOptional(mDash->makeDashSegmenterConfig(
            aDashConfig, PipelineOutputAudio::Audio, aAudioName, dashMetaConfig));
        dashInfo.representationId = audioDashConfig->representationId;
        {
            PipelineInfo info =
                buildPipeline(aView, audioDashConfig, PipelineOutputAudio::Audio,
                              {{PipelineOutputAudio::Audio, {source, allStreams}}}, nullptr,
                              aPipelineBuildInfo);

            AdaptationConfig adaptationConfig{};
            adaptationConfig.adaptationSetId = newAdaptationSetId();

            if (aView)
            {
                aView->addAudioAdaptationId(adaptationConfig.adaptationSetId);
            }

            adaptationConfig.representations = {Representation{
                info.source, RepresentationConfig{{}, *audioDashConfig, {}, {}, {}, {}}}};
            dashInfo.representationId = audioDashConfig->representationId;

            if (aView)
            {
                aView->addAudioRepresentationId(dashInfo.representationId);
            }

            DashMPDConfig dashMPDConfig {};
            dashMPDConfig.segmenterOutput = PipelineOutputAudio::Audio;
            dashMPDConfig.adaptationConfig = adaptationConfig;

            mDash->configureMPD(dashMPDConfig);
        }
        return dashInfo;
    }

    const TileIndexTrackGroupIdMap& ControllerBase::getTileIndexTrackGroupMap(PipelineOutput aOutput) const
    {
        return mTileIndexMap.at(aOutput);
    }

    TileIndexTrackGroupIdMap& ControllerBase::getTileIndexTrackGroupMap(PipelineOutput aOutput)
    {
        return mTileIndexMap[aOutput];
    }

    TrackId ControllerBase::newTrackId()
    {
        StreamId id;
        if (mDash) {
            id = mDash->newId(IdKind::TrackId);
        } else {
            id = mFakeAdaptationSetIdForMP4++;
        }
        return TrackId(id.get());
    }

    std::list<EntityIdReference> ControllerBase::resolveRefIdLabels(
        std::list<ParsedValue<RefIdLabel>> aRefIdLabels)
    {
        std::list<EntityIdReference> resolved;
        auto& mapping = mEntityGroupReadContext.labelEntityIdMapping.forward;
        for (auto& label: aRefIdLabels)
        {
            if (mapping.count(label)) {
                resolved.push_back(mapping.at(label));
            }
            else
            {
                throw ConfigValueInvalid("Corresponding entity not found", label.getConfigValue());
            }
        }
        return resolved;
    }
}
