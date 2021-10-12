
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

#include <memory>
#include <functional>

#include "common.h"
#include "common/optional.h"
#include "dash.h"
#include "mp4vrwriter.h"
#include "videoinput.h"
#include "reader/mp4vrfiledatatypes.h"
#include "mp4vromafreaders.h"
#include "mp4vromafinterpolate.h"

namespace VDD
{
    using OverlaySample = InterpolatedSample<MP4VR::OverlayConfigSample>;

    // Named thus to not collide with the InitialViewingOrientationSample in VDD
    using MP4VRInitialViewingOrientationSample =
        InterpolatedSample<MP4VR::InitialViewingOrientationSample>;

    struct OverlayMedia
    {
        std::string srcFile;
        Optional<ParsedValue<RefIdLabel>> refIdLabel;
        Optional<VideoInputMode> srcType;
        ISOBMFF::OverlayStruct ovly;
        std::vector<OverlaySample> timedMetaOvly;
        std::vector<MP4VRInitialViewingOrientationSample> timedMetaInvo;
    };

    struct OverlayVideoTrackInfo
    {
        std::set<TrackId> overlayMediaTrackIds;
        VideoGOP gopInfo;
        std::list<RepresentationId> overlayMediaAssociationIds;
        std::list<StreamId> overlayMediaAdaptationSetIds;
        LabelEntityIdMapping labelEntityIdMapping;
    };

    struct OverlayMetaTrackInfo
    {
        Optional<MP4VRWriter::Sink> mp4vrSink;
    };

    struct OverlayMultiMetaTrackInfo
    {
        OverlayMetaTrackInfo dyol;
        Optional<OverlayMetaTrackInfo> invo;
    };

    using PipelineModeName = std::pair<PipelineMode, std::string>;

    template <typename T> using PipelineModeNameMap = std::map<PipelineModeName, T>;

    using OutputOverlayMultiMetaTrackInfo = PipelineModeNameMap<OverlayMultiMetaTrackInfo>;

    auto readOverlayDeclaration(const EntityGroupReadContext& aEntityGroupReadContext)
        -> std::function<std::shared_ptr<OverlayMedia>(const ConfigValue& aNode)>;

    class MP4VRWriter;
    class DashOmaf;

    struct OverlayDashInfo
    {
        DashOmaf* dash;
        SegmentDurations dashDurations;
        std::string suffix;
        std::list<RepresentationId> metadataAssocationIds; // cdsc trefs from metadata to these association ids
        std::list<StreamId> backgroundAssocationIds; // ovbg associations from overlay video to background video
        AllocateAdaptationSetId allocateAdaptationsetId; // generate a fresh adaptation set id

        OverlayDashInfo addSuffix(std::string aSuffix) const;
        OverlayDashInfo withMetadataAssocationIds(std::list<RepresentationId> aMetadataAssocationIds) const;
        OverlayDashInfo withBackgroundAssocationIds(std::list<StreamId> aBackgroundAssocationIds) const;
    };

    /** @brief Create a timed metadata track for overlays
     *
     * @param aMP4VRWriter Optional, can be null if no MP4VRWriter is to be used
     * @note Applicable for OverlaySample and MP4VRInitialViewingOrientationSample
     */
    template <typename SampleTimeDuration>
    OverlayMetaTrackInfo createOverlayTimedMetadataTrack(
        ControllerConfigure& aConfigure, ControllerOps& aOps,
        const std::vector<SampleTimeDuration>& aSamples,
        const OverlayVideoTrackInfo& aOverlayVideoTrackInfo, CodedFormat aCodedFormat,
        std::shared_ptr<StreamSegmenter::Segmenter::SampleEntry> aSampleEntry,
        MP4VRWriter* aMP4VRWriter, Optional<OverlayDashInfo> aDashInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo);

    /** */
    OverlayVideoTrackInfo createOverlayMediaTracks(ControllerConfigure& aConfigure,
                                                   ControllerOps& aOps, const OverlayMedia& aMedia,
                                                   const MP4VROutputs* aMP4VROutputs,
                                                   Optional<OverlayDashInfo> aDashInfo,
                                                   Optional<PipelineBuildInfo> aPipelineBuildInfo);

    OutputOverlayMultiMetaTrackInfo createTimedOverlayMetadataTracks(
        ControllerConfigure& aConfigure, ControllerOps& aOps, const OverlayMedia& aMedia,
        const MP4VROutputs* aMP4VROutputs, Optional<OverlayDashInfo> aDashInfo,
        const OverlayVideoTrackInfo& aVideoTrackInfo,
        Optional<PipelineBuildInfo> aPipelineBuildInfo);
}  // namespace VDD