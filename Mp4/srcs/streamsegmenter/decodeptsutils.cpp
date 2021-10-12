
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
#include "decodeptsutils.hpp"

namespace StreamSegmenter
{
    PTSSampleMap getTimes(const DecodePts& aDecodePts, uint32_t aTimeScale)
    {
        PTSSampleMap pts;
        for (auto timeSample : aDecodePts.getTimeTS())
        {
            pts.insert(std::make_pair(RatS64{timeSample.first, static_cast<int64_t>(aTimeScale)}, timeSample.second));
        }
        return pts;
    }
}
