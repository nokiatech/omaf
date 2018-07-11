
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

#include <math.h>

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"
#include "Math/OMAFMathConstants.h"

namespace OMAF
{
    template<typename T>
    const T& min(const T& a, const T& b)
    {
        return (a < b) ? a : b;
    }

    template<typename T>
    const T& max(const T& a, const T& b)
    {
        return (a > b) ? a : b;
    }

    template<typename T>
    T abs(const T& a)
    {
        return (a >= 0) ? a : -a;
    }

    template<typename T>
    const T& clamp(const T& value, const T& min, const T& max)
    {
        if(value < min)
        {
            return min;
        }
        else if(value > max)
        {
            return max;
        }
        else
        {
            return value;
        }
    }

    template<typename T>
    T square(const T& value)
    {
        return (value * value);
    }

    OMAF_INLINE float32_t fabsf(float32_t value);
    OMAF_INLINE float64_t fabs(float64_t value);

    OMAF_INLINE float32_t powf(float32_t base, float32_t exponent);
    OMAF_INLINE float64_t pow(float64_t base, float64_t exponent);

    OMAF_INLINE float32_t sqrtf(float32_t value);
    OMAF_INLINE float64_t sqrt(float64_t value);

    OMAF_INLINE float32_t log2f(float32_t value);
    OMAF_INLINE float64_t log2(float64_t value);

    OMAF_INLINE float32_t floorf(float32_t value);
    OMAF_INLINE float64_t floor(float64_t value);

    OMAF_INLINE float32_t ceilf(float32_t value);
    OMAF_INLINE float64_t ceil(float64_t value);

    OMAF_INLINE float32_t roundf(float32_t value);
    OMAF_INLINE float64_t round(float64_t value);

    OMAF_INLINE float32_t fmodf(float32_t x, float32_t y);
    OMAF_INLINE float64_t fmod(float64_t x, float64_t y);

    OMAF_INLINE float32_t modff(float32_t value, float32_t& integer);
    OMAF_INLINE float64_t modf(float64_t value, float64_t& integer);

    OMAF_INLINE float32_t sinf(float32_t radians);
    OMAF_INLINE float64_t sin(float64_t radians);

    OMAF_INLINE float32_t cosf(float32_t radians);
    OMAF_INLINE float64_t cos(float64_t radians);

    OMAF_INLINE float32_t tanf(float32_t radians);
    OMAF_INLINE float64_t tan(float64_t radians);

    OMAF_INLINE float32_t asinf(float32_t value);
    OMAF_INLINE float64_t asin(float64_t value);

    OMAF_INLINE float32_t acosf(float32_t value);
    OMAF_INLINE float64_t acos(float64_t value);

    OMAF_INLINE float32_t atanf(float32_t value);
    OMAF_INLINE float64_t atan(float64_t value);

    OMAF_INLINE float32_t atan2f(float32_t y, float32_t x);
    OMAF_INLINE float64_t atan2(float64_t y, float64_t x);

    OMAF_INLINE float32_t fminf(float32_t a, float32_t b);
    OMAF_INLINE float64_t fmin(float64_t a, float64_t b);

    OMAF_INLINE float32_t fmaxf(float32_t a, float32_t b);
    OMAF_INLINE float64_t fmax(float64_t a, float64_t b);

    OMAF_INLINE float32_t toRadians(float32_t degrees);
    OMAF_INLINE float64_t toRadians(float64_t degrees);
    OMAF_INLINE float32_t toDegrees(float32_t radians);
    OMAF_INLINE float64_t toDegrees(float64_t radians);

    // Reduces a given angle to a value between π and -π
    OMAF_INLINE float32_t clampAngle(float32_t radians);

    OMAF_INLINE bool_t equals(float32_t l, float32_t r, float32_t epsilon = OMAF_FLOAT32_EPSILON);
}

namespace OMAF
{
    float32_t fabsf(float32_t value)
    {
        return ::fabsf(value);
    }

    float64_t fabs(float64_t value)
    {
        return ::fabs(value);
    }

    float32_t powf(float32_t base, float32_t exponent)
    {
        return ::powf(base, exponent);
    }

    float64_t pow(float64_t base, float64_t exponent)
    {
        return ::pow(base, exponent);
    }

    float32_t sqrtf(float32_t value)
    {
        return ::sqrtf(value);
    }

    float64_t sqrt(float64_t value)
    {
        return ::sqrt(value);
    }

    float32_t log2f(float32_t value)
    {
        float32_t a = ::logf(value);
        static const float32_t b = ::logf(2.0f);

        return (a / b);
    }

    float64_t log2(float64_t value)
    {
        float64_t a = ::log(value);
        static const float64_t b = ::log(2.0);

        return (a / b);
    }

    float32_t floorf(float32_t value)
    {
        return ::floorf(value);
    }

    float64_t floor(float64_t value)
    {
        return ::floor(value);
    }

    float32_t ceilf(float32_t value)
    {
        return ::ceilf(value);
    }

    float64_t ceil(float64_t value)
    {
        return ::ceil(value);
    }

    float32_t roundf(float32_t value)
    {
        return ::roundf(value);
    }

    float64_t round(float64_t value)
    {
        return ::round(value);
    }

    float32_t fmodf(float32_t x, float32_t y)
    {
        return ::fmodf(x, y);
    }

    float64_t fmod(float64_t x, float64_t y)
    {
        return ::fmod(x, y);
    }

    float32_t modff(float32_t value, float32_t& integer)
    {
        return ::modff(value, &integer);
    }

    float64_t modf(float64_t value, float64_t& integer)
    {
        return ::modf(value, &integer);
    }

    float32_t sinf(float32_t radians)
    {
        return ::sinf(radians);
    }

    float64_t sin(float64_t radians)
    {
        return ::sin(radians);
    }

    float32_t cosf(float32_t radians)
    {
        return ::cosf(radians);
    }

    float64_t cos(float64_t radians)
    {
        return ::cos(radians);
    }

    float32_t tanf(float32_t radians)
    {
        return ::tanf(radians);
    }

    float64_t tan(float64_t radians)
    {
        return ::tan(radians);
    }

    float32_t asinf(float32_t value)
    {
        return ::asinf(value);
    }

    float64_t asin(float64_t value)
    {
        return ::asin(value);
    }

    float32_t acosf(float32_t value)
    {
        return ::acosf(value);
    }

    float64_t acos(float64_t value)
    {
        return ::acos(value);
    }

    float32_t atanf(float32_t value)
    {
        return ::atanf(value);
    }

    float64_t atan(float64_t value)
    {
        return ::atan(value);
    }

    float32_t atan2f(float32_t y, float32_t x)
    {
        return ::atan2f(y, x);
    }

    float64_t atan2(float64_t y, float64_t x)
    {
        return ::atan2(y, x);
    }

    float32_t fminf(float32_t a, float32_t b)
    {
        return ::fminf(a, b);
    }

    float64_t fmin(float64_t a, float64_t b)
    {
        return ::fmin(a, b);
    }

    float32_t fmaxf(float32_t a, float32_t b)
    {
        return ::fmaxf(a, b);
    }

    float64_t fmax(float64_t a, float64_t b)
    {
        return ::fmax(a, b);
    }

    float32_t toRadians(float32_t degrees)
    {
        return degrees * (OMAF_PI / 180.0f);
    }

    float64_t toRadians(float64_t degrees)
    {
        return degrees * (OMAF_PI / 180.0);
    }

    float32_t toDegrees(float32_t radians)
    {
        return radians * (180.0f / OMAF_PI);
    }

    float64_t toDegrees(float64_t radians)
    {
        return radians * (180.0 / OMAF_PI);
    }

    float32_t clampAngle(float32_t radians)
    {
        while (radians < -OMAF_PI)
        {
            radians += OMAF_2PI;
        }

        while (radians > OMAF_PI)
        {
            radians -= OMAF_2PI;
        }

        return radians;
    }


    bool_t equals(float32_t l, float32_t r, float32_t epsilon)
    {
        return (fabsf(l - r) <= epsilon);
    }
}
