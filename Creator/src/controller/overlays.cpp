
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
#include "overlays.h"

#include "audio.h"
#include "async/futureops.h"
#include "config/config.h"
#include "controllerconfigure.h"
#include "controllerops.h"
#include "controllerparsers.h"
#include "dashomaf.h"
#include "mp4vromafinterpolate.h"
#include "mp4vromafreaders.h"
#include "mp4vrwriter.h"
#include "processor/metamodifyprocessor.h"
#include "processor/sequencesource.h"
#include "segmenter/tagtrackidprocessor.h"

namespace VDD
{
    namespace
    {
        void readRotation(ISOBMFF::Rotation& rotation, ConfigValue& valNode)
        {
            rotation.yaw = readFloatSigned180Degrees(kStepsInDegree)(valNode["yaw_degrees"]);
            rotation.pitch = readFloatSigned90Degrees(kStepsInDegree)(valNode["pitch_degrees"]);
            rotation.roll = readFloatSigned180Degrees(kStepsInDegree)(valNode["roll_degrees"]);
        }

        template <bool HasRange, bool HasInterpolate>
        void fillSphereRegionRange(
            ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate>& /* region */,
            ConfigValue& /* valNode */, bool /* withCentreTilt */)
        {
            // nothing
        }

        template <bool HasRange>
        void fillSphereRegionRange(ISOBMFF::SphereRegionStatic<HasRange, true>& region,
                                   ConfigValue& valNode, bool withCentreTilt)
        {
            region.centreTilt = readOptional(readFloatSigned180Degrees(kStepsInDegree))(valNode["centre_tilt_degrees"]).value_or(0.0);
            if (!withCentreTilt && region.centreTilt)
            {
                throw ConfigValueInvalid(
                    "expected the value to be missing or equal to 0.0 in this context",
                    valNode["centre_tilt_degrees"]);
            }
            region.azimuthRange =
                readFloatUint360Degrees(kStepsInDegree)(valNode["azimuth_range_degrees"]);
            region.elevationRange =
                readFloatUint180Degrees(kStepsInDegree)(valNode["elevation_range_degrees"]);
        }

        template <bool HasRange, bool HasInterpolate>
        void fillSphereRegionInterpolate(
            ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate>& /* region */,
            ConfigValue& /* valNode */)
        {
            // nothing
        }

        template <bool HasInterpolate>
        void fillSphereRegionInterpolate(ISOBMFF::SphereRegionStatic<true, HasInterpolate>& region,
                                         ConfigValue& valNode)
        {
            region.interpolate =
                readOptional(readBool)(valNode["interpolate_flag"]).value_or(false);
        }

        template <bool HasRange, bool HasInterpolate>
        void readSphereRegion(ISOBMFF::SphereRegionStatic<HasRange, HasInterpolate>& region,
                              ConfigValue& valNode, bool withCentreTilt)
        {
            region.centreAzimuth =
                readFloatSigned180Degrees(kStepsInDegree)(valNode["centre_azimuth_degrees"]);
            region.centreElevation =
                readFloatSigned90Degrees(kStepsInDegree)(valNode["centre_elevation_degrees"]);
            fillSphereRegionRange(region, valNode, withCentreTilt);
            fillSphereRegionInterpolate(region, valNode);
        }

        template <typename AugmentedSample>
        std::vector<Data> convertTimedMetaToData(
            const std::vector<AugmentedSample> aTimed, CodedFormat aCodedFormat,
            std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry,
            Optional<StreamSegmenter::Segmenter::Duration> aForceFrameInterval)
        {
            std::vector<Data> datas;

            Index presIndex{};
            CodingIndex codingIndex{};

            for (auto& sample : performSampleInterpolation(aTimed, aForceFrameInterval))
            {
                auto bytes = sample.sample.toBytes();
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
    }  // namespace

    OverlayDashInfo OverlayDashInfo::addSuffix(std::string aSuffix) const
    {
        OverlayDashInfo d(*this);
        if (d.suffix.size())
        {
            d.suffix += ".";
        }
        d.suffix += aSuffix;
        return d;
    }

    OverlayDashInfo OverlayDashInfo::withMetadataAssocationIds(
        std::list<RepresentationId> aMetadataAssocationIds) const
    {
        OverlayDashInfo d(*this);
        d.metadataAssocationIds = aMetadataAssocationIds;
        return d;
    }

    OverlayDashInfo OverlayDashInfo::withBackgroundAssocationIds(
        std::list<StreamId> aBackgroundAssocationIds) const
    {
        OverlayDashInfo d(*this);
        d.backgroundAssocationIds = aBackgroundAssocationIds;
        return d;
    }

    namespace OverlaysInternal  // not anonymous due to template usage with optionWithDefault
    {
        auto readRecommendedViewportOverlay(const EntityGroupReadContext& /*aEntityGroupReadContext*/)
            -> std::function<ISOBMFF::RecommendedViewportOverlay(const ConfigValue& aValue)>
        {
            return [&](const ConfigValue& aValue) {
                ISOBMFF::RecommendedViewportOverlay r{};
                if (readToOverlayControlFlagBase(r, aValue, OverlayControlRead::NoReadInherited))
                {
                    return r;
                }
                return r;
            };
        }
    }  // namespace OverlaysInternal

    auto readOverlayDeclaration(const EntityGroupReadContext& aEntityGroupReadContext)
        -> std::function<std::shared_ptr<OverlayMedia>(const ConfigValue& aNode)>
    {
        return [&](const ConfigValue& aNode) {
            auto media = std::make_shared<OverlayMedia>();
            media->ovly.numFlagBytes = 2;
            media->refIdLabel = readOptional(readRefIdLabel)(aNode["ref_id"]);
            media->srcFile = readFilename(aNode["filename"]);
            media->srcType = readVideoInputMode(aNode["src_type"]);

            // handler function for reading single overlay configuration
            auto readSingleOverlay = [&aEntityGroupReadContext](const ConfigValue& aNode) {
                ISOBMFF::SingleOverlayStruct singleOverlay;

                // must be unique
                singleOverlay.overlayId = readInt(aNode["id"]);

                //
                // Helpers to read all the different control structs from JSON -> SingleOverlay.
                //
                // Maybe I have written too much javascript, because I like to scope
                // these funcitons to exist only here where I should be to only place
                // they are needed.
                //

                auto readViewportRelativeControl = [](ISOBMFF::SingleOverlayStruct& dst,
                                                      const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["viewport_relative"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.viewportRelativeOverlay, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        // always use relative disparity for now
                        dst.viewportRelativeOverlay.relativeDisparityFlag = true;

                        dst.viewportRelativeOverlay.rectTopPercent =
                            readFloatPercentAsFP<uint16_t>(srcNode["rect_top_percent"]);
                        dst.viewportRelativeOverlay.rectLeftPercent =
                            readFloatPercentAsFP<uint16_t>(srcNode["rect_left_percent"]);
                        dst.viewportRelativeOverlay.rectWidthtPercent =
                            readFloatPercentAsFP<uint16_t>(srcNode["rect_width_percent"]);
                        dst.viewportRelativeOverlay.rectHeightPercent =
                            readFloatPercentAsFP<uint16_t>(srcNode["rect_height_percent"]);
                        dst.viewportRelativeOverlay.disparity =
                            readFloatPercentAsFP<int16_t>(srcNode["disparity_percent"]);
                        dst.viewportRelativeOverlay.mediaAlignment =
                            readOptional(readMediaAlignmentType)(srcNode["media_alignment"])
                                .value_or(ISOBMFF::MediaAlignmentType::STRETCH_TO_FILL);
                        dst.viewportRelativeOverlay.relativeDisparityFlag =
                            readOptional(readBool)(srcNode["relative_disparity_flag"])
                                .value_or(false);
                    }
                };

                auto readSphereRelative2d = [](ISOBMFF::SingleOverlayStruct& dst,
                                               const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["sphere_relative_2d"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.sphereRelative2DOverlay, srcNode,
                                                     OverlayControlRead::NoReadInherited);

                        // read sphere region without centre_tilt which is always 0 for 2d
                        // overlay
                        readSphereRegion(dst.sphereRelative2DOverlay.sphereRegion, srcNode, false);
                        readRotation(dst.sphereRelative2DOverlay.overlayRotation, srcNode);

                        // indicates the depth (z-value) of the region on which the overlay is
                        // to be rendered. The depth value is the norm of the normal vector of
                        // the overlay region. region_depth_minus1 + 1 specifies the depth value
                        // relative to a unit sphere in units of 2^-16
                        dst.sphereRelative2DOverlay.regionDepthMinus1 =
                            readFloatPercentAsFP<uint16_t>(srcNode["region_depth_percent"]);

                        dst.sphereRelative2DOverlay.timelineChangeFlag =
                            readOptional(readBool)(srcNode["timeline_change_flag"]).value_or(false);
                    }
                };

                auto readSphereRelativeOmni = [](ISOBMFF::SingleOverlayStruct& dst,
                                                 const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["sphere_relative_omni"];
                    if (srcNode)
                    {
                        dst.sphereRelativeOmniOverlay = {};
                        readToOverlayControlFlagBase(dst.sphereRelativeOmniOverlay, srcNode,
                                                     OverlayControlRead::NoReadInherited);

                        if (readOptional(readBool)(srcNode["region_indication_sphere_region"])
                                .value_or(true))
                        {
                            ISOBMFF::SphereRegionStatic</* HasRange */ true,
                                                        /* HasInterpolate */ true>& sphereRegion =
                                dst.sphereRelativeOmniOverlay.region
                                    .set<ISOBMFF::RegionIndicationType::SPHERE>({});

                            readSphereRegion(sphereRegion, srcNode, true);
                        }
                        else
                        {
                            ISOBMFF::ProjectedPictureRegion& projectedPictureRegion =
                                dst.sphereRelativeOmniOverlay.region
                                    .set<ISOBMFF::RegionIndicationType::PROJECTED_PICTURE>({});
                            projectedPictureRegion = readISOBMFFProjectedPictureRegion(srcNode);
                        }
                        dst.sphereRelativeOmniOverlay.timelineChangeFlag =
                            readOptional(readBool)(srcNode["timeline_change_flag"]).value_or(false);

                        dst.sphereRelativeOmniOverlay.regionDepthMinus1 =
                            readFloatPercentAsFP<uint16_t>(srcNode["region_depth_percent"]);
                    }
                };

                auto readOpacity = [](ISOBMFF::SingleOverlayStruct& dst,
                                      const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_opacity"];
                    // support both "overlay_opacity": {"opacity_percent": "xx"} and
                    // "opacity_percent": "xx"
                    if (!srcNode)
                    {
                        srcNode = ovlyConfigNode;
                    }

                    auto percentNode = srcNode["opacity_percent"];
                    if (percentNode)
                    {
                        readToOverlayControlFlagBase(dst.overlayOpacity, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        dst.overlayOpacity.opacity = readUint8Percent(percentNode);
                    }
                };

                auto readOverlayLayeringOrder = [](ISOBMFF::SingleOverlayStruct& dst,
                                                   const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_layering_order"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.overlayLayeringOrder, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        dst.overlayLayeringOrder.layeringOrder = readInt16(srcNode["layering_order"]);
                    }
                };

                auto readOverlaySourceRegion = [](ISOBMFF::SingleOverlayStruct& dst,
                                                   const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_source_region"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.overlaySourceRegion, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        dst.overlaySourceRegion.region = readPackedPictureRegion(srcNode);
                        dst.overlaySourceRegion.transformType = readTransformType(srcNode["transform_type"]);
                    }
                };

                auto readOverlayPriority = [](ISOBMFF::SingleOverlayStruct& dst,
                                                   const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_priority"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.overlayPriority, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        dst.overlayPriority.overlayPriority = readUint8(srcNode["overlay_priority"]);
                    }
                };

                auto readOverlayInteraction = [](ISOBMFF::SingleOverlayStruct& dst,
                                                   const ConfigValue& aValue) {
                    auto srcNode = aValue["overlay_interaction"];
                    if (srcNode)
                    {
                        readToOverlayControlFlagBase(dst.overlayInteraction, srcNode,
                                                     OverlayControlRead::NoReadInherited);
                        auto& r = dst.overlayInteraction;
                        r.changePositionFlag = readBool(srcNode["change_position_flag"]);
                        r.changeDepthFlag = readBool(srcNode["change_depth_flag"]);
                        r.switchOnOffFlag = readBool(srcNode["switch_on_off_flag"]);
                        r.changeOpacityFlag = readBool(srcNode["change_opacity_flag"]);
                        r.resizeFlag = readBool(srcNode["resize_flag"]);
                        r.rotationFlag = readBool(srcNode["rotation_flag"]);
                        r.sourceSwitchingFlag = readBool(srcNode["source_switching_flag"]);
                        r.cropFlag = readBool(srcNode["crop_flag"]);
                    }
                };

                auto readOverlayAlphaCompositing = [](ISOBMFF::SingleOverlayStruct& dst,
                                                      const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_alpha_compositing"];
                    if (srcNode)
                    {
                        dst.overlayAlphaCompositing.doesExist = true;
                        dst.overlayAlphaCompositing.alphaBlendingMode =
                            readAlphaBlendingModeType(srcNode);
                    }
                };

                auto readLabel = [](ISOBMFF::SingleOverlayStruct& dst,
                                    const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["overlay_label"];
                    // support both "overlay_label": {"label": "xx"} and "label": "xx"
                    if (!srcNode)
                    {
                        srcNode = ovlyConfigNode;
                    }
                    auto labelNode = srcNode["label"];
                    if (labelNode)
                    {
                        dst.overlayLabel.doesExist = true;
                        dst.overlayLabel.overlayLabel = readString(labelNode);
                    }
                };

                auto readAssociatedSphereRegion = [](ISOBMFF::SingleOverlayStruct& dst,
                                                     const ConfigValue& ovlyConfigNode) {
                    auto srcNode = ovlyConfigNode["associated_sphere_region"];
                    if (srcNode)
                    {
                        dst.associatedSphereRegion = readISOBMFFAssociatedSphereRegion(srcNode);
                    }
                };

                readViewportRelativeControl(singleOverlay, aNode);
                readSphereRelative2d(singleOverlay, aNode);
                readSphereRelativeOmni(singleOverlay, aNode);
                readOpacity(singleOverlay, aNode);
                readOverlayLayeringOrder(singleOverlay, aNode);
                readOverlaySourceRegion(singleOverlay, aNode);
                readOverlayPriority(singleOverlay, aNode);
                readOverlayInteraction(singleOverlay, aNode);
                readOverlayAlphaCompositing(singleOverlay, aNode);
                readLabel(singleOverlay, aNode);
                readAssociatedSphereRegion(singleOverlay, aNode);
                singleOverlay.recommendedViewportOverlay = optionWithDefault(
                    aNode, "recommended_viewport_overlay",
                    OverlaysInternal::readRecommendedViewportOverlay(aEntityGroupReadContext), {});

                return singleOverlay;
            };

            // read media track's overlay configuration and push them to media struct for later
            // use
            for (const auto& singleOvly :
                 readList("Base overlays config", readSingleOverlay)(aNode["config"]))
            {
                media->ovly.overlays.push_back(singleOvly);
            }

            if (aNode["timed_ovly"])
            {
                media->timedMetaOvly = readVector(
                    "timed metadata (ovly)",
                    readInterpolatedSample<MP4VR::OverlayConfigSample>([](const ConfigValue& aValue) {
                        MP4VR::OverlayConfigSample sample{};
                        std::vector<uint16_t> activeOverlayIds;
                        for (auto x : readList("single overlay property",
                                               readSingleOverlayProperty)(aValue["config"]))
                        {
                            if (!anySingleOverlayPropertySet(x))
                            {
                                activeOverlayIds.push_back(x.overlayId);
                            }
                        }

                        sample.activeOverlayIds = makeDynArray(activeOverlayIds);

                        sample.addlActiveOverlays = readOverlayConfigProperty(aValue);
                        return sample;
                    }))(aNode["timed_ovly"]);
                validateTimedMetadata(media->timedMetaOvly);
                media->timedMetaOvly = media->timedMetaOvly;
            }

            if (aNode["timed_invo"])
            {
                media->timedMetaInvo =
                    readVector("timed metadata (invo)",
                               readInterpolatedSample<MP4VR::InitialViewingOrientationSample>(
                                   readInitialViewingOrientationSample2))(aNode["timed_invo"]);
                validateTimedMetadata(media->timedMetaInvo);
                media->timedMetaInvo = media->timedMetaInvo;
            }

            return media;
        };
    }

    template <typename AugmentedSample>
    OverlayMetaTrackInfo createOverlayTimedMetadataTrack(
        ControllerConfigure& aConfigure, ControllerOps& aOps,
        const std::vector<AugmentedSample>& aSamples, const OverlayVideoTrackInfo& aVideoTrackInfo,
        CodedFormat aCodedFormat,
        std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry,
        MP4VRWriter* aMP4VRWriter, Optional<OverlayDashInfo> aDashInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        OverlayMetaTrackInfo metaInfo;
        PipelineOutput output{PipelineOutputTimedMetadata::TimedMetadata};

        SequenceSource::Config sequenceConfig{};

        Optional<StreamSegmenter::Segmenter::Duration> forceFrameInterval;
        if (aDashInfo)
        {
            SegmentDurations segmentDurations = aDashInfo->dashDurations;
            forceFrameInterval =
                segmentDurations.segmentDuration /
                StreamSegmenter::Segmenter::Duration(segmentDurations.subsegmentsPerSegment, 1);
        }

        sequenceConfig.samples =
            convertTimedMetaToData(aSamples, aCodedFormat, aSampleEntry, forceFrameInterval);

        if (sequenceConfig.samples.size())
        {
            FrameDuration timescale = timescaleForSamples(
                sequenceConfig.samples,
                static_cast<FrameTime::value>(aVideoTrackInfo.gopInfo.duration.minimize().den));

            std::map<std::string, std::set<TrackId>> trefs;
            if (aVideoTrackInfo.overlayMediaTrackIds.size())
            {
                trefs["cdsc"] = {aVideoTrackInfo.overlayMediaTrackIds.begin(),
                                 aVideoTrackInfo.overlayMediaTrackIds.end()};
            }

            MP4VRWriter::Sink mp4vrSink =
                aMP4VRWriter ? aMP4VRWriter->createSink({}, output, timescale, trefs)
                             : MP4VRWriter::Sink{};

            if (aMP4VRWriter)
            {
                metaInfo.mp4vrSink = mp4vrSink;
            }

            AsyncNode* source = aOps.makeForGraph<SequenceSource>("TMD for OMAF", sequenceConfig);

            if (aDashInfo && aDashInfo->dash)
            {
                DashOmaf& dash = *aDashInfo->dash;
                auto dashConfig = dash.dashConfigFor("media");
                auto segmentName = dash.getBaseName() + "." + aDashInfo->suffix;

                DashSegmenterMetaConfig dashMetaConfig{};
                dashMetaConfig.dashDurations = aDashInfo->dashDurations;
                dashMetaConfig.timeScale = timescale;
                dashMetaConfig.trackId = aConfigure.newTrackId();

                TagTrackIdProcessor::Config trackTaggerConfig{dashMetaConfig.trackId};
                auto trackTagger = aOps.makeForGraph<TagTrackIdProcessor>(
                    "track id:=" + std::to_string(dashMetaConfig.trackId.get()), trackTaggerConfig);

                connect(*source, *trackTagger);
                source = trackTagger;

                auto dashSegmenterConfig = dash.makeDashSegmenterConfig(
                    dashConfig, PipelineOutputTimedMetadata::TimedMetadata, segmentName,
                    dashMetaConfig);

                aConfigure.buildPipeline(dashSegmenterConfig, output, {{output, {source, allStreams}}},
                                         /*MP4VRSink*: */ nullptr, aPipelineBuildInfo);

                AdaptationConfig adaptationConfig{};
                adaptationConfig.adaptationSetId = aConfigure.newAdaptationSetId();
                RepresentationConfig representationConfig{};
                representationConfig.output = dashSegmenterConfig;
                if (aDashInfo->metadataAssocationIds.size())
                {
                    representationConfig.associationType = {"cdsc"};
                    representationConfig.associationId = aDashInfo->metadataAssocationIds;
                }
                Representation representation{};
                representation.representationConfig = representationConfig;
                representation.encodedFrameSource = source;
                adaptationConfig.representations = {representation};

                DashMPDConfig dashMPDConfig{};
                dashMPDConfig.segmenterOutput = output;
                dashMPDConfig.adaptationConfig = adaptationConfig;

                dash.configureMPD(dashMPDConfig);
            }

            if (aMP4VRWriter)
            {
                auto trackId = *mp4vrSink.trackIds.begin();
                TagTrackIdProcessor::Config trackTaggerConfig{trackId};
                auto trackTagger = aOps.makeForGraph<TagTrackIdProcessor>(
                    "track id:=" + std::to_string(trackId.get()), trackTaggerConfig);

                connect(*source, *trackTagger);
                source = trackTagger;

                aConfigure.buildPipeline({}, output, {{output, {source, allStreams}}}, mp4vrSink.sink,
                                         aPipelineBuildInfo);
            }
        }

        return metaInfo;
    }

    template OverlayMetaTrackInfo createOverlayTimedMetadataTrack(
        ControllerConfigure& aConfigure, ControllerOps& aOps,
        const std::vector<OverlaySample>& aSamples,
        const OverlayVideoTrackInfo& aOverlayVideoTrackInfo, CodedFormat aCodedFormat,
        std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry,
        MP4VRWriter* aMP4VRWriter, Optional<OverlayDashInfo> aDashInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo);

    template OverlayMetaTrackInfo createOverlayTimedMetadataTrack(
        ControllerConfigure& aConfigure, ControllerOps& aOps,
        const std::vector<MP4VRInitialViewingOrientationSample>& aSamples,
        const OverlayVideoTrackInfo& aOverlayVideoTrackInfo, CodedFormat aCodedFormat,
        std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry,
        MP4VRWriter* aMP4VRWriter, Optional<OverlayDashInfo> aDashInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo);

    OverlayVideoTrackInfo createOverlayMediaTracks(ControllerConfigure& aConfigure,
                                                   ControllerOps& aOps, const OverlayMedia& aMedia,
                                                   const MP4VROutputs* aMP4VROutputs,
                                                   Optional<OverlayDashInfo> aDashInfo,
                                                   Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        OverlayVideoTrackInfo info;
        VideoInput overlayVideoInput;
        ControllerConfigureCapture ovlyConf(aConfigure);
        VDD::Config captureConfig;
        captureConfig.setKeyValue("filename", aMedia.srcFile);
        captureConfig.setKeyValue("formats", "H265 AAC");
        captureConfig.setKeyValue("modes", "topbottom,sidebyside,mono,nonvr");

        ParsedValue<MediaInputConfig> mediaInputConfig(captureConfig.root(), readMediaInputConfig);
        auto media = mediaInputConfig->makeMediaLoader(false);

        // make audio first, so we get to find its track id to track reference from video
        if (media->getTracksByFormat(std::set<CodedFormat>{CodedFormat::AAC}).size())
        {
            if (aMP4VROutputs && aMP4VROutputs->size())
            {
                Optional<AudioMP4Info> mp4Info = makeAudioMp4(
                    aOps, aConfigure, *aMP4VROutputs, captureConfig.root(), AudioIsOverlay{true});

                if (!mp4Info)
                {
                    throw ConfigValueReadError("Failed to initialize audio overlay from " +
                                               aMedia.srcFile);
                }

                for (auto [output, outputInfo] : mp4Info->output)
                {
                    for (auto trackId : outputInfo.trackIds)
                    {
                        info.overlayMediaTrackIds.insert(trackId);
                    }
                }
            }
            else if (aDashInfo && aDashInfo->dash)
            {
                Optional<AudioDashInfo> audioDashInfo =
                    makeAudioDash(aOps, aConfigure, *aDashInfo->dash, captureConfig.root(),
                                  aPipelineBuildInfo);
                if (!audioDashInfo)
                {
                    throw ConfigValueReadError("Failed to initialize audio overlay from " +
                                               aMedia.srcFile);
                }
                info.overlayMediaAssociationIds.push_back(audioDashInfo->representationId);
            }
        }
        if (media->getTracksByFormat(std::set<CodedFormat>{CodedFormat::H265}).size())
        {
            Optional<ParsedValue<RefIdLabel>> dummyOverlayRefIdLabel;

            // video path
            if (overlayVideoInput.setupMP4VideoInput(aOps, ovlyConf, captureConfig.root(),
                                                     VideoProcessingMode::Passthrough,
                                                     dummyOverlayRefIdLabel))
            {
                auto videoInputMode = aMedia.srcType ? *aMedia.srcType : ovlyConf.mInputVideoMode;

                PipelineOutput output =
                    pipelineOutputOfVideoInputMode(videoInputMode).withVideoOverlaySet();
                Optional<OmafProjectionType> projectionOverride;

                switch (videoInputMode)
                {
                case VideoInputMode::NonVR:
                {
                    // we want to reset projection to None to indicate 2d material;
                    // projectionOverride is None
                    break;
                }
                case VideoInputMode::Mono:
                case VideoInputMode::LeftRight:
                case VideoInputMode::TopBottom:
                {
                    projectionOverride = OmafProjectionType::Equirectangular;
                    break;
                }
                case VideoInputMode::RightLeft:
                case VideoInputMode::BottomTop:
                case VideoInputMode::Separate:
                case VideoInputMode::TemporalInterleaving:
                {
                    throw UnsupportedVideoInput("Video not supported as OMAF layer");
                }
                }

                auto videoConfig =
                    makeOptional(VRVideoConfig({{}, OutputMode::OMAFV1, false, {}, aMedia.ovly}));

                auto metaModifyCallback = [projectionOverride](const std::vector<Meta>& aMetas) {
                    std::vector<Meta> metas = aMetas;
                    if (metas[0].getContentType() == ContentType::CODED)
                    {
                        auto coded = metas[0].getCodedFrameMeta();
                        coded.projection = projectionOverride;
                        metas[0] = coded;
                    }
                    return metas;
                };

                MetaModifyProcessor::Config metaModifyConfig{};
                metaModifyConfig.metaModifyCallback = metaModifyCallback;
                AsyncNode* inputLeft = ovlyConf.mInputLeft;

                if (aDashInfo && aDashInfo->dash)
                {
                    auto modifyProjection = aOps.makeForGraph<MetaModifyProcessor>(
                        "Modify projection dash", metaModifyConfig);
                    DashOmaf& dash = *aDashInfo->dash;
                    auto dashConfig = dash.dashConfigFor("media");
                    auto segmentName = dash.getBaseName() + "." + aDashInfo->suffix;

                    DashSegmenterMetaConfig dashMetaConfig{};
                    dashMetaConfig.viewId = aConfigure.getViewId();
                    dashMetaConfig.viewLabel = aConfigure.getViewLabel();
                    dashMetaConfig.dashDurations = aDashInfo->dashDurations;
                    dashMetaConfig.timeScale = ovlyConf.mVideoTimeScale;
                    dashMetaConfig.trackId = aConfigure.newTrackId();
                    dashMetaConfig.overlays = aMedia.ovly;

                    auto dashSegmenterConfig = dash.makeDashSegmenterConfig(
                        dashConfig, PipelineOutput(PipelineOutputVideo::Mono).withVideoOverlaySet(),
                        segmentName, dashMetaConfig);

                    TagTrackIdProcessor::Config trackTaggerConfig{dashMetaConfig.trackId};
                    auto trackTagger = aOps.makeForGraph<TagTrackIdProcessor>(
                        "track id:=" + std::to_string(dashMetaConfig.trackId.get()),
                        trackTaggerConfig);

                    connect(*inputLeft, *modifyProjection);
                    connect(*modifyProjection, *trackTagger);

                    auto source = trackTagger;

                    aConfigure.buildPipeline(dashSegmenterConfig, output,
                                             {{output, {source, allStreams}}},
                                             /*MP4VRSink*: */ nullptr, aPipelineBuildInfo);

                    AdaptationConfig adaptationConfig{};
                    adaptationConfig.adaptationSetId = dash.newId();
                    info.overlayMediaAdaptationSetIds.push_back(adaptationConfig.adaptationSetId);
                    adaptationConfig.overlayBackground = aDashInfo->backgroundAssocationIds;
                    Representation representation{
                        source, RepresentationConfig{{}, dashSegmenterConfig, {}, {}, {}, {}}};
                    // if there is audio, created above
                    if (info.overlayMediaAssociationIds.size())
                    {
                        representation.representationConfig.associationType = {
                            "soun"}; 
                        representation.representationConfig.associationId =
                            info.overlayMediaAssociationIds;
                    }
                    if (aMedia.refIdLabel)
                    {
                        EntityIdReference refId {};
                        refId.label = *aMedia.refIdLabel;
                        SingleEntityIdReference reference;
                        reference.adaptationSetId = adaptationConfig.adaptationSetId;
                        reference.representationId = makePromise(dashSegmenterConfig.representationId);
                        reference.trackId = dashMetaConfig.trackId;
                        reference.entityId = dashMetaConfig.trackId.get();
                        refId.references.push_back(reference);
                        info.labelEntityIdMapping.addLabel(refId);
                    }

                    adaptationConfig.representations = {representation};
                    info.overlayMediaAssociationIds.push_back(dashSegmenterConfig.representationId);

                    DashMPDConfig dashMPDConfig{};
                    dashMPDConfig.segmenterOutput = output;
                    dashMPDConfig.adaptationConfig = adaptationConfig;
                    dashMPDConfig.overlays = aMedia.ovly;

                    dash.configureMPD(dashMPDConfig);
                }

                if (aMP4VROutputs && aMP4VROutputs->size())
                {
                    auto modifyProjection = aOps.makeForGraph<MetaModifyProcessor>(
                        "Modify projection mp4vr", metaModifyConfig);
                    for (auto& labeledMP4VRWriter : *aMP4VROutputs)
                    {
                        auto& outputWriter = *labeledMP4VRWriter.second;

                        std::map<std::string, std::set<TrackId>> trackReferences;
                        if (info.overlayMediaTrackIds.size())
                        {
                            trackReferences["soun"] = {info.overlayMediaTrackIds.begin(),
                                                       info.overlayMediaTrackIds.end()};
                        }

                        auto mp4vrSink =
                            outputWriter.createSink(videoConfig, output, ovlyConf.mVideoTimeScale,
                                                    trackReferences);

                        if (aMedia.refIdLabel && mp4vrSink.trackIds.size())
                        {
                            EntityIdReference refId{};
                            assert(mp4vrSink.trackIds.size() == 1);
                            SingleEntityIdReference reference;
                            reference.trackId = *mp4vrSink.trackIds.begin();
                            reference.entityId = {reference.trackId.get()};
                            refId.references.push_back(reference);
                            refId.label = *aMedia.refIdLabel;
                            // refId.representationId = not set
                            info.labelEntityIdMapping.addLabel(refId);
                        }

                        for (auto trackId : mp4vrSink.trackIds)
                        {
                            info.overlayMediaTrackIds.insert(trackId);
                        }

                        TagTrackIdProcessor::Config trackTaggerConfig{*mp4vrSink.trackIds.begin()};
                        auto trackTagger = aOps.makeForGraph<TagTrackIdProcessor>(
                            "track id:=" + std::to_string(mp4vrSink.trackIds.begin()->get()),
                            trackTaggerConfig);

                        connect(*inputLeft, *modifyProjection);
                        connect(*modifyProjection, *trackTagger);

                        auto outputStream = trackTagger;

                        aConfigure.buildPipeline({}, output, {{output, {outputStream, allStreams}}},
                                                 mp4vrSink.sink, aPipelineBuildInfo);
                    }
                }
            }
        }
        if (info.overlayMediaAssociationIds.size() == 0 && info.overlayMediaTrackIds.size() == 0)
        {
            throw ConfigValueReadError(
                "Overlay source file does not contain H265 video or AAC audio");
        }
        info.gopInfo = ovlyConf.mGopInfo;
        return info;
    }

    OutputOverlayMultiMetaTrackInfo createTimedOverlayMetadataTracks(
        ControllerConfigure& aConfigure, ControllerOps& aOps, const OverlayMedia& aMedia,
        const MP4VROutputs* aMP4VROutputs, Optional<OverlayDashInfo> aDashInfo,
        const OverlayVideoTrackInfo& aVideoTrackInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo)
    {
        OutputOverlayMultiMetaTrackInfo outputMetaTrackInfo;

        // sample entries cannot be copied, so we contain them in a shared pointer
        auto sampleEntry = std::make_shared<StreamSegmenter::Segmenter::OverlaySampleEntry>();
        sampleEntry->overlayStruct.numFlagBytes = 2;
        sampleEntry->overlayStruct.overlays = aMedia.ovly.overlays;

        auto overlayIdStr = std::to_string(aMedia.ovly.overlays.at(0u).overlayId);

        if (aDashInfo && aDashInfo->dash)
        {
            createOverlayTimedMetadataTrack(aConfigure, aOps, aMedia.timedMetaOvly, aVideoTrackInfo,
                                            CodedFormat::TimedMetadataDyol, sampleEntry, nullptr,
                                            aDashInfo->addSuffix("ovly." + overlayIdStr), aPipelineBuildInfo);

            if (aMedia.timedMetaInvo.size())
            {
                createOverlayTimedMetadataTrack(aConfigure, aOps, aMedia.timedMetaInvo,
                                                aVideoTrackInfo, CodedFormat::TimedMetadataInvo,
                                                sampleEntry, nullptr,
                                                aDashInfo->addSuffix("invo." + overlayIdStr), aPipelineBuildInfo);
            }
        }

        if (aMP4VROutputs)
        {
            for (auto& labeledMP4VRWriter : *aMP4VROutputs)
            {
                outputMetaTrackInfo[labeledMP4VRWriter.first].dyol =
                    createOverlayTimedMetadataTrack(aConfigure, aOps, aMedia.timedMetaOvly,
                                                    aVideoTrackInfo, CodedFormat::TimedMetadataDyol,
                                                    sampleEntry, labeledMP4VRWriter.second.get(),
                                                    {}, aPipelineBuildInfo);

                if (aMedia.timedMetaInvo.size())
                {
                    outputMetaTrackInfo[labeledMP4VRWriter.first].invo =
                        createOverlayTimedMetadataTrack(
                            aConfigure, aOps, aMedia.timedMetaInvo, aVideoTrackInfo,
                            CodedFormat::TimedMetadataInvo, sampleEntry,
                            labeledMP4VRWriter.second.get(), {}, aPipelineBuildInfo);
                }
            }
        }

        return outputMetaTrackInfo;
    }
}  // namespace VDD
