
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

#include "NVREssentials.h"

OMAF_NS_BEGIN

struct Scissors
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    
    Scissors(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
    : x(x)
    , y(y)
    , width(width)
    , height(height)
    {
    }
};

OMAF_INLINE bool_t equals(const Scissors& left, const Scissors& right)
{
    if (left.x == right.x &&
        left.y == right.y &&
        left.width == right.width &&
        left.height == right.height)
    {
        return true;
    }
    
    return false;
}

OMAF_NS_END
