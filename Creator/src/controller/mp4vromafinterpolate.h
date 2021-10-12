
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

#include "config/config.h"
#include "reader/mp4vrfiledatatypes.h"

#include "common.h"

namespace VDD
{
    struct InterpolationInstructions
    {
        StreamSegmenter::FrameDuration frameInterval;
    };

    struct InterpolationContext
    {
        double at;
        const ConfigValue& valueA;
        const ConfigValue& valueB;
    };

    template <typename Sample>
    struct InterpolatedSample : public SampleTimeDuration<Sample>
    {
        ConfigValue configValue;
        Optional<ParsedValue<InterpolationInstructions>> interpolateToNext;
    };

    template <typename AugmentedSample>
        void validateTimedMetadata(const std::vector<AugmentedSample>& aSamples);

    template <typename Sample>
    std::vector<InterpolatedSample<Sample>> performSampleInterpolation(
        const std::vector<InterpolatedSample<Sample>>& aSamples,
        Optional<StreamSegmenter::Segmenter::Duration> aForceFrameInterval);

    template <typename Sample>
    std::function<InterpolatedSample<Sample>(const ConfigValue& aValue)> readInterpolatedSample(
        std::function<Sample(const ConfigValue& aValue)> aReader);

    InterpolationInstructions readInterpolationInstructions(const ConfigValue& aValue);

    ISOBMFF::SphereRegionSample interpolate(const InterpolationContext& aCtx, const ISOBMFF::SphereRegionSample& a,
                                            const ISOBMFF::SphereRegionSample& b);

    template <typename Sample>
    InterpolatedSample<Sample> interpolate(const InterpolationContext& aCtx, const InterpolatedSample<Sample>& a,
                                           const InterpolatedSample<Sample>& b);

    MP4VR::OverlayConfigSample interpolate(const InterpolationContext& aCtx /*0..1*/, const MP4VR::OverlayConfigSample& a, const MP4VR::OverlayConfigSample& b);
    MP4VR::InitialViewingOrientationSample interpolate(const InterpolationContext& aCtx /*0..1*/, const MP4VR::InitialViewingOrientationSample& a, const MP4VR::InitialViewingOrientationSample& b);
    ISOBMFF::InitialViewpointSample interpolate(const InterpolationContext& aCtx /*0..1*/,
                                                const ISOBMFF::InitialViewpointSample& a,
                                                const ISOBMFF::InitialViewpointSample&);
    ISOBMFF::DynamicViewpointSample interpolate(const InterpolationContext& aCtx /*0..1*/,
                                                const ISOBMFF::DynamicViewpointSample& a,
                                                const ISOBMFF::DynamicViewpointSample&);
}  // namespace VDD

#include "mp4vromafinterpolate.icpp"
