
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

#include "Math/OMAFQuaternion.h"
#include "Math/OMAFVector3.h"
#include "Math/OMAFVector4.h"

namespace OMAF
{
    static const Matrix44 Matrix44Identity = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
                                              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};

    OMAF_INLINE Matrix44 makeMatrix44(float32_t m00,
                                      float32_t m01,
                                      float32_t m02,
                                      float32_t m03,
                                      float32_t m10,
                                      float32_t m11,
                                      float32_t m12,
                                      float32_t m13,
                                      float32_t m20,
                                      float32_t m21,
                                      float32_t m22,
                                      float32_t m23,
                                      float32_t m30,
                                      float32_t m31,
                                      float32_t m32,
                                      float32_t m33);
    OMAF_INLINE Matrix44 makeMatrix44(const float32_t m[16]);
    OMAF_INLINE Matrix44 makeMatrix44(const float32_t c[4][4]);
    OMAF_INLINE Matrix44 makeMatrix44(const Quaternion& q);

    OMAF_INLINE Matrix44 makeTranslation(float32_t x, float32_t y, float32_t z);
    OMAF_INLINE Matrix44 makeTranslation(const Vector3& v);

    OMAF_INLINE Matrix44 makeTranspose(const Matrix44& m);

    OMAF_INLINE Matrix44 makeScale(float32_t x, float32_t y, float32_t z);

    OMAF_INLINE Matrix44 makeRotationX(float32_t radians);
    OMAF_INLINE Matrix44 makeRotationY(float32_t radians);
    OMAF_INLINE Matrix44 makeRotationZ(float32_t radians);

    OMAF_INLINE Matrix44
    makeOrthographic(float32_t left, float32_t right, float32_t bottom, float32_t top, float32_t zNear, float32_t zFar);

    OMAF_INLINE Matrix44
    makeFrustum(float32_t left, float32_t right, float32_t bottom, float32_t top, float32_t zNear, float32_t zFar);
    OMAF_INLINE Matrix44 makeFrustum(float32_t horizontalFov, float32_t verticalFov, float32_t zNear, float32_t zFar);

    OMAF_INLINE Matrix44 makeLookAtRH(const Vector3& eyePosition,
                                      const Vector3& lookAtPosition,
                                      const Vector3& upDirection);

    OMAF_INLINE Matrix44
    makePerspectiveRH(float32_t fovL, float32_t fovR, float32_t fovB, float32_t fovT, float32_t zNear, float32_t zFar);
    OMAF_INLINE Matrix44 makePerspectiveRH(float32_t fovY, float32_t aspectRatioHW, float32_t zNear, float32_t zFar);

    OMAF_INLINE void_t multiply(Matrix44& result, const Matrix44& left, const Matrix44& right);
    OMAF_INLINE Matrix44 multiply(const Matrix44& left, const Matrix44& right);

    OMAF_INLINE Matrix44 invert(const Matrix44& m);

    OMAF_INLINE void_t rotate(Matrix44& result, const Quaternion& q);

    OMAF_INLINE Vector3 transform(const Matrix44& m, const Vector3& v);

    OMAF_INLINE Matrix44 operator+(const Matrix44& l, const Matrix44& r);
    OMAF_INLINE Matrix44 operator-(const Matrix44& l, const Matrix44& r);
    OMAF_INLINE Matrix44 operator*(const Matrix44& l, const Matrix44& r);

    OMAF_INLINE bool_t operator==(const Matrix44& l, const Matrix44& r);
    OMAF_INLINE bool_t operator!=(const Matrix44& l, const Matrix44& r);
}  // namespace OMAF

namespace OMAF
{
    Matrix44 makeMatrix44(float32_t m00,
                          float32_t m01,
                          float32_t m02,
                          float32_t m03,
                          float32_t m10,
                          float32_t m11,
                          float32_t m12,
                          float32_t m13,
                          float32_t m20,
                          float32_t m21,
                          float32_t m22,
                          float32_t m23,
                          float32_t m30,
                          float32_t m31,
                          float32_t m32,
                          float32_t m33)
    {
        Matrix44 result = {
            m00, m01, m02, m03,

            m10, m11, m12, m13,

            m20, m21, m22, m23,

            m30, m31, m32, m33,
        };

        return result;
    }

    Matrix44 makeMatrix44(const float32_t m[16])
    {
        Matrix44 result = {
            m[0],  m[1],  m[2],  m[3],

            m[4],  m[5],  m[6],  m[7],

            m[8],  m[9],  m[10], m[11],

            m[12], m[13], m[14], m[15],
        };

        return result;
    }

    Matrix44 makeMatrix44(const float32_t c[4][4])
    {
        Matrix44 result = {
            c[0][0], c[0][1], c[0][2], c[0][3],

            c[1][0], c[1][1], c[1][2], c[1][3],

            c[2][0], c[2][1], c[2][2], c[2][3],

            c[3][0], c[3][1], c[3][2], c[3][3],
        };

        return result;
    }

    Matrix44 makeMatrix44(const Quaternion& q)
    {
        // Quaternion to Matrix:
        // This is the arithmetical formula optimized to work with unit quaternions:
        // |1-2yy-2zz        2xy-2zw         2xz+2yw       0|
        // | 2xy+2zw        1-2xx-2zz        2yz-2xw       0|
        // | 2xz-2yw         2yz+2xw        1-2xx-2yy      0|
        // |    0               0               0          1|

        return makeMatrix44(
            // 1st column
            1 - 2 * (q.y * q.y + q.z * q.z), 2 * (q.x * q.y + q.z * q.w), 2 * (q.x * q.z - q.y * q.w),
            0

            // 2nd column
            ,
            2 * (q.x * q.y - q.z * q.w), 1 - 2 * (q.x * q.x + q.z * q.z), 2 * (q.z * q.y + q.x * q.w),
            0

            // 3rd column
            ,
            2 * (q.x * q.z + q.y * q.w), 2 * (q.y * q.z - q.x * q.w), 1 - 2 * (q.x * q.x + q.y * q.y),
            0

            // 4th column
            ,
            0, 0, 0, 1);
    }

    void_t multiply(Matrix44& result, const Matrix44& left, const Matrix44& right)
    {
        // First column
        result.m[0] =
            left.m[0] * right.m[0] + left.m[4] * right.m[1] + left.m[8] * right.m[2] + left.m[12] * right.m[3];
        result.m[1] =
            left.m[1] * right.m[0] + left.m[5] * right.m[1] + left.m[9] * right.m[2] + left.m[13] * right.m[3];
        result.m[2] =
            left.m[2] * right.m[0] + left.m[6] * right.m[1] + left.m[10] * right.m[2] + left.m[14] * right.m[3];
        result.m[3] =
            left.m[3] * right.m[0] + left.m[7] * right.m[1] + left.m[11] * right.m[2] + left.m[15] * right.m[3];

        // Second column
        result.m[4] =
            left.m[0] * right.m[4] + left.m[4] * right.m[5] + left.m[8] * right.m[6] + left.m[12] * right.m[7];
        result.m[5] =
            left.m[1] * right.m[4] + left.m[5] * right.m[5] + left.m[9] * right.m[6] + left.m[13] * right.m[7];
        result.m[6] =
            left.m[2] * right.m[4] + left.m[6] * right.m[5] + left.m[10] * right.m[6] + left.m[14] * right.m[7];
        result.m[7] =
            left.m[3] * right.m[4] + left.m[7] * right.m[5] + left.m[11] * right.m[6] + left.m[15] * right.m[7];

        // Third column
        result.m[8] =
            left.m[0] * right.m[8] + left.m[4] * right.m[9] + left.m[8] * right.m[10] + left.m[12] * right.m[11];
        result.m[9] =
            left.m[1] * right.m[8] + left.m[5] * right.m[9] + left.m[9] * right.m[10] + left.m[13] * right.m[11];
        result.m[10] =
            left.m[2] * right.m[8] + left.m[6] * right.m[9] + left.m[10] * right.m[10] + left.m[14] * right.m[11];
        result.m[11] =
            left.m[3] * right.m[8] + left.m[7] * right.m[9] + left.m[11] * right.m[10] + left.m[15] * right.m[11];

        // Fourth column
        result.m[12] =
            left.m[0] * right.m[12] + left.m[4] * right.m[13] + left.m[8] * right.m[14] + left.m[12] * right.m[15];
        result.m[13] =
            left.m[1] * right.m[12] + left.m[5] * right.m[13] + left.m[9] * right.m[14] + left.m[13] * right.m[15];
        result.m[14] =
            left.m[2] * right.m[12] + left.m[6] * right.m[13] + left.m[10] * right.m[14] + left.m[14] * right.m[15];
        result.m[15] =
            left.m[3] * right.m[12] + left.m[7] * right.m[13] + left.m[11] * right.m[14] + left.m[15] * right.m[15];
    }

    Matrix44 multiply(const Matrix44& left, const Matrix44& right)
    {
        Matrix44 result;
        multiply(result, left, right);

        return result;
    }

    Matrix44 invert(const Matrix44& m)
    {
        float32_t inverted[16];

        inverted[0] = m.m11 * m.m22 * m.m33 - m.m11 * m.m23 * m.m32 - m.m21 * m.m12 * m.m33 + m.m21 * m.m13 * m.m32 +
            m.m31 * m.m12 * m.m23 - m.m31 * m.m13 * m.m22;

        inverted[1] = -m.m01 * m.m22 * m.m33 + m.m01 * m.m23 * m.m32 + m.m21 * m.m02 * m.m33 - m.m21 * m.m03 * m.m32 -
            m.m31 * m.m02 * m.m23 + m.m31 * m.m03 * m.m22;

        inverted[2] = m.m01 * m.m12 * m.m33 - m.m01 * m.m13 * m.m32 - m.m11 * m.m02 * m.m33 + m.m11 * m.m03 * m.m32 +
            m.m31 * m.m02 * m.m13 - m.m31 * m.m03 * m.m12;

        inverted[3] = -m.m01 * m.m12 * m.m23 + m.m01 * m.m13 * m.m22 + m.m11 * m.m02 * m.m23 - m.m11 * m.m03 * m.m22 -
            m.m21 * m.m02 * m.m13 + m.m21 * m.m03 * m.m12;

        inverted[4] = -m.m10 * m.m22 * m.m33 + m.m10 * m.m23 * m.m32 + m.m20 * m.m12 * m.m33 - m.m20 * m.m13 * m.m32 -
            m.m30 * m.m12 * m.m23 + m.m30 * m.m13 * m.m22;

        inverted[5] = m.m00 * m.m22 * m.m33 - m.m00 * m.m23 * m.m32 - m.m20 * m.m02 * m.m33 + m.m20 * m.m03 * m.m32 +
            m.m30 * m.m02 * m.m23 - m.m30 * m.m03 * m.m22;

        inverted[6] = -m.m00 * m.m12 * m.m33 + m.m00 * m.m13 * m.m32 + m.m10 * m.m02 * m.m33 - m.m10 * m.m03 * m.m32 -
            m.m30 * m.m02 * m.m13 + m.m30 * m.m03 * m.m12;

        inverted[7] = m.m00 * m.m12 * m.m23 - m.m00 * m.m13 * m.m22 - m.m10 * m.m02 * m.m23 + m.m10 * m.m03 * m.m22 +
            m.m20 * m.m02 * m.m13 - m.m20 * m.m03 * m.m12;

        inverted[8] = m.m10 * m.m21 * m.m33 - m.m10 * m.m23 * m.m31 - m.m20 * m.m11 * m.m33 + m.m20 * m.m13 * m.m31 +
            m.m30 * m.m11 * m.m23 - m.m30 * m.m13 * m.m21;

        inverted[9] = -m.m00 * m.m21 * m.m33 + m.m00 * m.m23 * m.m31 + m.m20 * m.m01 * m.m33 - m.m20 * m.m03 * m.m31 -
            m.m30 * m.m01 * m.m23 + m.m30 * m.m03 * m.m21;

        inverted[10] = m.m00 * m.m11 * m.m33 - m.m00 * m.m13 * m.m31 - m.m10 * m.m01 * m.m33 + m.m10 * m.m03 * m.m31 +
            m.m30 * m.m01 * m.m13 - m.m30 * m.m03 * m.m11;

        inverted[11] = -m.m00 * m.m11 * m.m23 + m.m00 * m.m13 * m.m21 + m.m10 * m.m01 * m.m23 - m.m10 * m.m03 * m.m21 -
            m.m20 * m.m01 * m.m13 + m.m20 * m.m03 * m.m11;

        inverted[12] = -m.m10 * m.m21 * m.m32 + m.m10 * m.m22 * m.m31 + m.m20 * m.m11 * m.m32 - m.m20 * m.m12 * m.m31 -
            m.m30 * m.m11 * m.m22 + m.m30 * m.m12 * m.m21;

        inverted[13] = m.m00 * m.m21 * m.m32 - m.m00 * m.m22 * m.m31 - m.m20 * m.m01 * m.m32 + m.m20 * m.m02 * m.m31 +
            m.m30 * m.m01 * m.m22 - m.m30 * m.m02 * m.m21;

        inverted[14] = -m.m00 * m.m11 * m.m32 + m.m00 * m.m12 * m.m31 + m.m10 * m.m01 * m.m32 - m.m10 * m.m02 * m.m31 -
            m.m30 * m.m01 * m.m12 + m.m30 * m.m02 * m.m11;

        inverted[15] = m.m00 * m.m11 * m.m22 - m.m00 * m.m12 * m.m21 - m.m10 * m.m01 * m.m22 + m.m10 * m.m02 * m.m21 +
            m.m20 * m.m01 * m.m12 - m.m20 * m.m02 * m.m11;

        float32_t determinant = m.m00 * inverted[0] + m.m01 * inverted[4] + m.m02 * inverted[8] + m.m03 * inverted[12];

        determinant = 1.0f / determinant;

        Matrix44 result = {inverted[0] * determinant,  inverted[1] * determinant,
                           inverted[2] * determinant,  inverted[3] * determinant,

                           inverted[4] * determinant,  inverted[5] * determinant,
                           inverted[6] * determinant,  inverted[7] * determinant,

                           inverted[8] * determinant,  inverted[9] * determinant,
                           inverted[10] * determinant, inverted[11] * determinant,

                           inverted[12] * determinant, inverted[13] * determinant,
                           inverted[14] * determinant, inverted[15] * determinant};

        return result;
    }

    void_t rotate(Matrix44& result, const Quaternion& q)
    {
        // Quaternion to Matrix:
        // This is the arithmetical formula optimized to work with unit quaternions:
        // |1-2yy-2zz        2xy-2zw         2xz+2yw       0|
        // | 2xy+2zw        1-2xx-2zz        2yz-2xw       0|
        // | 2xz-2yw         2yz+2xw        1-2xx-2yy      0|
        // |    0               0               0          1|

        // 1st column
        result.m[0] = 1 - 2 * (q.y * q.y + q.z * q.z);
        result.m[1] = 2 * (q.x * q.y + q.z * q.w);
        result.m[2] = 2 * (q.x * q.z - q.y * q.w);
        result.m[3] = 0;

        // 2nd column
        result.m[4] = 2 * (q.x * q.y - q.z * q.w);
        result.m[5] = 1 - 2 * (q.x * q.x + q.z * q.z);
        result.m[6] = 2 * (q.z * q.y + q.x * q.w);
        result.m[7] = 0;

        // 3rd column
        result.m[8] = 2 * (q.x * q.z + q.y * q.w);
        result.m[9] = 2 * (q.y * q.z - q.x * q.w);
        result.m[10] = 1 - 2 * (q.x * q.x + q.y * q.y);
        result.m[11] = 0;

        // 4th column
        result.m[12] = 0;
        result.m[13] = 0;
        result.m[14] = 0;
        result.m[15] = 1;
    }

    Vector3 transform(const Matrix44& m, const Vector3& v)
    {
        return makeVector3(m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z, m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z,
                           m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z);
    }

    Matrix44
    makePerspectiveRH(float32_t fovL, float32_t fovR, float32_t fovB, float32_t fovT, float32_t zNear, float32_t zFar)
    {
        float32_t l = -tanf(toRadians(fovL)) * zNear;
        float32_t r = tanf(toRadians(fovR)) * zNear;
        float32_t b = -tanf(toRadians(fovB)) * zNear;
        float32_t t = tanf(toRadians(fovT)) * zNear;

        Matrix44 frustum = makeFrustum(l, r, b, t, zNear, zFar);

        return frustum;
    }


    Matrix44 makePerspectiveRH(float32_t fovY, float32_t aspectRatioHW, float32_t zNear, float32_t zFar)
    {
        float32_t cotan = 1.0f / tanf(fovY / 2.0f);

        Matrix44 result = {cotan / aspectRatioHW,
                           0.0f,
                           0.0f,
                           0.0f,

                           0.0f,
                           cotan,
                           0.0f,
                           0.0f,

                           0.0f,
                           0.0f,
                           (zFar + zNear) / (zNear - zFar),
                           -1.0f,

                           0.0f,
                           0.0f,
                           (2.0f * zFar * zNear) / (zNear - zFar),
                           0.0f};

        return result;
    }

    Matrix44
    makeFrustum(float32_t left, float32_t right, float32_t bottom, float32_t top, float32_t zNear, float32_t zFar)
    {
        float32_t ral = right + left;
        float32_t rsl = right - left;

        float32_t tab = top + bottom;
        float32_t tsb = top - bottom;

        float32_t fan = zFar + zNear;
        float32_t fsn = zFar - zNear;

        Matrix44 result = {2.0f * zNear / rsl,
                           0.0f,
                           0.0f,
                           0.0f,

                           0.0f,
                           2.0f * zNear / tsb,
                           0.0f,
                           0.0f,

                           ral / rsl,
                           tab / tsb,
                           -fan / fsn,
                           -1.0f,

                           0.0f,
                           0.0f,
                           (-2.0f * zFar * zNear) / fsn,
                           0.0f};

        return result;
    }

    Matrix44 makeFrustum(float32_t horizontalFov, float32_t verticalFov, float32_t zNear, float32_t zFar)
    {
        float32_t hHalf = zNear * tanf(horizontalFov * 0.5f);
        float32_t vHalf = zNear * tanf(verticalFov * 0.5f);

        return makeFrustum(-hHalf, hHalf, -vHalf, vHalf, zNear, zFar);
    }

    Matrix44 makeLookAtRH(const Vector3& eyePosition, const Vector3& lookAtPosition, const Vector3& upDirection)
    {
        Vector3 ev = eyePosition;
        Vector3 cv = lookAtPosition;
        Vector3 uv = upDirection;
        Vector3 n = normalize(ev - cv);
        Vector3 u = normalize(crossProduct(uv, n));
        Vector3 v = crossProduct(n, u);

        Matrix44 result = {u.v[0],
                           v.v[0],
                           n.v[0],
                           0.0f,

                           u.v[1],
                           v.v[1],
                           n.v[1],
                           0.0f,

                           u.v[2],
                           v.v[2],
                           n.v[2],
                           0.0f,

                           dotProduct(negate(u), ev),
                           dotProduct(negate(v), ev),
                           dotProduct(negate(n), ev),
                           1.0f};

        return result;
    }

    Matrix44 operator+(const Matrix44& l, const Matrix44& r)
    {
        Matrix44 result = {// First column
                           l.m[0] + r.m[0], l.m[1] + r.m[1], l.m[2] + r.m[2], l.m[3] + r.m[3],

                           // Second column
                           l.m[4] + r.m[4], l.m[5] + r.m[5], l.m[6] + r.m[6], l.m[7] + r.m[7],

                           // Third column
                           l.m[8] + r.m[8], l.m[9] + r.m[9], l.m[10] + r.m[10], l.m[11] + r.m[11],

                           // Fourth column
                           l.m[12] + r.m[12], l.m[13] + r.m[13], l.m[14] + r.m[14], l.m[15] + r.m[15]};

        return result;
    }

    Matrix44 operator-(const Matrix44& l, const Matrix44& r)
    {
        Matrix44 result = {// First column
                           l.m[0] - r.m[0], l.m[1] - r.m[1], l.m[2] - r.m[2], l.m[3] - r.m[3],

                           // Second column
                           l.m[4] - r.m[4], l.m[5] - r.m[5], l.m[6] - r.m[6], l.m[7] - r.m[7],

                           // Third column
                           l.m[8] - r.m[8], l.m[9] - r.m[9], l.m[10] - r.m[10], l.m[11] - r.m[11],

                           // Fourth column
                           l.m[12] - r.m[12], l.m[13] - r.m[13], l.m[14] - r.m[14], l.m[15] - r.m[15]};

        return result;
    }

    Matrix44 operator*(const Matrix44& l, const Matrix44& r)
    {
        Matrix44 result;

        multiply(result, l, r);

        return result;
    }


    Matrix44 makeTranslation(float32_t x, float32_t y, float32_t z)
    {
        Matrix44 result = Matrix44Identity;
        result.m[12] = x;
        result.m[13] = y;
        result.m[14] = z;

        return result;
    }

    Matrix44 makeTranslation(const Vector3& v)
    {
        Matrix44 result = Matrix44Identity;
        result.m[12] = v.x;
        result.m[13] = v.y;
        result.m[14] = v.z;

        return result;
    }

    Matrix44 makeTranspose(const Matrix44& m)
    {
        return makeMatrix44(m.m[0], m.m[4], m.m[8], m.m[12], m.m[1], m.m[5], m.m[9], m.m[13], m.m[2], m.m[6], m.m[10],
                            m.m[14], m.m[3], m.m[7], m.m[11], m.m[15]);
    }

    Matrix44 makeScale(float32_t x, float32_t y, float32_t z)
    {
        Matrix44 result = Matrix44Identity;
        result.m[0] *= x;
        result.m[5] *= y;
        result.m[10] *= z;

        return result;
    }

    Matrix44 makeRotationX(float32_t radians)
    {
        float32_t cos = cosf(radians);
        float32_t sin = sinf(radians);

        Matrix44 result = {1.0f, 0.0f, 0.0f, 0.0f,

                           0.0f, cos,  sin,  0.0f,

                           0.0f, -sin, cos,  0.0f,

                           0.0f, 0.0f, 0.0f, 1.0f};

        return result;
    }

    Matrix44 makeRotationY(float32_t radians)
    {
        float32_t cos = cosf(radians);
        float32_t sin = sinf(radians);

        Matrix44 result = {cos,  0.0f, -sin, 0.0f,

                           0.0f, 1.0f, 0.0f, 0.0f,

                           sin,  0.0f, cos,  0.0f,

                           0.0f, 0.0f, 0.0f, 1.0f};

        return result;
    }

    Matrix44 makeRotationZ(float32_t radians)
    {
        float32_t cos = cosf(radians);
        float32_t sin = sinf(radians);

        Matrix44 result = {cos,  sin,  0.0f, 0.0f,

                           -sin, cos,  0.0f, 0.0f,

                           0.0f, 0.0f, 1.0f, 0.0f,

                           0.0f, 0.0f, 0.0f, 1.0f};

        return result;
    }

    Matrix44
    makeOrthographic(float32_t left, float32_t right, float32_t bottom, float32_t top, float32_t zNear, float32_t zFar)
    {
        Matrix44 result = {2.0f / (right - left),
                           0.0f,
                           0.0f,
                           0.0f,

                           0.0f,
                           2.0f / (top - bottom),
                           0.0f,
                           0.0f,

                           0.0f,
                           0.0f,
                           -2.0f / (zFar - zNear),
                           0.0f,

                           -(right + left) / (right - left),
                           -(top + bottom) / (top - bottom),
                           -(zFar + zNear) / (zFar - zNear),
                           1.0f};

        return result;
    }
}  // namespace OMAF
