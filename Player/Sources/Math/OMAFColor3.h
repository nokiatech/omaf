
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
#include "Math/OMAFMathTypes.h"

namespace OMAF
{
    static const Color3 Color3BlackColor = {0.0f, 0.0f, 0.0f};
    static const Color3 Color3WhiteColor = {1.0f, 1.0f, 1.0f};

    static const Color3 Color3RedColor = {1.0f, 0.0f, 0.0f};
    static const Color3 Color3GreenColor = {0.0f, 1.0f, 0.0f};
    static const Color3 Color3BlueColor = {0.0f, 0.0f, 1.0f};

    OMAF_INLINE Color3 makeColor3(float32_t r, float32_t g, float32_t b);
    OMAF_INLINE Color3 makeColor3(const float32_t v[3]);

    OMAF_INLINE bool_t equals(const Color3& l, const Color3& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE Color3 lerp(const Color3& a, const Color3& b, float32_t amount);

    OMAF_INLINE Color3 operator+(const Color3& l, const Color3& r);
    OMAF_INLINE Color3 operator-(const Color3& l, const Color3& r);
    OMAF_INLINE Color3 operator*(const Color3& l, const Color3& r);
    OMAF_INLINE Color3 operator/(const Color3& l, const Color3& r);

    OMAF_INLINE void_t operator+=(Color3& l, const Color3& r);
    OMAF_INLINE void_t operator-=(Color3& l, const Color3& r);
    OMAF_INLINE void_t operator*=(Color3& l, const Color3& r);
    OMAF_INLINE void_t operator/=(Color3& l, const Color3& r);

    OMAF_INLINE Color3 operator+(const Color3& c, float32_t s);
    OMAF_INLINE Color3 operator-(const Color3& c, float32_t s);
    OMAF_INLINE Color3 operator*(const Color3& c, float32_t s);
    OMAF_INLINE Color3 operator/(const Color3& c, float32_t s);

    OMAF_INLINE void_t operator+=(Color3& c, float32_t s);
    OMAF_INLINE void_t operator-=(Color3& c, float32_t s);
    OMAF_INLINE void_t operator*=(Color3& c, float32_t s);
    OMAF_INLINE void_t operator/=(Color3& c, float32_t s);

    OMAF_INLINE bool_t operator==(const Color3& l, const Color3& r);
    OMAF_INLINE bool_t operator!=(const Color3& l, const Color3& r);
}  // namespace OMAF

namespace OMAF
{
    Color3 makeColor3(float32_t r, float32_t g, float32_t b)
    {
        Color3 result = {r, g, b};

        return result;
    }

    Color3 makeColor3(const float32_t v[3])
    {
        Color3 result = {v[0], v[1], v[2]};

        return result;
    }

    bool_t equals(const Color3& l, const Color3& r, float32_t epsilon)
    {
        return (fabsf(l.r - r.r) <= epsilon && fabsf(l.g - r.g) <= epsilon && fabsf(l.b - r.b) <= epsilon);
    }

    Color3 lerp(const Color3& a, const Color3& b, float32_t amount)
    {
        Color3 result;
        result.r = (uint8_t)(a.r * (1.0f - amount) + (b.r * amount));
        result.g = (uint8_t)(a.g * (1.0f - amount) + (b.g * amount));
        result.b = (uint8_t)(a.b * (1.0f - amount) + (b.b * amount));

        return result;
    }


    Color3 operator+(const Color3& l, const Color3& r)
    {
        return makeColor3(l.r + r.r, l.g + r.g, l.b + r.b);
    }

    Color3 operator-(const Color3& l, const Color3& r)
    {
        return makeColor3(l.r - r.r, l.g - r.g, l.b - r.b);
    }

    Color3 operator*(const Color3& l, const Color3& r)
    {
        return makeColor3(l.r * r.r, l.g * r.g, l.b * r.b);
    }

    Color3 operator/(const Color3& l, const Color3& r)
    {
        return makeColor3(l.r / r.r, l.g / r.g, l.b / r.b);
    }


    void_t operator+=(Color3& l, const Color3& r)
    {
        l.r += r.r;
        l.g += r.g;
        l.b += r.b;
    }

    void_t operator-=(Color3& l, const Color3& r)
    {
        l.r -= r.r;
        l.g -= r.g;
        l.b -= r.b;
    }

    void_t operator*=(Color3& l, const Color3& r)
    {
        l.r *= r.r;
        l.g *= r.g;
        l.b *= r.b;
    }

    void_t operator/=(Color3& l, const Color3& r)
    {
        l.r /= r.r;
        l.g /= r.g;
        l.b /= r.b;
    }


    Color3 operator+(const Color3& c, float32_t s)
    {
        return makeColor3(c.r + s, c.g + s, c.b + s);
    }

    Color3 operator-(const Color3& c, float32_t s)
    {
        return makeColor3(c.r - s, c.g - s, c.b - s);
    }

    Color3 operator*(const Color3& c, float32_t s)
    {
        return makeColor3(c.r * s, c.g * s, c.b * s);
    }

    Color3 operator/(const Color3& c, float32_t s)
    {
        return makeColor3(c.r / s, c.g / s, c.b / s);
    }


    void_t operator+=(Color3& c, float32_t s)
    {
        c.r += s;
        c.g += s;
        c.b += s;
    }

    void_t operator-=(Color3& c, float32_t s)
    {
        c.r -= s;
        c.g -= s;
        c.b -= s;
    }

    void_t operator*=(Color3& c, float32_t s)
    {
        c.r *= s;
        c.g *= s;
        c.b *= s;
    }

    void_t operator/=(Color3& c, float32_t s)
    {
        c.r /= s;
        c.g /= s;
        c.b /= s;
    }


    bool_t operator==(const Color3& l, const Color3& r)
    {
        return equals(l, r);
    }

    bool_t operator!=(const Color3& l, const Color3& r)
    {
        return !equals(l, r);
    }
}  // namespace OMAF
