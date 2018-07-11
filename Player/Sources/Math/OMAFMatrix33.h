
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

namespace OMAF
{
    static const Matrix33 Matrix33Identity =
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f
    };

    OMAF_INLINE Matrix33 makeMatrix33(float32_t m00, float32_t m01, float32_t m02,
                                     float32_t m10, float32_t m11, float32_t m12,
                                     float32_t m20, float32_t m21, float32_t m22);

    OMAF_INLINE Matrix33 makeMatrix33(const float32_t m[9]);
    OMAF_INLINE Matrix33 makeMatrix33(const float32_t c[3][3]);
}

namespace OMAF
{
    Matrix33 makeMatrix33(float32_t m00, float32_t m01, float32_t m02,
                          float32_t m10, float32_t m11, float32_t m12,
                          float32_t m20, float32_t m21, float32_t m22)
    {
        Matrix33 result =
        {
            m00,
            m01,
            m02,

            m10,
            m11,
            m12,

            m20,
            m21,
            m22,
        };

        return result;
    }

    Matrix33 makeMatrix33(const float32_t m[9])
    {
        Matrix33 result =
        {
            m[0],
            m[1],
            m[2],

            m[3],
            m[4],
            m[5],

            m[6],
            m[7],
            m[8],
        };

        return result;
    }

    Matrix33 makeMatrix33(const float32_t c[3][3])
    {
        Matrix33 result =
        {
            c[0][0],
            c[0][1],
            c[0][2],

            c[1][0],
            c[1][1],
            c[1][2],

            c[2][0],
            c[2][1],
            c[2][2],
        };

        return result;
    }
}
