
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
#include <algorithm>
#include <iomanip>
#include <iostream>

#include "api/streamsegmenter/frame.hpp"
#include "utils.hpp"

namespace StreamSegmenter
{
    void dumpFrameInfo(const Frame& aFrame)
    {
        size_t processSize = std::min(size_t(6), aFrame.data.size());
        std::cout << std::dec << " " << aFrame.data.size() << ": ";
        for (size_t c = 0; c < processSize; ++c)
        {
            std::cout << (c ? " " : "") << std::setw(2) << std::setfill('0') << std::hex << unsigned(aFrame.data[c]);
        }
        if (aFrame.data.size() >= 1)
        {
            AvcNalUnitType naluType = AvcNalUnitType(aFrame.data[0] & 0x1f);
            std::cout << " " << Utils::stringOfAvcNalUnitType(naluType);
        }
        std::cout << std::endl;
    }
}
