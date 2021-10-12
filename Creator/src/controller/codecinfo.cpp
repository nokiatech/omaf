
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
#include "codecinfo.h"

#include "common/utils.h"

namespace VDD
{
    HevcCodecInfo getHevcCodecInfo(const std::vector<uint8_t>& aSps)
    {
        size_t k = 0; // k indexes aSps
        if (aSps[0] == 0 && aSps[1] == 0 && aSps[2] == 0 && aSps[3] == 1)
        {
            // SPS has 4 byte start code
            k += 4;
        }
        // scan the relevant part of SPS (NALU header + profile_tier_level == 16 RBSP bytes => 20+ input bytes) for EPB and remove if found
        std::vector<uint8_t> rbsp;
        for (; k < 25; k++)
        {
            rbsp.push_back(aSps[k]);
            if (aSps[k] == 0 && aSps[k + 1] == 0 && aSps[k + 2] == 3)
            {
                // skip the EPB
                rbsp.push_back(aSps[k + 1]);
                k += 2; // k gets updated again in the for-loop
            }
        }

        // NALU header 2 bytes
        //uint8_t sps_video_parameter_set_id: 4 bits
        //uint8_t sps_max_sub_layers_minus1: 3 bits
        //uint8_t sps_temporal_id_nesting_flag: 1 bit;
        size_t i = 3; // i indexes rbsp

        HevcCodecInfo hevcCodecInfo {};
        hevcCodecInfo.general_profile_space = (rbsp.at(i) >> 6) & 0x03;
        hevcCodecInfo.general_tier_flag = (rbsp.at(i) >> 5) & 0x01;
        hevcCodecInfo.general_profile_idc = rbsp.at(i) & 0x1F;
        i += 1;
        uint32_t spsu[4] = { rbsp.at(i), rbsp.at(i + 1), rbsp.at(i + 2), rbsp.at(i + 3) };
        hevcCodecInfo.general_profile_compatibility_flags = Utils::reverse((spsu[0] << 24) | (spsu[1] << 16) | (spsu[2] << 8) | spsu[3]);
        i += 4;
        hevcCodecInfo.general_progressive_source_flag_index = i;
        i += 6; // size of general_progressive_source_flags
        hevcCodecInfo.general_level_idc = rbsp.at(i);

        // fourth+n element: ( zero values at end can be omitted )
        uint32_t numberOfElements = 0;
        for (size_t j = 0; j < 6; ++j)
        {
            if (rbsp.at(hevcCodecInfo.general_progressive_source_flag_index + j) != 0)
            {
                numberOfElements = (uint32_t)j + 1;
            }
        }

        size_t element = 0;
        while (numberOfElements > 0)
        {
            hevcCodecInfo.flags.push_back(uint32_t(rbsp.at(hevcCodecInfo.general_progressive_source_flag_index + element)));
            element++;
            numberOfElements--;
        }

        return hevcCodecInfo;
    }

    std::string getCodecInfoString(const HevcCodecInfo& aCodecInfo)
    {
        std::string codecString;

        // first element: aCodecInfo.general_profile_space + aCodecInfo.general_profile_idc info
        if (aCodecInfo.general_profile_space) // 0 value is omitted from output on purpose
        {
            if (aCodecInfo.general_profile_space == 1)
            {
                codecString.push_back('A');
            }
            else if (aCodecInfo.general_profile_space == 2)
            {
                codecString.push_back('B');
            }
            else if (aCodecInfo.general_profile_space == 3)
            {
                codecString.push_back('C');
            }
        }
        codecString += std::to_string(static_cast<unsigned int>(aCodecInfo.general_profile_idc));
        codecString.push_back('.');

        // second element: aCodecInfo.general_profile_compatibility_flags info
        std::stringstream stream;
        stream << std::hex << aCodecInfo.general_profile_compatibility_flags;  // reverse bit order 32 bit flags field as hex
        codecString += stream.str();
        codecString.push_back('.');

        // third element: aCodecInfo.general_tier_flag + aCodecInfo.general_level_idc
        if (aCodecInfo.general_tier_flag == 0)
        {
            codecString.push_back('L');
        }
        else // aCodecInfo.general_tier_flag == 1, other values are not specified by spec so assume all non-zero are H.
        {
            codecString.push_back('H');
        }
        codecString += std::to_string(static_cast<unsigned int>(aCodecInfo.general_level_idc));

        size_t element = 0;
        for (auto flag: aCodecInfo.flags)
        {
            codecString.push_back('.');
            std::stringstream flags;
            flags << std::hex << flag;
            codecString += flags.str();
            element++;
        }

        return codecString;
    }

}