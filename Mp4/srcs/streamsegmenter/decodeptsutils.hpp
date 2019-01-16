
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
#ifndef STREAMSEGMENTER_DECODEPTSUTILS_HPP
#define STREAMSEGMENTER_DECODEPTSUTILS_HPP

#include "api/streamsegmenter/rational.hpp"
#include "decodepts.hpp"
#include "writeoncemap.hpp"

namespace StreamSegmenter
{
    using PTSTime      = RatS64;
    using PTSSampleMap = WriteOnceMap<PTSTime, DecodePts::SampleIndex>;

    PTSSampleMap getTimes(const DecodePts& aDecodePts, uint32_t aTimeScale);
}

#endif /* end of include guard: STREAMSEGMENTER_DECODEPTSUTILS_HPP */
