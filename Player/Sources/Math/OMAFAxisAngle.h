
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
    OMAF_INLINE AxisAngle makeAxisAngle(float32_t x, float32_t y, float32_t z, float32_t angle);
    OMAF_INLINE AxisAngle makeAxisAngle(const Vector3& axis, float32_t angle);

    OMAF_INLINE bool_t equals(const AxisAngle& l, const AxisAngle& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE bool_t operator==(const AxisAngle& l, const AxisAngle& r);
    OMAF_INLINE bool_t operator!=(const AxisAngle& l, const AxisAngle& r);
}  // namespace OMAF

namespace OMAF
{
    AxisAngle makeAxisAngle(float32_t x, float32_t y, float32_t z, float32_t angle)
    {
        AxisAngle result = {x, y, z, angle};

        return result;
    }

    AxisAngle makeAxisAngle(const Vector3& axis, float32_t angle)
    {
        AxisAngle result = {axis, angle};

        return result;
    }

    bool_t equals(const AxisAngle& l, const AxisAngle& r, float32_t epsilon)
    {
        return (fabsf(l.axis.x - r.axis.x) <= epsilon && fabsf(l.axis.y - r.axis.y) <= epsilon &&
                fabsf(l.axis.z - r.axis.z) <= epsilon && fabsf(l.angle - r.angle) <= epsilon);
    }


    bool_t operator==(const AxisAngle& l, const AxisAngle& r)
    {
        return equals(l, r);
    }

    bool_t operator!=(const AxisAngle& l, const AxisAngle& r)
    {
        return !equals(l, r);
    }
}  // namespace OMAF
