
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

#include "Platform/OMAFDataTypes.h"

namespace OMAF
{
    struct Vector2
    {
        union {
            struct
            {
                float32_t x, y;
            };

            float32_t v[2];
        };
    };

    struct Vector3
    {
        union {
            struct
            {
                float32_t x, y, z;
            };
            struct
            {
                float32_t r, g, b;
            };
            struct
            {
                float32_t s, t, p;
            };

            float32_t v[3];
        };
    };

    struct Vector4
    {
        union {
            struct
            {
                float32_t x, y, z, w;
            };
            struct
            {
                float32_t r, g, b, a;
            };
            struct
            {
                float32_t s, t, p, q;
            };

            float32_t v[4];
        };
    };

    struct Matrix33
    {
        union {
            struct
            {
                float32_t m00, m01, m02;
                float32_t m10, m11, m12;
                float32_t m20, m21, m22;
            };

            float32_t m[9];
            float32_t c[3][3];
        };
    };

    struct Matrix44
    {
        union {
            struct
            {
                // Column-major indexing (column, row)
                float32_t m00, m01, m02, m03;  // column #1
                float32_t m10, m11, m12, m13;  // column #2
                float32_t m20, m21, m22, m23;  // column #3
                float32_t m30, m31, m32, m33;  // column #4
            };

            struct
            {
                Vector4 c0;
                Vector4 c1;
                Vector4 c2;
                Vector4 c3;
            };

            float32_t m[16];
            float32_t c[4][4];
        };
    };

    struct Quaternion
    {
        union {
            struct
            {
                Vector3 v;
                float32_t s;
            };
            struct
            {
                float32_t x, y, z, w;
            };

            float32_t q[4];
        };
    };

    struct EulerAngle
    {
        float32_t yaw;
        float32_t pitch;
        float32_t roll;
    };

    struct AxisAngle
    {
        Vector3 axis;
        float32_t angle;
    };

    struct Point
    {
        float32_t x;
        float32_t y;
    };

    struct Size
    {
        float32_t width;
        float32_t height;
    };

    struct Rect
    {
        union {
            struct
            {
                Point origin;
                Size size;
            };
            // value range 0..1
            struct
            {
                float32_t x, y, w, h;
            };
        };
    };

    struct Color3
    {
        float32_t r, g, b;
    };

    struct Color4
    {
        float32_t r, g, b, a;
    };

    struct EulerAxisOrder
    {
        enum Enum
        {
            INVALID = -1,

            XYZ,
            YXZ,
            ZXY,

            ZYX,
            YZX,
            XZY,

            COUNT
        };
    };
}  // namespace OMAF
