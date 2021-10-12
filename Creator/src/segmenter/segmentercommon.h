
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

#include <map>
#include <vector>
#include <cstdint>
#include <string>

#include <streamsegmenter/id.hpp>

#include "processor/meta.h"

namespace VDD
{
    typedef StreamSegmenter::IdBase<std::int8_t, struct ScalTrefIndexTag> ScalTrefIndex;

    // First key is the extractor track id, second key is the track id to map; result
    // is obviously the final scal track reference index
    using TrackToScalTrafIndexMap = std::map<TrackId, std::map<TrackId, ScalTrefIndex>>;

    enum class OutputMode
    {
        None,
        OMAFV1,
        OMAFV2
    };

    StreamSegmenter::Segmenter::WriterMode writerModeOfOutputMode(OutputMode aOutputMode);

    enum class Xtyp
    {
        Styp,
        Ftyp,
        Sibm,
        Imds,
    };

    struct SegmentHeader {
        Xtyp type;
        std::string brand4cc;
    };

    std::vector<std::uint8_t> makeXtyp(const SegmentHeader& aSegmentHeader);

}  // namespace VDD