
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

#include "Math/OMAFMathTypes.h"
#include "Math/OMAFMathConstants.h"

namespace OMAF
{
    static const Vector4 Vector4Zero = { 0.0f, 0.0f, 0.0f, 0.0f };
    static const Vector4 Vector4One = { 1.0f, 1.0f, 1.0f, 1.0f };

    static const Vector4 Vector4UnitX = { 1.0f, 0.0f, 0.0f, 0.0f };
    static const Vector4 Vector4UnitY = { 0.0f, 1.0f, 0.0f, 0.0f };
    static const Vector4 Vector4UnitZ = { 0.0f, 0.0f, 1.0f, 0.0f };
    static const Vector4 Vector4UnitW = { 0.0f, 0.0f, 0.0f, 1.0f };

    OMAF_INLINE Vector4 makeVector4(float32_t x, float32_t y, float32_t z, float32_t w);
    OMAF_INLINE Vector4 makeVector4(const float32_t v[4]);
    OMAF_INLINE Vector4 makeVector4(const Vector4& v);

    OMAF_INLINE Vector4 add(const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 subtract(const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 multiply(const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 divide(const Vector4& l, const Vector4& r);

    OMAF_INLINE void_t add(Vector4& result, const Vector4& l, const Vector4& r);
    OMAF_INLINE void_t subtract(Vector4& result, const Vector4& l, const Vector4& r);
    OMAF_INLINE void_t multiply(Vector4& result, const Vector4& l, const Vector4& r);
    OMAF_INLINE void_t divide(Vector4& result, const Vector4& l, const Vector4& r);

    OMAF_INLINE float32_t dotProduct(const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 crossProduct(const Vector4& l, const Vector4& r);

    OMAF_INLINE float32_t distance(const Vector4& l, const Vector4& r);
    OMAF_INLINE float32_t distanceSquared(const Vector4& l, const Vector4& r);

    OMAF_INLINE Vector4 scale(const Vector4& v, float32_t s);
    OMAF_INLINE Vector4 negate(const Vector4& v);
    OMAF_INLINE Vector4 normalize(const Vector4& v);

    OMAF_INLINE void_t scale(Vector4& result, const Vector4& v, float32_t s);
    OMAF_INLINE void_t negate(Vector4& result, const Vector4& v);
    OMAF_INLINE void_t normalize(Vector4& result, const Vector4& v);

    OMAF_INLINE float32_t length(const Vector4& v);
    OMAF_INLINE float32_t lengthSquared(const Vector4& v);

    OMAF_INLINE float32_t angle(const Vector4& l, const Vector4& r);
    OMAF_INLINE float32_t angleForNormals(const Vector4& l, const Vector4& r);

    OMAF_INLINE Vector4 orthogonal(const Vector4& v);
    OMAF_INLINE Vector4 reflect(const Vector4& v, const Vector4& n);

    OMAF_INLINE bool_t equals(const Vector4& l, const Vector4& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE Vector4 operator + (const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 operator - (const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 operator * (const Vector4& l, const Vector4& r);
    OMAF_INLINE Vector4 operator / (const Vector4& l, const Vector4& r);

    OMAF_INLINE void_t operator += (Vector4& l, const Vector4& r);
    OMAF_INLINE void_t operator -= (Vector4& l, const Vector4& r);
    OMAF_INLINE void_t operator *= (Vector4& l, const Vector4& r);
    OMAF_INLINE void_t operator /= (Vector4& l, const Vector4& r);

    OMAF_INLINE Vector4 operator + (const Vector4& v, float32_t s);
    OMAF_INLINE Vector4 operator - (const Vector4& v, float32_t s);
    OMAF_INLINE Vector4 operator * (const Vector4& v, float32_t s);
    OMAF_INLINE Vector4 operator / (const Vector4& v, float32_t s);

    OMAF_INLINE void_t operator += (Vector4& v, float32_t s);
    OMAF_INLINE void_t operator -= (Vector4& v, float32_t s);
    OMAF_INLINE void_t operator *= (Vector4& v, float32_t s);
    OMAF_INLINE void_t operator /= (Vector4& v, float32_t s);

    OMAF_INLINE bool_t operator == (const Vector4& l, const Vector4& r);
    OMAF_INLINE bool_t operator != (const Vector4& l, const Vector4& r);
}

namespace OMAF
{
    Vector4 makeVector4(float32_t x, float32_t y, float32_t z, float32_t w)
    {
        Vector4 result = { x, y, z, w };

        return result;
    }

    Vector4 makeVector4(const float32_t v[4])
    {
        Vector4 result = { v[0], v[1], v[2], v[3] };

        return result;
    }

    Vector4 makeVector4(const Vector4& v)
    {
        Vector4 result = v;

        return result;
    }


    Vector4 add(const Vector4& l, const Vector4& r)
    {
        Vector4 result;
        result.x = l.x + r.x;
        result.y = l.y + r.y;
        result.z = l.z + r.z;
        result.w = l.w + r.w;

        return result;
    }

    Vector4 subtract(const Vector4& l, const Vector4& r)
    {
        Vector4 result;
        result.x = l.x - r.x;
        result.y = l.y - r.y;
        result.z = l.z - r.z;
        result.w = l.w - r.w;

        return result;
    }

    Vector4 multiply(const Vector4& l, const Vector4& r)
    {
        Vector4 result;
        result.x = l.x * r.x;
        result.y = l.y * r.y;
        result.z = l.z * r.z;
        result.w = l.w * r.w;

        return result;
    }

    Vector4 divide(const Vector4& l, const Vector4& r)
    {
        Vector4 result;
        result.x = l.x / r.x;
        result.y = l.y / r.y;
        result.z = l.z / r.z;
        result.w = l.w / r.w;

        return result;
    }


    void_t add(Vector4& result, const Vector4& l, const Vector4& r)
    {
        result.x = l.x + r.x;
        result.y = l.y + r.y;
        result.z = l.z + r.z;
        result.w = l.w + r.w;
    }

    void_t subtract(Vector4& result, const Vector4& l, const Vector4& r)
    {
        result.x = l.x - r.x;
        result.y = l.y - r.y;
        result.z = l.z - r.z;
        result.w = l.w - r.w;
    }

    void_t multiply(Vector4& result, const Vector4& l, const Vector4& r)
    {
        result.x = l.x * r.x;
        result.y = l.y * r.y;
        result.z = l.z * r.z;
        result.w = l.w * r.w;
    }

    void_t divide(Vector4& result, const Vector4& l, const Vector4& r)
    {
        result.x = l.x / r.x;
        result.y = l.y / r.y;
        result.z = l.z / r.z;
        result.w = l.w / r.w;
    }


    float32_t dotProduct(const Vector4& l, const Vector4& r)
    {
        return (l.x * r.x) + (l.y * r.y) + (l.z * r.z) + (l.w * r.w);
    }

    Vector4 crossProduct(const Vector4& l, const Vector4& r)
    {
        Vector4 result;
        result.x = (l.y * r.z) - (l.z * r.y);
        result.y = (l.z * r.x) - (l.x * r.z);
        result.z = (l.x * r.y) - (l.y * r.x);
        result.w = 0.0f;

        return result;
    }


    float32_t distance(const Vector4& l, const Vector4& r)
    {
        Vector4 d = subtract(l, r);

        return length(d);
    }

    float32_t distanceSquared(const Vector4& l, const Vector4& r)
    {
        Vector4 d = subtract(l, r);

        return lengthSquared(d);
    }


    Vector4 scale(const Vector4& v, float32_t s)
    {
        Vector4 result;
        scale(result, v, s);

        return result;
    }

    Vector4 negate(const Vector4& v)
    {
        Vector4 result;
        negate(result, v);

        return result;
    }

    Vector4 normalize(const Vector4& v)
    {
        Vector4 result;
        normalize(result, v);

        return result;
    }

    void_t scale(Vector4& result, const Vector4& v, float32_t s)
    {
        result.x = v.x * s;
        result.y = v.y * s;
        result.z = v.z * s;
        result.w = v.w * s;
    }

    void_t negate(Vector4& result, const Vector4& v)
    {
        result.x = -v.x;
        result.y = -v.y;
        result.z = -v.z;
        result.w = -v.w;
    }

    void_t normalize(Vector4& result, const Vector4& v)
    {
        float32_t invLength = 1.0f / length(v);

        result.x = v.x * invLength;
        result.y = v.y * invLength;
        result.z = v.z * invLength;
        result.w = v.w * invLength;
    }


    float32_t length(const Vector4& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }

    float32_t lengthSquared(const Vector4& v)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z + v.w * v.w);
    }


    float32_t angle(const Vector4& l, const Vector4& r)
    {
        float32_t dot = dotProduct(l, r);
        float32_t lengthL = length(l);
        float32_t lengthR = length(r);
        float32_t div = dot / (lengthL * lengthR);

        return acosf(clamp(div, -1.0f, 1.0f));
    }

    float32_t angleForNormals(const Vector4& l, const Vector4& r)
    {
        float32_t dot = dotProduct(l, r);

        return acosf(clamp(dot, -1.0f, 1.0f));
    }


    Vector4 orthogonal(const Vector4& v)
    {
        // http://blog.selfshadow.com/2011/10/17/perp-vectors/

        Vector4 result;

        const float32_t epsilon = 0.99f; // Arbitrary epsilon

        if (fabsf(v.y) < epsilon) // Abs(DotProduct(u, UP))
        {
            // CrossProduct(v, UP)
            result.x = -v.z;
            result.y = 0.0f;
            result.z = v.x;
            result.w = 0.0f;
        }
        else
        {
            // CrossProduct(v, RIGHT)
            result.x = 0.0f;
            result.y = v.z;
            result.z = -v.y;
            result.w = 0.0f;
        }

        return result;
    }

    Vector4 reflect(const Vector4& v, const Vector4& n)
    {
        float32_t s = dotProduct(v, n);
        s = s * 2.0f;

        Vector4 result;
        result.x = v.x + n.x * s;
        result.y = v.y + n.y * s;
        result.z = v.z + n.z * s;
        result.w = v.w + n.w * s;

        return result;
    }


    bool_t equals(const Vector4& l, const Vector4& r, float32_t epsilon)
    {
        return (fabsf(l.x - r.x) <= epsilon &&
                fabsf(l.y - r.y) <= epsilon &&
                fabsf(l.z - r.z) <= epsilon &&
                fabsf(l.w - r.w) <= epsilon);
    }


    Vector4 operator + (const Vector4& l, const Vector4& r)
    {
        return add(l, r);
    }

    Vector4 operator - (const Vector4& l, const Vector4& r)
    {
        return subtract(l, r);
    }

    Vector4 operator * (const Vector4& l, const Vector4& r)
    {
        return multiply(l, r);
    }

    Vector4 operator / (const Vector4& l, const Vector4& r)
    {
        return divide(l, r);
    }


    void_t operator += (Vector4& l, const Vector4& r)
    {
        return add(l, l, r);
    }

    void_t operator -= (Vector4& l, const Vector4& r)
    {
        return subtract(l, l, r);
    }

    void_t operator *= (Vector4& l, const Vector4& r)
    {
        return multiply(l, l, r);
    }

    void_t operator /= (Vector4& l, const Vector4& r)
    {
        return divide(l, l, r);
    }


    Vector4 operator + (const Vector4& v, float32_t s)
    {
        Vector4 result;
        result.x = v.x + s;
        result.y = v.y + s;
        result.z = v.z + s;
        result.w = v.w + s;

        return result;
    }

    Vector4 operator - (const Vector4& v, float32_t s)
    {
        Vector4 result;
        result.x = v.x - s;
        result.y = v.y - s;
        result.z = v.z - s;
        result.w = v.w - s;

        return result;
    }

    Vector4 operator * (const Vector4& v, float32_t s)
    {
        Vector4 result;
        result.x = v.x * s;
        result.y = v.y * s;
        result.z = v.z * s;
        result.w = v.w * s;

        return result;
    }

    Vector4 operator / (const Vector4& v, float32_t s)
    {
        Vector4 result;
        result.x = v.x / s;
        result.y = v.y / s;
        result.z = v.z / s;
        result.w = v.w / s;

        return result;
    }


    void_t operator += (Vector4& v, float32_t s)
    {
        v.x += s;
        v.y += s;
        v.z += s;
        v.w += s;
    }

    void_t operator -= (Vector4& v, float32_t s)
    {
        v.x -= s;
        v.y -= s;
        v.z -= s;
        v.w -= s;
    }

    void_t operator *= (Vector4& v, float32_t s)
    {
        v.x *= s;
        v.y *= s;
        v.z *= s;
        v.w *= s;
    }

    void_t operator /= (Vector4& v, float32_t s)
    {
        v.x /= s;
        v.y /= s;
        v.z /= s;
        v.w /= s;
    }


    bool_t operator == (const Vector4& l, const Vector4& r)
    {
        return equals(l, r);
    }

    bool_t operator != (const Vector4& l, const Vector4& r)
    {
        return !equals(l, r);
    }
}
