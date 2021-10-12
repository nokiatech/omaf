
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

#include "Math/OMAFMathTypes.h"

#include "Math/OMAFMathConstants.h"
#include "Math/OMAFMathFunctions.h"

namespace OMAF
{
    static const Color4 Color4BlackColor = {0.0f, 0.0f, 0.0f, 1.0f};
    static const Color4 Color4WhiteColor = {1.0f, 1.0f, 1.0f, 1.0f};

    static const Color4 Color4RedColor = {1.0f, 0.0f, 0.0f, 1.0f};
    static const Color4 Color4GreenColor = {0.0f, 1.0f, 0.0f, 1.0f};
    static const Color4 Color4BlueColor = {0.0f, 0.0f, 1.0f, 1.0f};

    OMAF_INLINE Color4 makeColor4(float32_t r, float32_t g, float32_t b, float32_t a);
    OMAF_INLINE Color4 makeColor4(const float32_t v[4]);
    OMAF_INLINE Color4 makeColor4(const Color4& c, float32_t alpha);

    OMAF_INLINE bool_t equals(const Color4& l, const Color4& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE Color4 lerp(const Color4& a, const Color4& b, float32_t amount);

    OMAF_INLINE uint32_t toABGR(const Color4& color);

    OMAF_INLINE Color4 operator+(const Color4& l, const Color4& r);
    OMAF_INLINE Color4 operator-(const Color4& l, const Color4& r);
    OMAF_INLINE Color4 operator*(const Color4& l, const Color4& r);
    OMAF_INLINE Color4 operator/(const Color4& l, const Color4& r);

    OMAF_INLINE void_t operator+=(Color4& l, const Color4& r);
    OMAF_INLINE void_t operator-=(Color4& l, const Color4& r);
    OMAF_INLINE void_t operator*=(Color4& l, const Color4& r);
    OMAF_INLINE void_t operator/=(Color4& l, const Color4& r);

    OMAF_INLINE Color4 operator+(const Color4& c, float32_t s);
    OMAF_INLINE Color4 operator-(const Color4& c, float32_t s);
    OMAF_INLINE Color4 operator*(const Color4& c, float32_t s);
    OMAF_INLINE Color4 operator/(const Color4& c, float32_t s);

    OMAF_INLINE void_t operator+=(Color4& c, float32_t s);
    OMAF_INLINE void_t operator-=(Color4& c, float32_t s);
    OMAF_INLINE void_t operator*=(Color4& c, float32_t s);
    OMAF_INLINE void_t operator/=(Color4& c, float32_t s);

    OMAF_INLINE bool_t operator==(const Color4& l, const Color4& r);
    OMAF_INLINE bool_t operator!=(const Color4& l, const Color4& r);
}  // namespace OMAF

namespace OMAF
{
    Color4 makeColor4(float32_t r, float32_t g, float32_t b, float32_t a)
    {
        Color4 result = {r, g, b, a};

        return result;
    }

    Color4 makeColor4(const float32_t v[4])
    {
        Color4 result = {v[0], v[1], v[2], v[3]};

        return result;
    }

    Color4 makeColor4(const Color4& c, float32_t alpha)
    {
        Color4 result = {c.r, c.g, c.b, alpha};

        return result;
    }

    bool_t equals(const Color4& l, const Color4& r, float32_t epsilon)
    {
        return (fabsf(l.r - r.r) <= epsilon && fabsf(l.g - r.g) <= epsilon && fabsf(l.b - r.b) <= epsilon &&
                fabsf(l.a - r.a) <= epsilon);
    }

    Color4 lerp(const Color4& a, const Color4& b, float32_t amount)
    {
        Color4 result;
        result.r = (uint8_t)(a.r * (1.0f - amount) + (b.r * amount));
        result.g = (uint8_t)(a.g * (1.0f - amount) + (b.g * amount));
        result.b = (uint8_t)(a.b * (1.0f - amount) + (b.b * amount));
        result.a = (uint8_t)(a.a * (1.0f - amount) + (b.a * amount));

        return result;
    }

    uint32_t toABGR(const Color4& color)
    {
        uint32_t outColor = (uint32_t)(color.a * 255) << 24;
        outColor |= (uint32_t)(color.b * 255) << 16;
        outColor |= (uint32_t)(color.g * 255) << 8;
        outColor |= (uint32_t)(color.r * 255);
        return outColor;
    }


    Color4 operator+(const Color4& l, const Color4& r)
    {
        return makeColor4(l.r + r.r, l.g + r.g, l.b + r.b, l.a + r.a);
    }

    Color4 operator-(const Color4& l, const Color4& r)
    {
        return makeColor4(l.r - r.r, l.g - r.g, l.b - r.b, l.a - r.a);
    }

    Color4 operator*(const Color4& l, const Color4& r)
    {
        return makeColor4(l.r * r.r, l.g * r.g, l.b * r.b, l.a * r.a);
    }

    Color4 operator/(const Color4& l, const Color4& r)
    {
        return makeColor4(l.r / r.r, l.g / r.g, l.b / r.b, l.a / r.a);
    }


    void_t operator+=(Color4& l, const Color4& r)
    {
        l.r += r.r;
        l.g += r.g;
        l.b += r.b;
        l.a += r.a;
    }

    void_t operator-=(Color4& l, const Color4& r)
    {
        l.r -= r.r;
        l.g -= r.g;
        l.b -= r.b;
        l.a -= r.a;
    }

    void_t operator*=(Color4& l, const Color4& r)
    {
        l.r *= r.r;
        l.g *= r.g;
        l.b *= r.b;
        l.a *= r.a;
    }

    void_t operator/=(Color4& l, const Color4& r)
    {
        l.r /= r.r;
        l.g /= r.g;
        l.b /= r.b;
        l.a /= r.a;
    }


    Color4 operator+(const Color4& c, float32_t s)
    {
        return makeColor4(c.r + s, c.g + s, c.b + s, c.a + s);
    }

    Color4 operator-(const Color4& c, float32_t s)
    {
        return makeColor4(c.r - s, c.g - s, c.b - s, c.a - s);
    }

    Color4 operator*(const Color4& c, float32_t s)
    {
        return makeColor4(c.r * s, c.g * s, c.b * s, c.a * s);
    }

    Color4 operator/(const Color4& c, float32_t s)
    {
        return makeColor4(c.r / s, c.g / s, c.b / s, c.a / s);
    }


    void_t operator+=(Color4& c, float32_t s)
    {
        c.r += s;
        c.g += s;
        c.b += s;
        c.a += s;
    }

    void_t operator-=(Color4& c, float32_t s)
    {
        c.r -= s;
        c.g -= s;
        c.b -= s;
        c.a -= s;
    }

    void_t operator*=(Color4& c, float32_t s)
    {
        c.r *= s;
        c.g *= s;
        c.b *= s;
        c.a *= s;
    }

    void_t operator/=(Color4& c, float32_t s)
    {
        c.r /= s;
        c.g /= s;
        c.b /= s;
        c.a /= s;
    }


    bool_t operator==(const Color4& l, const Color4& r)
    {
        return equals(l, r);
    }

    bool_t operator!=(const Color4& l, const Color4& r)
    {
        return !equals(l, r);
    }
}  // namespace OMAF
