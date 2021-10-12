
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

#include "Math/OMAFMathConstants.h"
#include "Math/OMAFMathFunctions.h"
#include "Math/OMAFMathTypes.h"

namespace OMAF
{
    static const Vector2 Vector2Zero = {0.0f, 0.0f};
    static const Vector2 Vector2One = {1.0f, 1.0f};

    static const Vector2 Vector2UnitX = {1.0f, 0.0f};
    static const Vector2 Vector2UnitY = {0.0f, 1.0f};

    OMAF_INLINE Vector2 makeVector2(float32_t x, float32_t y);
    OMAF_INLINE Vector2 makeVector2(const float32_t v[2]);
    OMAF_INLINE Vector2 makeVector2(const Vector2& v);

    OMAF_INLINE Vector2 add(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 subtract(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 multiply(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 divide(const Vector2& l, const Vector2& r);

    OMAF_INLINE void_t add(Vector2& result, const Vector2& l, const Vector2& r);
    OMAF_INLINE void_t subtract(Vector2& result, const Vector2& l, const Vector2& r);
    OMAF_INLINE void_t multiply(Vector2& result, const Vector2& l, const Vector2& r);
    OMAF_INLINE void_t divide(Vector2& result, const Vector2& l, const Vector2& r);

    OMAF_INLINE float32_t dotProduct(const Vector2& l, const Vector2& r);
    OMAF_INLINE float32_t crossProduct(const Vector2& l, const Vector2& r);

    OMAF_INLINE float32_t distance(const Vector2& l, const Vector2& r);
    OMAF_INLINE float32_t distanceSquared(const Vector2& l, const Vector2& r);

    OMAF_INLINE Vector2 scale(const Vector2& v, float32_t s);
    OMAF_INLINE Vector2 negate(const Vector2& v);
    OMAF_INLINE Vector2 normalize(const Vector2& v);
    OMAF_INLINE Vector2 rotate(const Vector2& v, float32_t angleRad);

    OMAF_INLINE void_t scale(Vector2& result, const Vector2& v, float32_t s);
    OMAF_INLINE void_t negate(Vector2& result, const Vector2& v);
    OMAF_INLINE void_t normalize(Vector2& result, const Vector2& v);
    OMAF_INLINE void_t rotate(Vector2& result, const Vector2& v, float32_t angleRad);

    OMAF_INLINE float32_t length(const Vector2& v);
    OMAF_INLINE float32_t lengthSquared(const Vector2& v);

    OMAF_INLINE float32_t angle(const Vector2& l, const Vector2& r);
    OMAF_INLINE float32_t angleForNormals(const Vector2& l, const Vector2& r);

    OMAF_INLINE Vector2 orthogonal(const Vector2& v);
    OMAF_INLINE Vector2 reflect(const Vector2& v, const Vector2& n);

    OMAF_INLINE bool_t equals(const Vector2& l, const Vector2& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE Vector2 operator+(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 operator-(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 operator*(const Vector2& l, const Vector2& r);
    OMAF_INLINE Vector2 operator/(const Vector2& l, const Vector2& r);

    OMAF_INLINE void_t operator+=(Vector2& l, const Vector2& r);
    OMAF_INLINE void_t operator-=(Vector2& l, const Vector2& r);
    OMAF_INLINE void_t operator*=(Vector2& l, const Vector2& r);
    OMAF_INLINE void_t operator/=(Vector2& l, const Vector2& r);

    OMAF_INLINE Vector2 operator+(const Vector2& v, float32_t s);
    OMAF_INLINE Vector2 operator-(const Vector2& v, float32_t s);
    OMAF_INLINE Vector2 operator*(const Vector2& v, float32_t s);
    OMAF_INLINE Vector2 operator/(const Vector2& v, float32_t s);

    OMAF_INLINE void_t operator+=(Vector2& v, float32_t s);
    OMAF_INLINE void_t operator-=(Vector2& v, float32_t s);
    OMAF_INLINE void_t operator*=(Vector2& v, float32_t s);
    OMAF_INLINE void_t operator/=(Vector2& v, float32_t s);

    OMAF_INLINE bool_t operator==(const Vector2& l, const Vector2& r);
    OMAF_INLINE bool_t operator!=(const Vector2& l, const Vector2& r);
}  // namespace OMAF

namespace OMAF
{
    Vector2 makeVector2(float32_t x, float32_t y)
    {
        Vector2 result = {x, y};

        return result;
    }

    Vector2 makeVector2(const float32_t v[2])
    {
        Vector2 result = {v[0], v[1]};

        return result;
    }

    Vector2 makeVector2(const Vector2& v)
    {
        Vector2 result = v;

        return result;
    }


    Vector2 add(const Vector2& l, const Vector2& r)
    {
        Vector2 result;
        result.x = l.x + r.x;
        result.y = l.y + r.y;

        return result;
    }

    Vector2 subtract(const Vector2& l, const Vector2& r)
    {
        Vector2 result;
        result.x = l.x - r.x;
        result.y = l.y - r.y;

        return result;
    }

    Vector2 multiply(const Vector2& l, const Vector2& r)
    {
        Vector2 result;
        result.x = l.x * r.x;
        result.y = l.y * r.y;

        return result;
    }

    Vector2 divide(const Vector2& l, const Vector2& r)
    {
        Vector2 result;
        result.x = l.x / r.x;
        result.y = l.y / r.y;

        return result;
    }


    void_t add(Vector2& result, const Vector2& l, const Vector2& r)
    {
        result.x = l.x + r.x;
        result.y = l.y + r.y;
    }

    void_t subtract(Vector2& result, const Vector2& l, const Vector2& r)
    {
        result.x = l.x - r.x;
        result.y = l.y - r.y;
    }

    void_t multiply(Vector2& result, const Vector2& l, const Vector2& r)
    {
        result.x = l.x * r.x;
        result.y = l.y * r.y;
    }

    void_t divide(Vector2& result, const Vector2& l, const Vector2& r)
    {
        result.x = l.x / r.x;
        result.y = l.y / r.y;
    }


    float32_t dotProduct(const Vector2& l, const Vector2& r)
    {
        return (l.x * r.x) + (l.y * r.y);
    }

    float32_t crossProduct(const Vector2& l, const Vector2& r)
    {
        return (l.x * r.y) - (l.y * r.x);
    }


    float32_t distance(const Vector2& l, const Vector2& r)
    {
        Vector2 d = subtract(l, r);

        return length(d);
    }

    float32_t distanceSquared(const Vector2& l, const Vector2& r)
    {
        Vector2 d = subtract(l, r);

        return lengthSquared(d);
    }


    Vector2 scale(const Vector2& v, float32_t s)
    {
        Vector2 result;
        scale(result, v, s);

        return result;
    }

    Vector2 negate(const Vector2& v)
    {
        Vector2 result;
        negate(result, v);

        return result;
    }

    Vector2 normalize(const Vector2& v)
    {
        Vector2 result;
        normalize(result, v);

        return result;
    }

    Vector2 rotate(const Vector2& v, float32_t angleRad)
    {
        float32_t s = sinf(angleRad);
        float32_t c = cosf(angleRad);

        Vector2 result = {v.x * c - v.y * s, v.x * s + v.y * c};

        return result;
    }

    void_t scale(Vector2& result, const Vector2& v, float32_t s)
    {
        result.x = v.x * s;
        result.y = v.y * s;
    }

    void_t negate(Vector2& result, const Vector2& v)
    {
        result.x = -v.x;
        result.y = -v.y;
    }

    void_t normalize(Vector2& result, const Vector2& v)
    {
        float32_t invLength = 1.0f / length(v);

        result.x = v.x * invLength;
        result.y = v.y * invLength;
    }

    void_t rotate(Vector2& result, const Vector2& v, float32_t angleRad)
    {
        float32_t s = sinf(angleRad);
        float32_t c = cosf(angleRad);

        float32_t x = v.x * c - v.y * s;

        result.y = v.x * s + v.y * c;
        result.x = x;
    }

    float32_t length(const Vector2& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y);
    }

    float32_t lengthSquared(const Vector2& v)
    {
        return (v.x * v.x + v.y * v.y);
    }


    float32_t angle(const Vector2& l, const Vector2& r)
    {
        float32_t dot = dotProduct(l, r);
        float32_t lengthL = length(l);
        float32_t lengthR = length(r);
        float32_t div = dot / (lengthL * lengthR);

        return acosf(clamp(div, -1.0f, 1.0f));
    }

    float32_t angleForNormals(const Vector2& l, const Vector2& r)
    {
        float32_t dot = dotProduct(l, r);

        return acosf(clamp(dot, -1.0f, 1.0f));
    }


    Vector2 orthogonal(const Vector2& v)
    {
        Vector2 result;
        result.x = -v.y;
        result.y = v.x;

        return result;
    }

    Vector2 reflect(const Vector2& v, const Vector2& n)
    {
        float32_t s = dotProduct(v, n);
        s = s * 2.0f;

        Vector2 result;
        result.x = v.x + n.x * s;
        result.y = v.y + n.y * s;

        return result;
    }


    bool_t equals(const Vector2& l, const Vector2& r, float32_t epsilon)
    {
        return (fabsf(l.x - r.x) <= epsilon && fabsf(l.y - r.y) <= epsilon);
    }


    Vector2 operator+(const Vector2& l, const Vector2& r)
    {
        return add(l, r);
    }

    Vector2 operator-(const Vector2& l, const Vector2& r)
    {
        return subtract(l, r);
    }

    Vector2 operator*(const Vector2& l, const Vector2& r)
    {
        return multiply(l, r);
    }

    Vector2 operator/(const Vector2& l, const Vector2& r)
    {
        return divide(l, r);
    }


    void_t operator+=(Vector2& l, const Vector2& r)
    {
        return add(l, l, r);
    }

    void_t operator-=(Vector2& l, const Vector2& r)
    {
        return subtract(l, l, r);
    }

    void_t operator*=(Vector2& l, const Vector2& r)
    {
        return multiply(l, l, r);
    }

    void_t operator/=(Vector2& l, const Vector2& r)
    {
        return divide(l, l, r);
    }


    Vector2 operator+(const Vector2& v, float32_t s)
    {
        Vector2 result;
        result.x = v.x + s;
        result.y = v.y + s;

        return result;
    }

    Vector2 operator-(const Vector2& v, float32_t s)
    {
        Vector2 result;
        result.x = v.x - s;
        result.y = v.y - s;

        return result;
    }

    Vector2 operator*(const Vector2& v, float32_t s)
    {
        Vector2 result;
        result.x = v.x * s;
        result.y = v.y * s;

        return result;
    }

    Vector2 operator/(const Vector2& v, float32_t s)
    {
        Vector2 result;
        result.x = v.x / s;
        result.y = v.y / s;

        return result;
    }


    void_t operator+=(Vector2& v, float32_t s)
    {
        v.x += s;
        v.y += s;
    }

    void_t operator-=(Vector2& v, float32_t s)
    {
        v.x -= s;
        v.y -= s;
    }

    void_t operator*=(Vector2& v, float32_t s)
    {
        v.x *= s;
        v.y *= s;
    }

    void_t operator/=(Vector2& v, float32_t s)
    {
        v.x /= s;
        v.y /= s;
    }


    bool_t operator==(const Vector2& l, const Vector2& r)
    {
        return equals(l, r);
    }

    bool_t operator!=(const Vector2& l, const Vector2& r)
    {
        return !equals(l, r);
    }
}  // namespace OMAF
