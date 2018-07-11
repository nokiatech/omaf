
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
#ifndef STREAMSEGMENTER_CONFIG_HPP
#define STREAMSEGMENTER_CONFIG_HPP

#include <string>

#include "api/streamsegmenter/segmenterapi.hpp"
#include "json.hh"

namespace StreamSegmenter
{
    namespace Config
    {
        struct Input
        {
            std::string file;
        };

        struct MPD
        {
            std::string outputMPD;
        };

        struct Config
        {
            Input input;
            Segmenter::Duration segmentDuration;
            MPD mpd;
            FrameDuration timescale;  // 1/timescale
            std::uint16_t width;
            std::uint16_t height;
        };

        Config load(const Json::Value& aConfig);
    }
}  // namespace StreamSegmenter::Config

#endif  // STREAMSEGMENTER_CONFIG_HPP
