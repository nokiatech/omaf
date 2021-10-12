
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

#include "common/optional.h"
#include "dash.h"
#include "mp4vrwriter.h"
#include "videoinput.h"
#include "reader/mp4vrfiledatatypes.h"

namespace VDD
{
    using AugmentedDyvpSample = SampleTimeDuration<ISOBMFF::DynamicViewpointSample>;

    struct ViewpointMedia
    {
        ViewpointInfo viewpointInfo;
        Optional<std::vector<AugmentedDyvpSample>> samples;
        std::string label;
        bool isInitial = false;
    };

    ViewpointMedia readViewpointDeclaration(const ConfigValue& aNode);

    class MP4VRWriter;
    class DashOmaf;
    class View;

    struct ViewpointMediaTrackInfo
    {
        std::set<TrackId> mediaTrackIds;
        VideoGOP gopInfo;
        std::list<StreamId> mediaAdaptationSetIds;
        std::list<RepresentationId> mediaAssociationIds;
    };

    struct ViewpointDashInfo
    {
        DashOmaf* dash;
        SegmentDurations dashDurations;
        std::string suffix;
    };

    /** @brief Create a timed metadata track for viewpoints
     *
     * @param aMP4VRWriter Optional, can be null if no MP4VRWriter is to be used
     * @note Applicable for DynamicViewpointSamples
     */
    void createViewpointMetadataTrack(const View& aView,
                                      ControllerConfigure& aConfigure, ControllerOps& aOps,
                                      const ViewpointMedia& aMedia,
                                      MP4VROutputs* aMP4VROutputs,
                                      Optional<ViewpointDashInfo> aDashInfo,
                                      const ViewpointMediaTrackInfo& aMediaTrackInfo,
                                      Optional<PipelineBuildInfo> aPipelineBuildInfo);
}  // namespace VDD
