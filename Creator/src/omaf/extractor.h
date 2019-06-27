
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
#pragma once

#include <memory>
#include <vector>
#include "parser/h265datastructs.hpp"
#include "processor/meta.h"

namespace VDD {

    typedef std::uint32_t StreamId;
    static const StreamId StreamIdUninitialized = 0xffffffff;

    struct Extractor 
    {
        Extractor() : idx(0), streamId(StreamIdUninitialized), nuhTemporalIdPlus1(0) {}

        struct SampleConstruct 
        {
            // idx is internal member. It should be able to give the order of constructors, also between sample and inline, as they are expected to be interleaved. 
            // E.g. you add as many SampleConstruct instances until SampleConstruct::idx >= InlineConstruct::idx, and then add InlineConstruct instances 
            size_t idx;
            // Actual track id; mapped later to the scal track reference index
            TrackId        trackId;
            // sample offset not relevant with VD use cases
            std::uint64_t  dataOffset;
            std::uint64_t  dataLength;

            struct
            {
                // original slice header as a parsed data structure
                H265::SliceHeader origSliceHeader;
                std::vector<std::uint8_t> naluHeader;
                std::uint64_t payloadOffset;
                std::uint64_t origSliceHeaderLength;
            } sliceInfo;
        };
        struct InlineConstruct 
        {
            size_t idx;
            std::vector<std::uint8_t>  inlineData;
        };

        std::vector<SampleConstruct> sampleConstruct;
        std::vector<InlineConstruct> inlineConstruct;
        size_t idx;
        StreamId streamId;
        std::uint8_t nuhTemporalIdPlus1;
    };

    typedef std::vector<Extractor> Extractors;

}
