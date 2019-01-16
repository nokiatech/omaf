
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "dash.h"

#include "common.h"
#include "controllerops.h"
#include "configreader.h"
#include "log/log.h"

namespace VDD
{

    Dash::Dash(std::shared_ptr<Log> aLog,
        const ConfigValue& aDashConfig,
        ControllerOps& aControllerOps)
        : mOps(aControllerOps)
        , mLog(aLog)
        , mDashConfig(aDashConfig)
        , mOverrideTotalDuration(VDD::readOptional("total duration", readSegmentDuration)(mDashConfig.tryTraverse(configPathOfString("mpd.total_duration"))))
    {

    }

    Dash::~Dash() = default;

    void Dash::hookSegmentSaverSignal(const DashSegmenterConfig& aDashOutput, AsyncProcessor* aNode)
    {
        if (!mSegmentSavedSignals.count(aDashOutput.segmenterName))
        {
            SegmentSaverSignal signal;
            signal.combineSignals = Utils::make_unique<CombineNode>(mOps.getGraph());
            mSegmentSavedSignals.insert(std::make_pair(aDashOutput.segmenterName, std::move(signal)));
        }
        auto& savedSignal = mSegmentSavedSignals.at(aDashOutput.segmenterName);

        connect(*aNode, *mOps.configureForGraph(savedSignal.combineSignals->getSink(savedSignal.sinkCount++)));
    }

    bool Dash::isEnabled() const
    {
        return mDashConfig.valid();
    }

    ConfigValue Dash::dashConfigFor(std::string aDashName) const
    {
        return mDashConfig[aDashName];
    }

    void Dash::createHevcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec)
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

        uint8_t general_profile_space = (rbsp.at(i) >> 6) & 0x03;
        uint8_t general_tier_flag = (rbsp.at(i) >> 5) & 0x01;
        uint8_t general_profile_idc = rbsp.at(i) & 0x1F;
        i += 1;
        uint32_t spsu[4]{ rbsp.at(i), rbsp.at(i + 1), rbsp.at(i + 2), rbsp.at(i + 3) };
        uint32_t general_profile_compatibility_flags = Utils::reverse((spsu[0] << 24) | (spsu[1] << 16) | (spsu[2] << 8) | spsu[3]);
        i += 4;
        size_t general_progressive_source_flag_index = i;
        i += 6; // size of general_progressive_source_flags
        uint8_t general_level_idc = rbsp.at(i);

        // first element: general_profile_space + general_profile_idc info
        if (general_profile_space) // 0 value is omitted from output on purpose
        {
            if (general_profile_space == 1)
            {
                aCodec.push_back('A');
            }
            else if (general_profile_space == 2)
            {
                aCodec.push_back('B');
            }
            else if (general_profile_space == 3)
            {
                aCodec.push_back('C');
            }
        }
        aCodec += std::to_string(general_profile_idc);
        aCodec.push_back('.');

        // second element: general_profile_compatibility_flags info
        std::stringstream stream;
        stream << std::hex << general_profile_compatibility_flags;  // reverse bit order 32 bit flags field as hex
        aCodec += stream.str();
        aCodec.push_back('.');

        // third element: general_tier_flag + general_level_idc
        if (general_tier_flag == 0)
        {
            aCodec.push_back('L');
        }
        else // general_tier_flag == 1, other values are not specified by spec so assume all non-zero are H.
        {
            aCodec.push_back('H');
        }
        aCodec += std::to_string(general_level_idc);

        // fourth+n element: ( zero values at end can be omitted )
        uint32_t numberOfElements = 0;
        for (size_t j = 0; j < 6; ++j)
        {
            if (rbsp.at(general_progressive_source_flag_index + j) != 0)
            {
                numberOfElements = (uint32_t)j + 1;
            }
        }

        size_t element = 0;
        while (numberOfElements > 0)
        {
            aCodec.push_back('.');
            std::stringstream flags;
            flags << std::hex << uint32_t(rbsp.at(general_progressive_source_flag_index + element));
            aCodec += flags.str();
            element++;
            numberOfElements--;
        }
    }

    void Dash::createAvcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec)
    {
        static constexpr char hex[] = "0123456789ABCDEF";
        size_t i = 0;
        if (aSps[0] == 0 && aSps[1] == 0 && aSps[2] == 0 && aSps[3] == 1)
        {
            i += 4;
        }
        i++;
        // skip start code (4 bytes) + first byte (forbidden_zero_bit + nal_ref_idc + nal_unit_type)
        // no EPB can take place here, as profile_idc > 0 and level_idc > 0
        for (int j = 0; j < 3; ++i, j++)
        {// copy profile_idc, constraint_set_flags and level_idc (3 bytes in total)
            aCodec.push_back(hex[aSps.at(i) / 16]);
            aCodec.push_back(hex[aSps.at(i) % 16]);
        }
    }
}
