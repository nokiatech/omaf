
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

namespace OMAF
{
    OMAF_INLINE Size makeSize(float32_t width, float32_t height);

    OMAF_INLINE bool_t equals(const Size& l, const Size& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);

    OMAF_INLINE bool_t operator==(const Size& l, const Size& r);
    OMAF_INLINE bool_t operator!=(const Size& l, const Size& r);
}  // namespace OMAF

namespace OMAF
{
    Size makeSize(float32_t width, float32_t height)
    {
        Size result = {width, height};

        return result;
    }

    bool_t equals(const Size& l, const Size& r, float32_t epsilon)
    {
        return (fabsf(l.width - r.width) <= epsilon && fabsf(l.height - r.height) <= epsilon);
    }

    bool_t operator==(const Size& l, const Size& r)
    {
        return equals(l, r);
    }

    bool_t operator!=(const Size& l, const Size& r)
    {
        return !equals(l, r);
    }
}  // namespace OMAF
