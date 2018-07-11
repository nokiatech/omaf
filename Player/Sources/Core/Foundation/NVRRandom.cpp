
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
#include "Foundation/NVRRandom.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

OMAF_INLINE uint32_t xorshift(uint32_t& x, uint32_t& y, uint32_t& z, uint32_t& w)
{
    uint32_t t = x ^ (x << 11);
    x = y;
    y = z;
    z = w;
    w = w ^ (w >> 19) ^ (t ^ (t >> 8));
    
    return w;
}

Random::Random(uint64_t seed)
{
    setSeed(seed);
}

Random::~Random()
{
}

void_t Random::setSeed(uint64_t seed)
{
    mSeed = seed;
    
    mInitVector.x = (uint32_t)seed;
    mInitVector.y = 362436069;
    mInitVector.z = 521288629;
    mInitVector.w = 88675123;
}

uint32_t Random::getUInt32()
{
    return xorshift(mInitVector.x, mInitVector.y, mInitVector.z, mInitVector.w);
}

uint32_t Random::getUInt32(uint32_t min, uint32_t max)
{
    OMAF_ASSERT(min <= max, "Minimum value must be equal or smaller than the maximum value.");
    
    uint32_t range = max - min + 1;
    
    return (getUInt32() % range) + min;
}

OMAF_NS_END
