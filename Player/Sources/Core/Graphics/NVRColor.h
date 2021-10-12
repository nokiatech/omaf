
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

#include "Math/OMAFMathFunctions.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN
// NOTE: these functions do RGBA order, which might be confusing since usually alpha is at the top 8 bits (ie. ARGB)..
OMAF_INLINE void_t unpackColor(uint32_t rgba, float32_t& r, float32_t& g, float32_t& b, float32_t& a)
{
    uint8_t cr = (uint8_t)((rgba & 0xFF000000) >> 24);
    uint8_t cg = (uint8_t)((rgba & 0x00FF0000) >> 16);
    uint8_t cb = (uint8_t)((rgba & 0x0000FF00) >> 8);
    uint8_t ca = (uint8_t)((rgba & 0x000000FF) >> 0);

    r = (float32_t) cr / 255.0f;
    g = (float32_t) cg / 255.0f;
    b = (float32_t) cb / 255.0f;
    a = (float32_t) ca / 255.0f;
}

OMAF_INLINE uint32_t packColor(float32_t r, float32_t g, float32_t b, float32_t a)
{
    uint8_t cr = (uint8_t) fmaxf(0.0f, fminf(r * 255.0f, 255.0f));
    uint8_t cg = (uint8_t) fmaxf(0.0f, fminf(g * 255.0f, 255.0f));
    uint8_t cb = (uint8_t) fmaxf(0.0f, fminf(b * 255.0f, 255.0f));
    uint8_t ca = (uint8_t) fmaxf(0.0f, fminf(a * 255.0f, 255.0f));

    uint32_t color =
        (((uint32_t) cr) << 24) | (((uint32_t) cg) << 16) | (((uint32_t) cb) << 8) | (((uint32_t) ca) << 0);

    return color;
}

OMAF_NS_END
