
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
#include "timedmetadata.h"
#include "dash.h"
#include "dashomaf.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "mp4vrwriter.h"
#include "configreader.h"

#include "async/asyncnode.h"
#include "config/config.h"
#include "omaf/omafviewingorientation.h"
#include "processor/staticmetadatagenerator.h"
#include "processor/sequencesource.h"
#include "segmenter/tagtrackidprocessor.h"

namespace VDD
{
    AsyncProcessor* makeInvoInput(ControllerOps& aOps, FrameDuration aSampleDuration,
                                  InitialViewingOrientationSample aSample)
    {
        StaticMetadataGenerator::Config generatorConfig{};
        generatorConfig.metadataType = CodedFormat::TimedMetadataInvo;
        generatorConfig.metadataSamples.push_back(aSample.toBitstream());
        generatorConfig.sampleDuration = aSampleDuration;
        return aOps.makeForGraph<StaticMetadataGenerator>(
            "Initial Viewing Orientation\nTimed Metadata\ngenerator", generatorConfig);
    }

    AsyncProcessor* makeOverlayInput(ControllerOps& aOps, FrameDuration aSampleDuration,
                                     std::vector<MP4VR::OverlayConfigSample>& aSamples)
    {
        StaticMetadataGenerator::Config generatorConfig{};
        generatorConfig.metadataType = CodedFormat::TimedMetadataDyol;

        for (auto& sample : aSamples)
        {
            auto bytes = sample.toBytes();
            generatorConfig.metadataSamples.push_back(
                std::vector<uint8_t>(bytes.begin(), bytes.end()));
        }

        generatorConfig.sampleDuration = aSampleDuration;
        return aOps.makeForGraph<StaticMetadataGenerator>("Overlay\nTimed Metadata\ngenerator",
                                                          generatorConfig);
    }

    namespace {
        std::map<std::string, std::set<TrackId>> resolveTrackReferences(ControllerConfigure& aConfigure, TrackReferences aTrackReferences)
        {
            std::map<std::string, std::set<TrackId>> resolved;
            for (auto [fourcc, refIdLabels] : aTrackReferences)
            {
                for (const EntityIdReference& entityIdRef :
                     aConfigure.resolveRefIdLabels(refIdLabels))
                {
                    assert(entityIdRef.references.size() > 0);
                    if (entityIdRef.references.size() > 1)
                    {
                        throw ConfigValueInvalid(
                            "Cannot resolve one track reference to multiple references",
                            entityIdRef.label.getConfigValue());
                    }
                    resolved[fourcc].insert(entityIdRef.references.front().trackId);
                }
            }
            return resolved;
        }
    }

    HookDashInfo hookDashTimedMetadata(AsyncNode* aTMDSource, DashOmaf& aDash, StreamId aStreamId,
                                       ControllerConfigure& aConfigure,
                                       ControllerOps& aOps,
                                       Optional<PipelineBuildInfo> aPipelineBuildInfo,
                                       Optional<FrameDuration> aTimeScale,
                                       TrackReferences aTrackReferences)
    {
        HookDashInfo hookDashInfo{};
        DashSegmenterMetaConfig dashMetaConfig{};
        dashMetaConfig.dashDurations = aConfigure.getDashDurations();
        dashMetaConfig.timeScale = aTimeScale ? *aTimeScale : aConfigure.getFrameDuration();
        dashMetaConfig.trackId = aConfigure.newTrackId();
        dashMetaConfig.fileMeta = aConfigure.getFileMeta();

        TagTrackIdProcessor::Config taggerConfig{dashMetaConfig.trackId};
        auto taggedSource = aOps.makeForGraph<TagTrackIdProcessor>(
            "track id:=" + std::to_string(dashMetaConfig.trackId.get()), taggerConfig);
        connect(*aTMDSource, *taggedSource);

        Optional<ViewId> viewId;
        if (aConfigure.isView())
        {
            viewId = aConfigure.getViewId();
            dashMetaConfig.viewId = viewId;
            dashMetaConfig.viewLabel = aConfigure.getViewLabel();
        }
        auto representationId = Promise<RepresentationId>();
        hookDashInfo.representationId = representationId.getFuture();
        std::shared_ptr<ControllerConfigure> configure(aConfigure.clone());
        aConfigure.postponeOperation(
            PostponeTo::Phase3, [configure, &aDash, aStreamId, dashMetaConfig, aPipelineBuildInfo,
                                 taggedSource, viewId, representationId, aTrackReferences]() {
                auto dashMetaConfig2 = dashMetaConfig;
                dashMetaConfig2.trackReferences =
                    resolveTrackReferences(*configure, aTrackReferences);

                DashSegmenterConfig dashConfig = aDash.makeDashSegmenterConfig(
                    aDash.dashConfigFor("media"), PipelineOutputTimedMetadata::TimedMetadata,
                    aDash.getBaseName(), dashMetaConfig2);

                representationId.set(dashConfig.representationId);

                // postpone so we can resolve track references
                AdaptationConfig adaptationConfig{};
                adaptationConfig.adaptationSetId = aStreamId;
                adaptationConfig.representations = {Representation{
                    taggedSource, RepresentationConfig{{},
                                                       dashConfig,
                                                       {},  // associationType
                                                       {},  // associationId
                                                       viewId,
                                                       /*storeContentComponent*/ {}}}};

                DashMPDConfig dashMPDConfig{};
                dashMPDConfig.segmenterOutput = PipelineOutputTimedMetadata::TimedMetadata;
                dashMPDConfig.adaptationConfig = adaptationConfig;

                configure->buildPipeline(
                    dashConfig, PipelineOutputTimedMetadata::TimedMetadata,
                    {{PipelineOutputTimedMetadata::TimedMetadata, {taggedSource, allStreams}}},
                    nullptr, aPipelineBuildInfo);
                aDash.configureMPD(dashMPDConfig);
            });
        return hookDashInfo;
    }

    TimedMetadataTrackInfo hookMP4TimedMetadata(AsyncNode* aTMDSource,
                                                ControllerConfigure& aConfigure,
                                                ControllerOps& aOps,
                                                std::list<TrackId> aVideoTrackIds,
                                                MP4VRWriter& aMP4VRWriter,
                                                Optional<PipelineBuildInfo> /*aPipelineBuildInfo*/,
                                                Optional<FrameDuration> aTimeScale,
                                                TrackReferences aTrackReferences)
    {
        TimedMetadataTrackInfo trackInfo{};
        std::map<std::string, std::set<TrackId>> trefs =
            resolveTrackReferences(aConfigure, aTrackReferences);
        std::copy(aVideoTrackIds.begin(), aVideoTrackIds.end(),
                  std::inserter(trefs["cdsc"], trefs["cdsc"].begin()));

        trackInfo.mp4vrSink = aMP4VRWriter.createSink(
            {} /* aVRVideoConfig */, PipelineOutputTimedMetadata::TimedMetadata,
            aTimeScale ? *aTimeScale : aConfigure.getFrameDuration(),
            trefs);

        assert(trackInfo.mp4vrSink.trackIds.size());
        TagTrackIdProcessor::Config taggerConfig {*trackInfo.mp4vrSink.trackIds.begin()};
        auto tagger = aOps.makeForGraph<TagTrackIdProcessor>("Set track id", taggerConfig);
        connect(*aTMDSource, *tagger);
        connect(*tagger, *trackInfo.mp4vrSink.sink);

        return trackInfo;
    }

    InitialViewingOrientationSample readInitialViewingOrientationSample(const ConfigValue& aNode)
    {
        InitialViewingOrientationSample sample{};

        sample.cAzimuth = readDouble(aNode["azimuth_degrees"]);
        sample.cElevation = readDouble(aNode["elevation_degrees"]);
        sample.cTilt = readDouble(aNode["tilt_degrees"]);

        return sample;
    }

    TimedMetadataType readTimedMetadataType(const ConfigValue& aValue)
    {
        // clang-format off
        static std::map<std::string, TimedMetadataType> mapping{
            {"rcvp", TimedMetadataType::Rcvp}
            ,{"invp", TimedMetadataType::Invp}
            ,{"dyol", TimedMetadataType::Dyol}
            ,{"dyvp", TimedMetadataType::Dyvp}
        };
        // clang-format on

        return readMapping("timed metadata type", mapping)(aValue);
    }

    TimedMetadataDeclarationRcvp readTimedMetadataDeclarationRcvp(const ConfigValue& aValue)
    {
        TimedMetadataDeclarationRcvp r;
        r.config.sphereRegionConfig = readSphereRegionConfig(aValue["config"]);
        r.config.recommendedViewportInfo = readRecommendedViewportInfo(aValue["config"]["info"]);
        r.samples =
            readVector("rcvp samples", readInterpolatedSample(readSphereRegionSample(
                                           r.config.sphereRegionConfig)))(aValue["samples"]);
        if (r.samples.size() && r.samples.back().duration.num == 0)
        {
            throw ConfigValueInvalid("Last sample must have duration",
                                     aValue["samples"].childValues().back());
        }
        return r;
    }

    TimedMetadataConfig readTimedMetadataConfig(const ConfigValue& aValue)
    {
        auto type = readTimedMetadataType(aValue["type"]);
        TimedMetadataSampleEntry sampleEntry{};
        switch (type) {
        case TimedMetadataType::Rcvp:
        {
            RcvpConfig config {};
            config.sphereRegionConfig = readSphereRegionConfig(aValue);
            config.recommendedViewportInfo = readRecommendedViewportInfo(aValue["info"]);
            sampleEntry.set<TimedMetadataType::Rcvp>(config);
            break;
        }
        case TimedMetadataType::Invp:
        {
            InvpConfig config {};
            config.initialViewpointSampleEntry.idOfInitialViewpoint = readUint32(aValue["id_of_initial_viewpoint"]);
            sampleEntry.set<TimedMetadataType::Invp>(config);
            break;
        }
        case TimedMetadataType::Dyol:
        {
            DyolConfig config {};
            config.overlayStruct = readOverlayStruct(aValue);
            sampleEntry.set<TimedMetadataType::Dyol>(config);
            break;
        }
        case TimedMetadataType::Dyvp:
        {
            DyvpConfig config;
            config.dynamicViewpointSampleEntry = readDynamicViewpointSampleEntry(aValue);
            sampleEntry.set<TimedMetadataType::Dyvp>(config);
            break;
        }
        }
        TimedMetadataConfig config {};
        config.sampleEntry = sampleEntry;
        config.trackReferences = readOptional(readTrackReferences)(aValue["track_references"])
                                     .value_or(TrackReferences{});
        return config;
    }

    TimedMetadataDeclaration readTimedMetadataDeclaration(const ConfigValue& aValue)
    {
        TimedMetadataDeclaration r;

        TimedMetadataConfig metadataConfig = readTimedMetadataConfig(aValue["sample_entry"]);

        switch (metadataConfig.sampleEntry.getKey())
        {
        case TimedMetadataType::Rcvp:
        {
            auto config = metadataConfig.sampleEntry.at<TimedMetadataType::Rcvp>();
            auto samples = readVector(
                "rcvp samples", readInterpolatedSample(readSphereRegionSample(
                                    config.sphereRegionConfig)))(aValue["samples"]);
            if (samples.size() && samples.back().duration.num == 0)
            {
                throw ConfigValueInvalid("Last sample must have duration",
                                         aValue["samples"].childValues().back());
            }


            TimedMetadataDeclarationRcvp decl;
            decl.refId = readRefIdLabel(aValue["ref_id"]);
            decl.config = config;
            decl.samples = samples;
            r.set<TimedMetadataType::Rcvp>(decl);
            break;
        }
        case TimedMetadataType::Invp:
        {
            auto config = metadataConfig.sampleEntry.at<TimedMetadataType::Invp>();
            auto samples =
                readVector("invp samples", readInterpolatedSample<ISOBMFF::InitialViewpointSample>(
                                               readInitialViewpointSample))(aValue["samples"]);
            if (samples.size() && samples.back().duration.num == 0)
            {
                throw ConfigValueInvalid("Last sample must have duration",
                                         aValue["samples"].childValues().back());
            }

            TimedMetadataDeclarationInvp decl;
            decl.refId = readRefIdLabel(aValue["ref_id"]);
            decl.config = config;
            decl.samples = samples;
            r.set<TimedMetadataType::Invp>(decl);
            break;
        }
        case TimedMetadataType::Dyol:
        {
            auto config = metadataConfig.sampleEntry.at<TimedMetadataType::Dyol>();
            auto samples =
                readVector("dyol samples", readInterpolatedSample<MP4VR::OverlayConfigSample>(
                                               readOverlayConfigSample))(aValue["samples"]);
            if (samples.size() && samples.back().duration.num == 0)
            {
                throw ConfigValueInvalid("Last sample must have duration",
                                         aValue["samples"].childValues().back());
            }

            TimedMetadataDeclarationDyol decl;
            decl.refId = readRefIdLabel(aValue["ref_id"]);
            decl.config = config;
            decl.samples = samples;
            r.set<TimedMetadataType::Dyol>(decl);
            break;
        }
        case TimedMetadataType::Dyvp:
        {
            auto config = metadataConfig.sampleEntry.at<TimedMetadataType::Dyvp>();
            auto samples = readVector(
                "dyvp samples",
                readInterpolatedSample<ISOBMFF::DynamicViewpointSample>(readDynamicViewpointSample(
                    config.dynamicViewpointSampleEntry)))(aValue["samples"]);
            if (samples.size() && samples.back().duration.num == 0)
            {
                throw ConfigValueInvalid("Last sample must have duration",
                                         aValue["samples"].childValues().back());
            }

            TimedMetadataDeclarationDyvp decl;
            decl.refId = readRefIdLabel(aValue["ref_id"]);
            decl.config = config;
            decl.samples = samples;
            r.set<TimedMetadataType::Dyvp>(decl);
            break;
        }
        }

        r.base().trackReferences = metadataConfig.trackReferences;

        return r;
    }

    template <typename Declaration, typename ConfigType, typename SampleType>
    const Data dataOfTimedMetadataSample(const TmdTrackContext& /* aTmdTrackContext */,
                                         const ConfigType& aConfig, Index aIndex,
                                         const SampleType& aSample)
    {
        auto sampleEntry = std::make_shared<typename std::remove_const<
            typename std::remove_reference<decltype(aConfig)>::type>::type>(aConfig);
        ISOBMFF::DynArray<uint8_t> bytes;
        if constexpr (Declaration::ConfigMapping::HasConfigMapping)
        {
            bytes = ISOBMFF::isobmffToBytes(aSample.sample,
                                            typename Declaration::ConfigMapping()(aConfig));
        }
        else if constexpr (Declaration::UseToBytes)
        {
            bytes = aSample.sample.toBytes();
        }
        else
        {
            bytes = ISOBMFF::isobmffToBytes(aSample.sample);
        }
        assert(bytes.numElements);
        CPUDataVector contents{{{bytes.begin(), bytes.end()}}};
        Meta meta(aSample.getCodedFrameMeta(aIndex, Declaration::codedFormat));
        meta.attachTag(SampleEntryTag(sampleEntry));
        assert(meta.getCodedFrameMeta().duration.num > 0);
        return Data(contents, meta);
    }

    template <typename Declaration>
    void createSamplesForType(const TmdTrackContext& aTmdTrackContext, const Declaration* aTimedMetadata, std::vector<Data>& aSamples)
    {
        if (aTimedMetadata)
        {
            auto interpolated = performSampleInterpolation(aTimedMetadata->samples, {});
            for (size_t index = 0; index < interpolated.size(); ++index)
            {
                auto& sample = interpolated[index];
                assert(sample.duration.num > 0);
                auto data = dataOfTimedMetadataSample<Declaration>(
                    aTmdTrackContext, aTimedMetadata->config, Index(index), sample);
                aSamples.push_back(data);
            }
        }
    }

    std::vector<Data> createSamplesForTimedMetadataTrack(
        const TmdTrackContext& aTmdTrackContext, const TimedMetadataDeclaration& aTimedMetadata)
    {
        std::vector<Data> samples;

        // Only one of these does proper work, others skip
        createSamplesForType(aTmdTrackContext, aTimedMetadata.atPtr<TimedMetadataType::Rcvp>(), samples);
        createSamplesForType(aTmdTrackContext, aTimedMetadata.atPtr<TimedMetadataType::Invp>(), samples);
        createSamplesForType(aTmdTrackContext, aTimedMetadata.atPtr<TimedMetadataType::Dyol>(), samples);
        createSamplesForType(aTmdTrackContext, aTimedMetadata.atPtr<TimedMetadataType::Dyvp>(), samples);

        return samples;
    }

    AsyncNode* createSourceForTimedMetadataTrack(const TmdTrackContext& aTmdTrackContext,
                                                 ControllerOps& aOps,
                                                 const TimedMetadataDeclaration& aTimedMetadata)
    {
        std::string label = "";
        switch (aTimedMetadata.getKey())
        {
        case TimedMetadataType::Rcvp:
        {
            label = "rcvp";
            break;
        };
        case TimedMetadataType::Invp:
        {
            label = "invp";
            break;
        };
        case TimedMetadataType::Dyol:
        {
            label = "dyol";
            break;
        };
        case TimedMetadataType::Dyvp:
        {
            label = "dyvp";
            break;
        };
        }

        SequenceSource::Config config{};
        config.samples = createSamplesForTimedMetadataTrack(aTmdTrackContext, aTimedMetadata);
        return aOps.makeForGraph<SequenceSource>(label, config);
    }
}
