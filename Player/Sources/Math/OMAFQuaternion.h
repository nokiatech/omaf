
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
#include "Math/OMAFMathTypes.h"

#include <cassert>
#include "Math/OMAFVector3.h"

namespace OMAF
{
    static const Quaternion QuaternionIdentity = {0.0f, 0.0f, 0.0f, 1.0f};

    OMAF_INLINE Quaternion makeQuaternion(float32_t x, float32_t y, float32_t z, float32_t w);
    OMAF_INLINE Quaternion makeQuaternion(float32_t x, float32_t y, float32_t z, EulerAxisOrder::Enum order);
    OMAF_INLINE Quaternion makeQuaternion(const Vector3& axis, float32_t angle);
    OMAF_INLINE Quaternion makeQuaternion(const Matrix33& rotationMatrix);
    OMAF_INLINE Quaternion makeQuaternion(const Matrix44& m);
    OMAF_INLINE Quaternion makeQuaternion(const Vector3& forward, const Vector3& up);

    OMAF_INLINE void_t concatenate(Quaternion& result, const Quaternion& l, const Quaternion& r);
    OMAF_INLINE void_t invert(Quaternion& result, const Quaternion& q);
    OMAF_INLINE void_t normalize(Quaternion& result, const Quaternion& q);

    OMAF_INLINE Quaternion concatenate(const Quaternion& l, const Quaternion& r);
    OMAF_INLINE void_t invert(Quaternion& q);
    OMAF_INLINE void_t normalize(Quaternion& q);

    OMAF_INLINE void_t conjugate(Quaternion& result, const Quaternion& q);
    OMAF_INLINE Quaternion conjugate(const Quaternion& q);

    OMAF_INLINE float32_t length(const Quaternion& quaternion);

    OMAF_INLINE Vector3 rotate(const Quaternion& q, const Vector3& v);

    OMAF_INLINE void_t
    eulerAngles(const Quaternion& q, float32_t& yaw, float32_t& pitch, float32_t& roll, EulerAxisOrder::Enum order);
    OMAF_INLINE float32_t yaw(const OMAF::Quaternion& q);

    OMAF_INLINE Quaternion operator+(const Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion operator-(const Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion operator*(const Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion operator/(const Quaternion& l, const Quaternion& r);

    OMAF_INLINE Quaternion& operator+=(Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion& operator-=(Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion& operator*=(Quaternion& l, const Quaternion& r);
    OMAF_INLINE Quaternion& operator/=(Quaternion& l, const Quaternion& r);
}  // namespace OMAF

namespace OMAF
{
    Quaternion makeQuaternion(float32_t x, float32_t y, float32_t z, float32_t w)
    {
        Quaternion result = {x, y, z, w};

        return result;
    }

    Quaternion makeQuaternion(const Vector3& axis, float32_t angle)
    {
        float32_t s = sinf(angle * 0.5f);
        float32_t c = cosf(angle * 0.5f);

        return makeQuaternion(axis.x * s, axis.y * s, axis.z * s, c);
    }

    Quaternion makeQuaternion(float32_t x, float32_t y, float32_t z, EulerAxisOrder::Enum order)
    {
        float c1 = cosf(x * 0.5f);
        float c2 = cosf(y * 0.5f);
        float c3 = cosf(z * 0.5f);

        float s1 = sinf(x * 0.5f);
        float s2 = sinf(y * 0.5f);
        float s3 = sinf(z * 0.5f);

        Quaternion result;

        if (order == EulerAxisOrder::XYZ)
        {
            result.x = s1 * c2 * c3 + c1 * s2 * s3;
            result.y = c1 * s2 * c3 - s1 * c2 * s3;
            result.z = c1 * c2 * s3 + s1 * s2 * c3;
            result.w = c1 * c2 * c3 - s1 * s2 * s3;
        }
        else if (order == EulerAxisOrder::YXZ)
        {
            result.x = s1 * c2 * c3 + c1 * s2 * s3;
            result.y = c1 * s2 * c3 - s1 * c2 * s3;
            result.z = c1 * c2 * s3 - s1 * s2 * c3;
            result.w = c1 * c2 * c3 + s1 * s2 * s3;
        }
        else if (order == EulerAxisOrder::ZXY)
        {
            result.x = s1 * c2 * c3 - c1 * s2 * s3;
            result.y = c1 * s2 * c3 + s1 * c2 * s3;
            result.z = c1 * c2 * s3 + s1 * s2 * c3;
            result.w = c1 * c2 * c3 - s1 * s2 * s3;
        }
        else if (order == EulerAxisOrder::ZYX)
        {
            result.x = s1 * c2 * c3 - c1 * s2 * s3;
            result.y = c1 * s2 * c3 + s1 * c2 * s3;
            result.z = c1 * c2 * s3 - s1 * s2 * c3;
            result.w = c1 * c2 * c3 + s1 * s2 * s3;
        }
        else if (order == EulerAxisOrder::YZX)
        {
            result.x = s1 * c2 * c3 + c1 * s2 * s3;
            result.y = c1 * s2 * c3 + s1 * c2 * s3;
            result.z = c1 * c2 * s3 - s1 * s2 * c3;
            result.w = c1 * c2 * c3 - s1 * s2 * s3;
        }
        else if (order == EulerAxisOrder::XZY)
        {
            result.x = s1 * c2 * c3 - c1 * s2 * s3;
            result.y = c1 * s2 * c3 - s1 * c2 * s3;
            result.z = c1 * c2 * s3 + s1 * s2 * c3;
            result.w = c1 * c2 * c3 + s1 * s2 * s3;
        }

        return result;
    }

    Quaternion makeQuaternion(const Matrix33& rotationMatrix)
    {
        Quaternion q;

        const float32_t* a = rotationMatrix.m;

        float32_t t = a[0] + a[4] + a[8];

        if (t > 0.00000001f)
        {
            float32_t r = sqrtf(1 + t);
            q.w = 0.5f * r;
            q.x = 0.5f * (a[7] - a[5]) / r;
            q.y = 0.5f * (a[2] - a[6]) / r;
            q.z = 0.5f * (a[3] - a[1]) / r;
        }
        else if (a[0] > a[4] && a[0] > a[8])
        {
            float32_t r = sqrtf(1 + a[0] - a[4] - a[8]);
            q.w = 0.5f * (a[7] - a[5]) / r;
            q.x = 0.5f * r;
            q.y = 0.5f * (a[1] + a[3]) / r;
            q.z = 0.5f * (a[6] + a[2]) / r;
        }
        else if (a[4] > a[8])
        {
            float32_t r = sqrtf(1 - a[0] + a[4] - a[8]);
            q.w = 0.5f * (a[2] - a[6]) / r;
            q.x = 0.5f * (a[1] + a[3]) / r;
            q.y = 0.5f * r;
            q.z = 0.5f * (a[5] + a[7]) / r;
        }
        else
        {
            float32_t r = sqrtf(1 - a[0] - a[4] + a[8]);
            q.w = 0.5f * (a[3] - a[1]) / r;
            q.x = 0.5f * (a[2] + a[6]) / r;
            q.y = 0.5f * (a[5] + a[7]) / r;
            q.z = 0.5f * r;
        }

        normalize(q);

        return q;
    }

    Quaternion makeQuaternion(const Matrix44& m)
    {
        float32_t t = m.m[0] + m.m[5] + m.m[10];
        float32_t x, y, z, w;

        if (t >= 0.0f)
        {
            float32_t s = sqrtf(t + 1.0f);
            w = s * 0.5f;
            s = 0.5f / s;

            x = (m.m[6] - m.m[9]) * s;
            y = (m.m[8] - m.m[2]) * s;
            z = (m.m[1] - m.m[4]) * s;
        }
        else if ((m.m[0] > m.m[5]) && (m.m[0] > m.m[10]))
        {
            float32_t s = sqrtf(1.0f + m.m[0] - m.m[5] - m.m[10]);
            x = s * 0.5f;
            s = 0.5f / s;

            y = (m.m[4] + m.m[1]) * s;
            z = (m.m[2] + m.m[8]) * s;
            w = (m.m[6] - m.m[9]) * s;
        }
        else if (m.m[5] > m.m[10])
        {
            float32_t s = sqrtf(1.0f + m.m[5] - m.m[0] - m.m[10]);
            y = s * 0.5f;
            s = 0.5f / s;

            x = (m.m[4] + m.m[1]) * s;
            z = (m.m[9] + m.m[6]) * s;
            w = (m.m[8] - m.m[2]) * s;
        }
        else
        {
            float32_t s = sqrtf(1.0f + m.m[10] - m.m[0] - m.m[5]);
            z = s * 0.5f;
            s = 0.5f / s;

            x = (m.m[2] + m.m[8]) * s;
            y = (m.m[9] + m.m[6]) * s;
            w = (m.m[1] - m.m[4]) * s;
        }

        float32_t l = sqrtf(x * x + y * y + z * z + w * w);

        return makeQuaternion(x / l, y / l, z / l, w / l);
    }

    void_t concatenate(Quaternion& result, const Quaternion& l, const Quaternion& r)
    {
        Quaternion tmp;
        tmp.x = (l.w * r.x) + (l.x * r.w) + (l.y * r.z) - (l.z * r.y);
        tmp.y = (l.w * r.y) - (l.x * r.z) + (l.y * r.w) + (l.z * r.x);
        tmp.z = (l.w * r.z) + (l.x * r.y) - (l.y * r.x) + (l.z * r.w);
        tmp.w = (l.w * r.w) - (l.x * r.x) - (l.y * r.y) - (l.z * r.z);

        result = tmp;
    }

    void_t invert(Quaternion& result, const Quaternion& q)
    {
        float32_t scale = 1.0f / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

        result.x = -q.x * scale;
        result.y = -q.y * scale;
        result.z = -q.z * scale;
        result.w = q.w * scale;
    }

    void_t normalize(Quaternion& result, const Quaternion& q)
    {
        float32_t invlength = 1.0f / sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

        result.x *= invlength;
        result.y *= invlength;
        result.z *= invlength;
        result.w *= invlength;
    }

    Quaternion concatenate(const Quaternion& l, const Quaternion& r)
    {
        Quaternion result;
        concatenate(result, l, r);

        return result;
    }

    void_t invert(Quaternion& q)
    {
        float32_t scale = 1.0f / (q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);

        q.x = -q.x * scale;
        q.y = -q.y * scale;
        q.z = -q.z * scale;
        q.w = q.w * scale;
    }

    void_t normalize(Quaternion& q)
    {
        normalize(q, q);
    }

    void_t conjugate(Quaternion& result, const Quaternion& q)
    {
        result.x = -q.x;
        result.y = -q.y;
        result.z = -q.z;
        result.w = q.w;
    }

    Quaternion conjugate(const Quaternion& q)
    {
        Quaternion result;

        conjugate(result, q);

        return result;
    }

    float32_t length(const Quaternion& q)
    {
        return sqrtf(q.w * q.w + q.x * q.x + q.y * q.y + q.z * q.z);
    }

    Vector3 rotate(const Quaternion& q, const Vector3& v)
    {
        float32_t xx = q.x * q.x;
        float32_t yy = q.y * q.y;
        float32_t zz = q.z * q.z;

        float32_t xy = q.x * q.y;
        float32_t xz = q.x * q.z;
        float32_t yz = q.y * q.z;

        float32_t wx = q.w * q.x;
        float32_t wy = q.w * q.y;
        float32_t wz = q.w * q.z;

        return makeVector3((1.0f - 2.0f * (yy + zz)) * v.x + (2.0f * (xy - wz)) * v.y + (2.0f * (xz + wy)) * v.z,
                           (2.0f * (xy + wz)) * v.x + (1.0f - 2.0f * (xx + zz)) * v.y + (2.0f * (yz - wx)) * v.z,
                           (2.0f * (xz - wy)) * v.x + (2.0f * (yz + wx)) * v.y + (1.0f - 2.0f * (xx + yy)) * v.z);
    }

    float32_t yaw(const Quaternion& q)
    {
        float32_t y = 2.0f * (q.w * q.x - q.y * q.z);
        float32_t yaw = (fabsf(y) < (1.0f - OMAF_FLOAT32_EPSILON))
            ? atan2f(2.0f * (q.x * q.z + q.w * q.y), 1.0f - 2.0f * (q.x * q.x + q.y * q.y))
            : 0.0f;

        return yaw;
    }

    void_t
    eulerAngles(const Quaternion& q, float32_t& yaw, float32_t& pitch, float32_t& roll, EulerAxisOrder::Enum order)
    {
        if (order == EulerAxisOrder::XYZ)
        {
            assert(false);
        }
        else if (order == EulerAxisOrder::YXZ)
        {
            yaw = atan2f((2.0f * (q.x * q.z + q.w * q.y)), (q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z));
            pitch = asinf(clamp((-2.0f * (q.y * q.z - q.w * q.x)), -1.0f, 1.0f));
            roll = atan2f((2.0f * (q.x * q.y + q.w * q.z)), (q.w * q.w - q.x * q.x + q.y * q.y - q.z * q.z));
        }
        else if (order == EulerAxisOrder::ZXY)
        {
            assert(false);
        }
        else if (order == EulerAxisOrder::ZYX)
        {
            assert(false);
        }
        else if (order == EulerAxisOrder::YZX)
        {
            assert(false);
        }
        else if (order == EulerAxisOrder::XZY)
        {
            assert(false);
        }
    }

    Quaternion operator+(const Quaternion& l, const Quaternion& r)
    {
        return makeQuaternion(l.x + r.x, l.y + r.y, l.z + r.z, l.w + r.w);
    }

    Quaternion operator-(const Quaternion& l, const Quaternion& r)
    {
        return makeQuaternion(l.x - r.x, l.y - r.y, l.z - r.z, l.w - r.w);
    }

    Quaternion operator*(const Quaternion& l, const Quaternion& r)
    {
        float32_t ww = (l.z + l.x) * (r.x + r.y);
        float32_t yy = (l.w - l.y) * (r.w + r.z);
        float32_t zz = (l.w + l.y) * (r.w - r.z);
        float32_t xx = ww + yy + zz;
        float32_t qq = 0.5f * (xx + (l.z - l.x) * (r.x - r.y));

        float32_t w = qq - ww + (l.z - l.y) * (r.y - r.z);
        float32_t x = qq - xx + (l.x + l.w) * (r.x + r.w);
        float32_t y = qq - yy + (l.w - l.x) * (r.y + r.z);
        float32_t z = qq - zz + (l.z + l.y) * (r.w - r.x);

        return makeQuaternion(x, y, z, w);
    }

    Quaternion& operator+=(Quaternion& l, const Quaternion& r)
    {
        l.x += r.x;
        l.y += r.y;
        l.z += r.z;
        l.w += r.w;

        return l;
    }

    Quaternion& operator-=(Quaternion& l, const Quaternion& r)
    {
        l.x -= r.x;
        l.y -= r.y;
        l.z -= r.z;
        l.w -= r.w;

        return l;
    }

    Quaternion& operator*=(Quaternion& l, const Quaternion& r)
    {
        float32_t ww = (l.z + l.x) * (r.x + r.y);
        float32_t yy = (l.w - l.y) * (r.w + r.z);
        float32_t zz = (l.w + l.y) * (r.w - r.z);
        float32_t xx = ww + yy + zz;
        float32_t qq = 0.5f * (xx + (l.z - l.x) * (r.x - r.y));

        float32_t w = qq - ww + (l.z - l.y) * (r.y - r.z);
        float32_t x = qq - xx + (l.x + l.w) * (r.x + r.w);
        float32_t y = qq - yy + (l.w - l.x) * (r.y + r.z);
        float32_t z = qq - zz + (l.z + l.y) * (r.w - r.x);

        l.w = w;
        l.x = x;
        l.y = y;
        l.z = z;

        return l;
    }
}  // namespace OMAF
