
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
#include "viewpoints.h"

#include "controllerops.h"
#include "controllerparsers.h"
#include "dashomaf.h"
#include "mp4vromafreaders.h"
#include "processor/sequencesource.h"
#include "processor/tagprocessor.h"
#include "view.h"

namespace VDD
{
    namespace {
        std::vector<Data> convertDyvpTimedMetaToData(
            const ViewpointMedia& aDyvp, CodedFormat aCodedFormat,
            std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry)
        {
            std::vector<Data> datas;
            assert(aDyvp.samples);

            Index presIndex{};
            CodingIndex codingIndex{};

            for (auto& sample : *aDyvp.samples)
            {
                auto bytes = ISOBMFF::isobmffToBytes(sample.sample, aDyvp.viewpointInfo.getDyvpSampleEntry());
                std::vector<uint8_t> bytesVec{bytes.begin(), bytes.end()};
                CPUDataVector dataVec{{std::move(bytesVec)}};
                CodedFrameMeta codedFrameMeta{};
                codedFrameMeta.presIndex = presIndex;
                codedFrameMeta.codingIndex = codingIndex;
                codedFrameMeta.type = FrameType::IDR;
                codedFrameMeta.format = aCodedFormat;
                codedFrameMeta.codingTime = sample.time;
                codedFrameMeta.presTime = sample.time;
                codedFrameMeta.duration = sample.duration;
                codedFrameMeta.inCodingOrder = true;
                Meta meta(codedFrameMeta);
                if (aSampleEntry)
                {
                    meta.attachTag(SampleEntryTag(aSampleEntry));
                }
                Data data(dataVec, meta);
                datas.push_back(data);

                ++presIndex;
                ++codingIndex;
            }

            return datas;
        }

        StreamSegmenter::MPDTree::Omaf2Viewpoint omaf2ViewpointOfDynamicViewpointSampleEntry(
            std::string aId, std::string aLabel,
            const ISOBMFF::DynamicViewpointSampleEntry& aSampleEntry,
            ViewpointGroupId aViewpointGroupId, bool aIsInitial)
        {
            StreamSegmenter::MPDTree::Omaf2Viewpoint vp;
            vp.id = aId;
            vp.viewpointInfo.label = aLabel;
            StreamSegmenter::MPDTree::Omaf2ViewpointInfo& vpi = vp.viewpointInfo;
            if (auto& vps = aSampleEntry.viewpointPosStruct)
            {
                vpi.position.x = vps->viewpointPosX;
                vpi.position.y = vps->viewpointPosY;
                vpi.position.z = vps->viewpointPosZ;
            }

            vp.viewpointInfo.groupInfo.groupId = aViewpointGroupId.get();
            vp.viewpointInfo.initialViewpoint = aIsInitial ? true : Optional<bool>();
            return vp;
        }
    }  // namespace

    ViewpointMedia readViewpointDeclaration(const ConfigValue& aNode)
    {
        ViewpointMedia media;
        media.viewpointInfo = readViewpointInfo(aNode["initial_sample"]);
        media.label = readString(aNode["label"]);
        media.isInitial = readOptional(readBool)(aNode["initial_viewpoint"]).value_or(false);
        if (aNode["samples"]) {
            auto time = FrameTime(0, 1);
            auto sampleValues = aNode["samples"].childValues();
            media.samples = std::vector<AugmentedDyvpSample>();
            media.samples->reserve(sampleValues.size());
            auto sampleReader = readDynamicViewpointSample(media.viewpointInfo.getDyvpSampleEntry());
            for (auto& sampleNode : sampleValues)
            {
                auto sample = sampleReader(sampleNode);
                AugmentedDyvpSample r{};
                std::tie(r.time, r.duration) = readTimeDuration(sampleNode);
                r.sample = sample;
                media.samples->push_back(r);
            }
        }
        return media;
    }

    namespace
    {
        struct ConvertedSamples {
            AsyncSource* tmd;
            FrameDuration timescale;
        };

        Optional<ConvertedSamples> convertSamples(const ViewpointMedia& aMedia, ControllerOps& aOps)
        {
            if (aMedia.samples)
            {
                SequenceSource::Config sequenceConfig{};
                auto sampleEntry =
                    std::make_shared<StreamSegmenter::Segmenter::DynamicViewpointSampleEntry>();
                sampleEntry->dynamicViewpointSampleEntry = aMedia.viewpointInfo.getDyvpSampleEntry();
                sequenceConfig.samples =
                    convertDyvpTimedMetaToData(aMedia, CodedFormat::TimedMetadataDyvp, sampleEntry);

                AsyncSource* tmd =
                    aOps.makeForGraph<SequenceSource>("dyvp TMD for OMAF", sequenceConfig);

                FrameDuration timescale = timescaleForSamples(sequenceConfig.samples, {});

                return Optional<ConvertedSamples>({tmd, timescale});
            }
            else
            {
                return {};
            }
        }
    }  // namespace

    void createViewpointMetadataTrack(const View& aView, ControllerConfigure& aConfigure,
                                      ControllerOps& aOps, const ViewpointMedia& aMedia,
                                      MP4VROutputs* aMP4VROutputs,
                                      Optional<ViewpointDashInfo> aDashInfo,
                                      const ViewpointMediaTrackInfo& aMediaTrackInfo,
                                      Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        PipelineOutput output{PipelineOutputTimedMetadata::TimedMetadata};

        std::string viewId = std::to_string(aView.getId().get());

        if (aDashInfo && aDashInfo->dash)
        {
            Optional<ConvertedSamples> convertedSamples = convertSamples(aMedia, aOps);

            if (convertedSamples)
            {
                DashOmaf& dash = *aDashInfo->dash;
                auto dashConfig = dash.dashConfigFor("media");
                auto segmentName = dash.getBaseName() + "." + aDashInfo->suffix;

                DashSegmenterMetaConfig dashMetaConfig{};
                dashMetaConfig.viewId = aConfigure.getViewId();
                dashMetaConfig.viewLabel = aConfigure.getViewLabel();
                dashMetaConfig.dashDurations = aDashInfo->dashDurations;
                dashMetaConfig.timeScale = convertedSamples->timescale;
                dashMetaConfig.trackId = aConfigure.newTrackId();

                auto dashSegmenterConfig = dash.makeDashSegmenterConfig(
                    dashConfig, PipelineOutputTimedMetadata::TimedMetadata, segmentName,
                    dashMetaConfig);

                aConfigure.buildPipeline(dashSegmenterConfig, output,
                                         {{output, {convertedSamples->tmd, allStreams}}},
                                         /*MP4VRSink*: */ nullptr, aPipelineBuildInfo);

                AdaptationConfig adaptationConfig{};
                adaptationConfig.adaptationSetId = aConfigure.newAdaptationSetId();
                RepresentationConfig representationConfig{};
                representationConfig.output = dashSegmenterConfig;
                if (aMediaTrackInfo.mediaAssociationIds.size())
                {
                    representationConfig.allMediaAssociationViewpoint = aView.getId();
                }
                Representation representation{};
                representation.representationConfig = representationConfig;
                representation.encodedFrameSource = convertedSamples->tmd;
                adaptationConfig.representations = {representation};

                DashMPDConfig dashMPDConfig{};
                dashMPDConfig.segmenterOutput = output;
                dashMPDConfig.adaptationConfig = adaptationConfig;
                dash.configureMPD(dashMPDConfig);
            }

            // viewpoint information is set to the mpd from entity group handling
        }
        if (aMP4VROutputs && aMP4VROutputs->size())
        {
            Optional<ConvertedSamples> convertedSamples = convertSamples(aMedia, aOps);

            if (convertedSamples)
            {
                std::map<std::string, std::set<TrackId>> trefs;
                trefs["cdtg"] = {aView.getId().get()};

                for (auto& labeledMP4VRWriter : *aMP4VROutputs)
                {
                    auto& mp4vrWriter = labeledMP4VRWriter.second;
                    auto mp4vrSink =
                        mp4vrWriter->createSink({}, output, convertedSamples->timescale, trefs);
                    aConfigure.buildPipeline(/*dashSegmenterConfig*/ {}, output,
                                             {{output, {convertedSamples->tmd, allStreams}}},
                                             /*MP4VRSink*: */ mp4vrSink.sink, aPipelineBuildInfo);
                }
            }
        }
    }
}  // namespace VDD
