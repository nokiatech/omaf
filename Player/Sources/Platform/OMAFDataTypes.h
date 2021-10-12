
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

#include <stddef.h>
#include <stdint.h>

typedef void void_t;
typedef bool bool_t;
typedef char char_t;
typedef char utf8_t;

typedef float float32_t;
typedef double float64_t;

// NOTE: C99 should define int64_t and INT64_MAX simultaneously.
#ifndef INT64_MAX
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#endif


#define OMAF_INT8_MIN (-0x7f - 1)
#define OMAF_INT8_MAX 0x7f

#define OMAF_UINT8_MIN 0
#define OMAF_UINT8_MAX 0xff

#define OMAF_INT16_MIN (-0x7fff - 1)
#define OMAF_INT16_MAX 0x7fff

#define OMAF_UINT16_MIN 0
#define OMAF_UINT16_MAX 0xffff

#define OMAF_INT32_MIN (-0x7fffffff - 1)
#define OMAF_INT32_MAX 0x7fffffff

#define OMAF_UINT32_MIN 0
#define OMAF_UINT32_MAX 0xffffffff

#define OMAF_INT64_MIN (-0x7fffffffffffffff - 1)
#define OMAF_INT64_MAX 0x7fffffffffffffff

#define OMAF_UINT64_MIN 0
#define OMAF_UINT64_MAX 0xffffffffffffffffu

#define OMAF_FLOAT32_MIN 3.4E-38f
#define OMAF_FLOAT32_MAX 3.4E+38f

#define OMAF_FLOAT32_POS_INF (OMAF::castUInt32ToFloat32(0x7F800000))
#define OMAF_FLOAT32_NEG_INF (OMAF::castUInt32ToFloat32(0xFF800000))

#define OMAF_FLOAT32_NANS (OMAF::castUInt32ToFloat32(0x7F800001))
#define OMAF_FLOAT32_NANQ (OMAF::castUInt32ToFloat32(0x7FC00000))
#define OMAF_FLOAT32_NAN OMAF_FLOAT32_NANQ

#define OMAF_FLOAT64_MIN 1.7E-308
#define OMAF_FLOAT64_MAX 1.7E+308

#define OMAF_FLOAT64_POS_INF (OMAF::castUInt64ToFloat64(0x7FF0000000000000))
#define OMAF_FLOAT64_NEG_INF (OMAF::castUInt64ToFloat64(0xFFF0000000000000))

#define OMAF_FLOAT64_NANS (OMAF::castUInt64ToFloat64(0x7FF0000000000001))
#define OMAF_FLOAT64_NANQ (OMAF::castUInt64ToFloat64(0x7FF8000000000000))
#define OMAF_FLOAT64_NAN OMAF_FLOAT64_NANQ

#ifndef OMAF_NULL

#if defined(__cplusplus)

#define OMAF_NULL 0

#else

#define OMAF_NULL ((void_t*) 0)

#endif

#endif

namespace OMAF
{
    namespace ComparisonResult
    {
        enum Enum
        {
            INVALID = -1,

            LESS,     // left < right
            EQUAL,    // left == right
            GREATER,  // left > right

            COUNT,
        };
    }
}  // namespace OMAF

namespace OMAF
{
    inline float32_t castUInt32ToFloat32(uint32_t value)
    {
        struct UnionCast
        {
            union {
                uint32_t integer;
                float32_t float32;
            };
        };

        UnionCast cast;
        cast.integer = value;

        return cast.float32;
    }

    inline float64_t castUInt64ToFloat64(uint64_t value)
    {
        struct UnionCast
        {
            union {
                uint64_t integer;
                float64_t float64;
            };
        };

        UnionCast cast;
        cast.integer = value;

        return cast.float64;
    }
}  // namespace OMAF
