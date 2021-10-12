
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
#pragma once

#include <list>
#include <string>

#include "controller/dash.h"
#include "omaf/omafviewingorientation.h"
#include "controller/mp4vromafinterpolate.h"
#include "controller/mp4vromafreaders.h"
#include "controller/mp4vrwriter.h"

#include <reader/mp4vrfiledatatypes.h>

namespace VDD
{
    class AsyncNode;
    class AsyncProcessor;
    class ConfigValue;
    class ControllerOps;
    class ControllerConfigure;
    class DashOmaf;
    class MP4VRWriter;

    struct TimedMetadataTrackInfo
    {
        MP4VRWriter::Sink mp4vrSink;
    };

    /** Make invo (Initial Viewing Orientation) input source that repeats sample aSample every
     * aSampleDuration, given the input has produced as many samples. Typically aSampleDuration
     * would be the duration of a DASH segment.
     */
    AsyncProcessor* makeInvoInput(ControllerOps& aOps, FrameDuration aSampleDuration,
                                  InitialViewingOrientationSample aSample);

    /** Make overlay metadata input source input source that repeats over aSamples entries
     */
    AsyncProcessor* makeOverlayInput(ControllerOps& aOps, FrameDuration aSampleDuration,
                                     std::vector<MP4VR::OverlayConfigSample>& aSamples);

    /** @brief Read the initial viewing orientation sample from the configuration */
    InitialViewingOrientationSample readInitialViewingOrientationSample(const ConfigValue& aNode);

    /** @brief Connects the given frame source to the timed metadata generator and then construct
     * the following DASH segmenter and MPD generation pipeline by calling aConfigure.buildPipeline
     * and aDash.configureMPD
     *
     * Assigns the `cdtg` association to the viewpoint id.
     *
     * @param aVideoRepresentationIds is used to build cdsc association to the video tracks
     *
     * Call only if DASH is enabled.
     */
    struct HookDashInfo
    {
        Future<RepresentationId> representationId;
    };

    HookDashInfo hookDashTimedMetadata(AsyncNode* aTMDSource, DashOmaf& aDash, StreamId aStreamId,
                                       ControllerConfigure& aConfigure,
                                       ControllerOps& aOps,
                                       Optional<PipelineBuildInfo> aPipelineBuildInfo,
                                       Optional<FrameDuration> aTimeScale = Optional<FrameDuration>(),
                                       TrackReferences aTrackReferences = TrackReferences{});

    TimedMetadataTrackInfo hookMP4TimedMetadata(AsyncNode* aTMDSource,
                                                ControllerConfigure& aConfigure,
                                                ControllerOps& aOps,
                                                std::list<TrackId> aVideoTrackIds,
                                                MP4VRWriter& aMP4VRWriter,
                                                Optional<PipelineBuildInfo> aPipelineBuildInfo,
                                                Optional<FrameDuration> aTimeScale = Optional<FrameDuration>(),
                                                TrackReferences aTrackReferences = TrackReferences{});

    enum class TimedMetadataType
    {
        Rcvp,
        Invp,
        Dyol,
        Dyvp
    };

    struct TimedMetadataDeclarationCommon
    {
        ParsedValue<RefIdLabel> refId;
        TrackReferences trackReferences;
    };

    struct NoConfigMapping {
        static const bool HasConfigMapping = false;
    };

    template <typename ConfigType, typename SampleType,
              bool UseToBytesArg,
              CodedFormat ACodedFormat,
              typename DataConfigMapping = NoConfigMapping>
    struct TimedMetadataDeclarationGeneric: TimedMetadataDeclarationCommon
    {
        using ConfigMapping = DataConfigMapping;
        static const bool UseToBytes = UseToBytesArg;
        static const CodedFormat codedFormat = ACodedFormat;
        ConfigType config;
        std::vector<SampleType> samples;
    };

    /** @brief Convert a metadata sample into a Data */
    template <typename ConfigType, typename SampleType>
    const Data dataOfTimedMetadataSample(const ConfigType& aConfig, Index aIndex,
                                         const SampleType& aSample);

    using RcvpConfig = StreamSegmenter::Segmenter::RecommendedViewportSampleEntry;
    using RcvpSample = InterpolatedSample<ISOBMFF::SphereRegionSample>;
    struct RcvpConfigMapping {
        static const bool HasConfigMapping = true;
        // function used to get the part to pass to dataOfTimedMetadataSample (which uses
        // ISOBMFF::isobmffToBytes which requires proper config type)
        ISOBMFF::SphereRegionContext operator()(const RcvpConfig& aConfig) const
        {
            return aConfig.sphereRegionConfig;
        };
    };
    using TimedMetadataDeclarationRcvp = TimedMetadataDeclarationGeneric<RcvpConfig, RcvpSample, false, CodedFormat::TimedMetadataRcvp, RcvpConfigMapping>;

    using InvpConfig = StreamSegmenter::Segmenter::InitialViewpointSampleEntry;
    using InvpSample = InterpolatedSample<ISOBMFF::InitialViewpointSample>;
    using TimedMetadataDeclarationInvp = TimedMetadataDeclarationGeneric<InvpConfig, InvpSample, false, CodedFormat::TimedMetadataInvp>;

    using DyolConfig = StreamSegmenter::Segmenter::OverlaySampleEntry;
    using DyolSample = InterpolatedSample<MP4VR::OverlayConfigSample>;
    using TimedMetadataDeclarationDyol = TimedMetadataDeclarationGeneric<DyolConfig, DyolSample, true, CodedFormat::TimedMetadataDyol>;

    using DyvpConfig = StreamSegmenter::Segmenter::DynamicViewpointSampleEntry;
    using DyvpSample = InterpolatedSample<ISOBMFF::DynamicViewpointSample>;
    struct DyvpConfigMapping {
        static const bool HasConfigMapping = true;
        ISOBMFF::DynamicViewpointSampleEntry operator()(const DyvpConfig& aConfig) const
        {
            return aConfig.dynamicViewpointSampleEntry;
        };
    };
    using TimedMetadataDeclarationDyvp = TimedMetadataDeclarationGeneric<DyvpConfig, DyvpSample, false, CodedFormat::TimedMetadataDyvp, DyvpConfigMapping>;

    // clang-format off
    using TimedMetadataSampleEntry =
        ISOBMFF::Union<TimedMetadataType
                       ,RcvpConfig
                       ,InvpConfig
                       ,DyolConfig
                       ,DyvpConfig
                       >;

    struct TimedMetadataConfig
    {
        TimedMetadataSampleEntry sampleEntry;
        TrackReferences trackReferences;
    };

    using TimedMetadataDeclaration =
        ISOBMFF::UnionCommonBase<TimedMetadataType                // key
                                 ,TimedMetadataDeclarationCommon  // base class
                                 ,TimedMetadataDeclarationRcvp
                                 ,TimedMetadataDeclarationInvp
                                 ,TimedMetadataDeclarationDyol
                                 ,TimedMetadataDeclarationDyvp
                                 >;
    // clang-format on

    TimedMetadataType readTimedMetadataType(const ConfigValue& aValue);

    TimedMetadataDeclarationRcvp readTimedMetadataDeclarationRcvp(const ConfigValue& aValue);

    TimedMetadataDeclaration readTimedMetadataDeclaration(const ConfigValue& aValue);

    struct TmdTrackContext
    {
    };

    std::vector<Data> createSamplesForTimedMetadataTrack(
        const TmdTrackContext& aSampleContext, const TimedMetadataDeclaration& aTimedMetadata);

    AsyncNode* createSourceForTimedMetadataTrack(const TmdTrackContext& aSampleContext,
                                                 ControllerOps& aOps,
                                                 const TimedMetadataDeclaration& aTimedMetadata);
}  // namespace VDD
