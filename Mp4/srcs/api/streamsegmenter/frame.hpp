
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#ifndef FRAME_H
#define FRAME_H

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <list>
#include <vector>

#include "optional.hpp"
#include "rational.hpp"

namespace StreamSegmenter
{
    // Originates from moviefragmentsdatatypes.hpp from MP4VR
    // Sample Flags Field as defined in 8.8.3.1 of ISO/IEC 14496-12:2015(E)
    struct SampleFlagsType
    {
        uint32_t reserved : 4, is_leading : 2, sample_depends_on : 2, sample_is_depended_on : 2,
            sample_has_redundancy : 2, sample_padding_value : 3, sample_is_non_sync_sample : 1,
            sample_degradation_priority : 16;
    };

    union SampleFlags {
        uint32_t flagsAsUInt;
        SampleFlagsType flags;
    };

    struct SampleDefaults
    {
        std::uint32_t trackId;
        std::uint32_t defaultSampleDescriptionIndex;
        std::uint32_t defaultSampleDuration;
        std::uint32_t defaultSampleSize;
        SampleFlags defaultSampleFlags;
    };

    typedef std::vector<uint8_t> FrameData;

    typedef RatS64 FrameTime;
    typedef RatU64 FrameDuration;
    typedef RatU64 FrameRate;

    const FrameDuration NO_DURATION = InvalidRational();

    typedef std::list<FrameTime> FrameCts;  // Composition time
    typedef FrameTime FrameDts;             // Decoding time

    const FrameCts NO_CTS{};

    struct FrameInfo
    {
        FrameCts cts;
        FrameDuration duration;
        bool isIDR;
        SampleFlags sampleFlags;

        Utils::Optional<FrameDts> dts;

        FrameInfo() = default;
    };

    struct Frame
    {
        FrameInfo info;
        FrameData data;
    };

    void dumpFrameInfo(const Frame& aFrame);
}

#endif  // FRAME_H
