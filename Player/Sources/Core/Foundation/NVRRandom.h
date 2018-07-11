
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Pseudo Random Number Generator (PRNG) using Xorshift algoritm, https://en.wikipedia.org/wiki/Xorshift
//
// Features:
//
//  - Evenly distributed bit-sequence, survices e.g. Diehard statical test
//  - Small memory footprint
//  - Extremely fast (using only shift and xor bit-operations)
//  - Period length 2 ^ 128 - 1
//
// Note: Xorshift is NOT a cryptographically-secure random number generator, it will fail in some statical tests e.g. MatrixRank and LinearComp.
//

class Random
{
    public:

        static const uint32_t Max = 0xffffffff;

    public:

        OMAF_EXPLICIT Random(uint64_t seed);
        ~Random();

        void_t setSeed(uint64_t seed);

        uint32_t getUInt32();
        uint32_t getUInt32(uint32_t min, uint32_t max = Max);

    private:
    
        OMAF_NO_DEFAULT(Random);
        OMAF_NO_COPY(Random);
        OMAF_NO_ASSIGN(Random);

    private:

        struct InitVector
        {
            uint32_t x;
            uint32_t y;
            uint32_t z;
            uint32_t w;
        };

        uint64_t mSeed;

        InitVector mInitVector;
};

OMAF_NS_END
