
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
#pragma once

#include <vector>
#include "customallocator.hpp"

namespace Hvc2Extractor
{
    struct ExtNalHdr
    {
        std::uint8_t forbidden_zero_bit    = 0;
        std::uint8_t nal_unit_type         = 0;
        std::uint8_t nuh_layer_id          = 0;
        std::uint8_t nuh_temporal_id_plus1 = 0;
    };

    struct ExtNalDat
    {
        struct SampleConstruct
        {
            std::uint8_t order_idx;
            std::uint8_t constructor_type;
            std::uint8_t track_ref_index;
            std::int8_t sample_offset;
            std::uint32_t data_offset;
            std::uint32_t data_length;
        };
        struct InlineConstruct
        {
            std::uint8_t order_idx;
            std::uint8_t constructor_type;
            std::uint8_t data_length;
            std::vector<std::uint8_t> inline_data;
        };

        std::vector<SampleConstruct> sampleConstruct;
        std::vector<InlineConstruct> inlineConstruct;
    };


    struct Hvc2ExtractorNal
    {
        ExtNalHdr extNalHdr = {};
        ExtNalDat extNalDat = {};
    };

    using DataVector = Vector<std::uint8_t>;

    // If the NalData is affirmed to be an extractor NAL, parse it to extNalDat
    bool parseExtractorNal(const DataVector& NalData,
                           ExtNalDat& extNalDat,
                           uint8_t lengthSizeMinus1,
                           uint64_t& extractionSize);
};  // namespace Hvc2Extractor
