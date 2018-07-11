
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

#include "Math/OMAFMathTypes.h"
#include "Math/OMAFMathConstants.h"
#include "Math/OMAFMathFunctions.h"

namespace OMAF
{
    static const Vector3 Vector3Zero = { 0.0f, 0.0f, 0.0f };
    static const Vector3 Vector3One = { 1.0f, 1.0f, 1.0f };

    static const Vector3 Vector3UnitX = { 1.0f, 0.0f, 0.0f };
    static const Vector3 Vector3UnitY = { 0.0f, 1.0f, 0.0f };
    static const Vector3 Vector3UnitZ = { 0.0f, 0.0f, 1.0f };

    static const Vector3 Vector3Front = { 0.0f, 0.0f, -1.0f };
    static const Vector3 Vector3Back = { 0.0f, 0.0f, 1.0f };
    static const Vector3 Vector3Up = { 0.0f, 1.0f, 0.0f };
    static const Vector3 Vector3Down = { 0.0f, -1.0f, 0.0f };
    static const Vector3 Vector3Left = { -1.0f, 0.0f, 0.0f };
    static const Vector3 Vector3Right = { 1.0f, 0.0f, 0.0f };

    OMAF_INLINE Vector3 makeVector3(float32_t x, float32_t y, float32_t z);
    OMAF_INLINE Vector3 makeVector3(const float32_t v[3]);
    OMAF_INLINE Vector3 makeVector3(const Vector3& v);

    OMAF_INLINE Vector3 add(const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 subtract(const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 multiply(const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 divide(const Vector3& l, const Vector3& r);

    OMAF_INLINE void_t add(Vector3& result, const Vector3& l, const Vector3& r);
    OMAF_INLINE void_t subtract(Vector3& result, const Vector3& l, const Vector3& r);
    OMAF_INLINE void_t multiply(Vector3& result, const Vector3& l, const Vector3& r);
    OMAF_INLINE void_t divide(Vector3& result, const Vector3& l, const Vector3& r);

    OMAF_INLINE float32_t dotProduct(const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 crossProduct(const Vector3& l, const Vector3& r);

    OMAF_INLINE float32_t distance(const Vector3& l, const Vector3& r);
    OMAF_INLINE float32_t distanceSquared(const Vector3& l, const Vector3& r);

    OMAF_INLINE Vector3 scale(const Vector3& v, float32_t s);
    OMAF_INLINE Vector3 negate(const Vector3& v);
    OMAF_INLINE Vector3 normalize(const Vector3& v);

    OMAF_INLINE void_t scale(Vector3& result, const Vector3& v, float32_t s);
    OMAF_INLINE void_t negate(Vector3& result, const Vector3& v);
    OMAF_INLINE void_t normalize(Vector3& result, const Vector3& v);

    OMAF_INLINE float32_t length(const Vector3& v);
    OMAF_INLINE float32_t lengthSquared(const Vector3& v);

    OMAF_INLINE float32_t angle(const Vector3& l, const Vector3& r);
    OMAF_INLINE float32_t angleForNormals(const Vector3& l, const Vector3& r);

    OMAF_INLINE Vector3 orthogonal(const Vector3& v);
    OMAF_INLINE Vector3 reflect(const Vector3& v, const Vector3& n);
    OMAF_INLINE Vector3 normal(const Vector3& a, const Vector3& b, const Vector3& c);

    OMAF_INLINE bool_t equals(const Vector3& l, const Vector3& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE Vector3 operator + (const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 operator - (const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 operator * (const Vector3& l, const Vector3& r);
    OMAF_INLINE Vector3 operator / (const Vector3& l, const Vector3& r);

    OMAF_INLINE void_t operator += (Vector3& l, const Vector3& r);
    OMAF_INLINE void_t operator -= (Vector3& l, const Vector3& r);
    OMAF_INLINE void_t operator *= (Vector3& l, const Vector3& r);
    OMAF_INLINE void_t operator /= (Vector3& l, const Vector3& r);

    OMAF_INLINE Vector3 operator + (const Vector3& v, float32_t s);
    OMAF_INLINE Vector3 operator - (const Vector3& v, float32_t s);
    OMAF_INLINE Vector3 operator * (const Vector3& v, float32_t s);
    OMAF_INLINE Vector3 operator / (const Vector3& v, float32_t s);

    OMAF_INLINE void_t operator += (Vector3& v, float32_t s);
    OMAF_INLINE void_t operator -= (Vector3& v, float32_t s);
    OMAF_INLINE void_t operator *= (Vector3& v, float32_t s);
    OMAF_INLINE void_t operator /= (Vector3& v, float32_t s);

    OMAF_INLINE bool_t operator == (const Vector3& l, const Vector3& r);
    OMAF_INLINE bool_t operator != (const Vector3& l, const Vector3& r);
}

namespace OMAF
{
    Vector3 makeVector3(float32_t x, float32_t y, float32_t z)
    {
        Vector3 result = { x, y, z };

        return result;
    }

    Vector3 makeVector3(const float32_t v[3])
    {
        Vector3 result = { v[0], v[1], v[2] };

        return result;
    }

    Vector3 makeVector3(const Vector3& v)
    {
        Vector3 result = v;

        return result;
    }


    Vector3 add(const Vector3& l, const Vector3& r)
    {
        Vector3 result;
        result.x = l.x + r.x;
        result.y = l.y + r.y;
        result.z = l.z + r.z;

        return result;
    }

    Vector3 subtract(const Vector3& l, const Vector3& r)
    {
        Vector3 result;
        result.x = l.x - r.x;
        result.y = l.y - r.y;
        result.z = l.z - r.z;

        return result;
    }

    Vector3 multiply(const Vector3& l, const Vector3& r)
    {
        Vector3 result;
        result.x = l.x * r.x;
        result.y = l.y * r.y;
        result.z = l.z * r.z;

        return result;
    }

    Vector3 divide(const Vector3& l, const Vector3& r)
    {
        Vector3 result;
        result.x = l.x / r.x;
        result.y = l.y / r.y;
        result.z = l.z / r.z;

        return result;
    }


    void_t add(Vector3& result, const Vector3& l, const Vector3& r)
    {
        result.x = l.x + r.x;
        result.y = l.y + r.y;
        result.z = l.z + r.z;
    }

    void_t subtract(Vector3& result, const Vector3& l, const Vector3& r)
    {
        result.x = l.x - r.x;
        result.y = l.y - r.y;
        result.z = l.z - r.z;
    }

    void_t multiply(Vector3& result, const Vector3& l, const Vector3& r)
    {
        result.x = l.x * r.x;
        result.y = l.y * r.y;
        result.z = l.z * r.z;
    }

    void_t divide(Vector3& result, const Vector3& l, const Vector3& r)
    {
        result.x = l.x / r.x;
        result.y = l.y / r.y;
        result.z = l.z / r.z;
    }


    float32_t dotProduct(const Vector3& l, const Vector3& r)
    {
        return (l.x * r.x) + (l.y * r.y) + (l.z * r.z);
    }

    Vector3 crossProduct(const Vector3& l, const Vector3& r)
    {
        Vector3 result;
        result.x = (l.y * r.z) - (l.z * r.y);
        result.y = (l.z * r.x) - (l.x * r.z);
        result.z = (l.x * r.y) - (l.y * r.x);

        return result;
    }


    float32_t distance(const Vector3& l, const Vector3& r)
    {
        Vector3 d = subtract(l, r);

        return length(d);
    }

    float32_t distanceSquared(const Vector3& l, const Vector3& r)
    {
        Vector3 d = subtract(l, r);

        return lengthSquared(d);
    }


    Vector3 scale(const Vector3& v, float32_t s)
    {
        Vector3 result;
        scale(result, v, s);

        return result;
    }

    Vector3 negate(const Vector3& v)
    {
        Vector3 result;
        negate(result, v);

        return result;
    }

    Vector3 normalize(const Vector3& v)
    {
        Vector3 result;
        normalize(result, v);

        return result;
    }

    void_t scale(Vector3& result, const Vector3& v, float32_t s)
    {
        result.x = v.x * s;
        result.y = v.y * s;
        result.z = v.z * s;
    }

    void_t negate(Vector3& result, const Vector3& v)
    {
        result.x = -v.x;
        result.y = -v.y;
        result.z = -v.z;
    }

    void_t normalize(Vector3& result, const Vector3& v)
    {
        float32_t invLength = 1.0f / length(v);

        result.x = v.x * invLength;
        result.y = v.y * invLength;
        result.z = v.z * invLength;
    }


    float32_t length(const Vector3& v)
    {
        return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    float32_t lengthSquared(const Vector3& v)
    {
        return (v.x * v.x + v.y * v.y + v.z * v.z);
    }


    float32_t angle(const Vector3& l, const Vector3& r)
    {
        float32_t dot = dotProduct(l, r);
        float32_t lengthL = length(l);
        float32_t lengthR = length(r);
        float32_t div = dot / (lengthL * lengthR);

        return acosf(clamp(div, -1.0f, 1.0f));
    }

    float32_t angleForNormals(const Vector3& l, const Vector3& r)
    {
        float32_t dot = dotProduct(l, r);

        return acosf(clamp(dot, -1.0f, 1.0f));
    }


    Vector3 orthogonal(const Vector3& v)
    {
        // http://blog.selfshadow.com/2011/10/17/perp-vectors/

        Vector3 result;

        const float32_t epsilon = 0.99f; // Arbitrary epsilon

        if (fabsf(v.y) < epsilon) // Abs(DotProduct(u, UP))
        {
            // CrossProduct(v, UP)
            result.x = -v.z;
            result.y = 0.0f;
            result.z = v.x;
        }
        else
        {
            // CrossProduct(v, RIGHT)
            result.x = 0.0f;
            result.y = v.z;
            result.z = -v.y;
        }

        return result;
    }

    Vector3 reflect(const Vector3& v, const Vector3& n)
    {
        float32_t s = dotProduct(v, n);
        s = s * 2.0f;

        Vector3 result;
        result.x = v.x + n.x * s;
        result.y = v.y + n.y * s;
        result.z = v.z + n.z * s;

        return result;
    }

    Vector3 normal(const Vector3& a, const Vector3& b, const Vector3& c)
    {
        Vector3 ab = subtract(b, a);
        Vector3 ca = subtract(c, a);

        Vector3 n = crossProduct(ab, ca);
        normalize(n, n);

        return n;
    }


    bool_t equals(const Vector3& l, const Vector3& r, float32_t epsilon)
    {
        return (fabsf(l.x - r.x) <= epsilon &&
                fabsf(l.y - r.y) <= epsilon &&
                fabsf(l.z - r.z) <= epsilon);
    }


    Vector3 operator + (const Vector3& l, const Vector3& r)
    {
        return add(l, r);
    }

    Vector3 operator - (const Vector3& l, const Vector3& r)
    {
        return subtract(l, r);
    }

    Vector3 operator * (const Vector3& l, const Vector3& r)
    {
        return multiply(l, r);
    }

    Vector3 operator / (const Vector3& l, const Vector3& r)
    {
        return divide(l, r);
    }


    void_t operator += (Vector3& l, const Vector3& r)
    {
        return add(l, l, r);
    }

    void_t operator -= (Vector3& l, const Vector3& r)
    {
        return subtract(l, l, r);
    }

    void_t operator *= (Vector3& l, const Vector3& r)
    {
        return multiply(l, l, r);
    }

    void_t operator /= (Vector3& l, const Vector3& r)
    {
        return divide(l, l, r);
    }


    Vector3 operator + (const Vector3& v, float32_t s)
    {
        Vector3 result;
        result.x = v.x + s;
        result.y = v.y + s;
        result.z = v.z + s;

        return result;
    }

    Vector3 operator - (const Vector3& v, float32_t s)
    {
        Vector3 result;
        result.x = v.x - s;
        result.y = v.y - s;
        result.z = v.z - s;

        return result;
    }

    Vector3 operator * (const Vector3& v, float32_t s)
    {
        Vector3 result;
        result.x = v.x * s;
        result.y = v.y * s;
        result.z = v.z * s;

        return result;
    }

    Vector3 operator / (const Vector3& v, float32_t s)
    {
        Vector3 result;
        result.x = v.x / s;
        result.y = v.y / s;
        result.z = v.z / s;

        return result;
    }


    void_t operator += (Vector3& v, float32_t s)
    {
        v.x += s;
        v.y += s;
        v.z += s;
    }

    void_t operator -= (Vector3& v, float32_t s)
    {
        v.x -= s;
        v.y -= s;
        v.z -= s;
    }

    void_t operator *= (Vector3& v, float32_t s)
    {
        v.x *= s;
        v.y *= s;
        v.z *= s;
    }

    void_t operator /= (Vector3& v, float32_t s)
    {
        v.x /= s;
        v.y /= s;
        v.z /= s;
    }


    bool_t operator == (const Vector3& l, const Vector3& r)
    {
        return equals(l, r);
    }

    bool_t operator != (const Vector3& l, const Vector3& r)
    {
        return !equals(l, r);
    }
}
