
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

#include <cstdint>
#include <string>
#include <vector>

namespace VDD
{
    struct HevcCodecInfo
    {
        uint8_t general_profile_space;
        uint8_t general_tier_flag;
        uint8_t general_profile_idc;
        uint32_t spsu[4];
        uint32_t general_profile_compatibility_flags;
        size_t general_progressive_source_flag_index;
        uint8_t general_level_idc;

        std::vector<uint32_t> flags;
    };

    /** Given an SPS of a HEVC video stream, produce general information out of it, useful for
     * constructing the MPD codec string */
    HevcCodecInfo getHevcCodecInfo(const std::vector<uint8_t>& aSps);

    /** Given the codec info produced by getHevcCodecInfo, produce the codec info string suitable
     * for MPD */
    std::string getCodecInfoString(const HevcCodecInfo& aCodecInfo);
}