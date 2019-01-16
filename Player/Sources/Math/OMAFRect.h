
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
    OMAF_INLINE Rect makeRect(float32_t x, float32_t y, float32_t w, float32_t h);
    OMAF_INLINE Rect makeRect(const Point& origin, const Size& size);
    
    OMAF_INLINE bool_t equals(const Rect& l, const Rect& r, float32_t epsilon = OMAF_FLOAT32_EPSILON);
    OMAF_INLINE bool_t isInside(const Rect& rect, const OMAF::Vector2& point);

    OMAF_INLINE bool_t operator == (const Rect& l, const Rect& r);
    OMAF_INLINE bool_t operator != (const Rect& l, const Rect& r);
}

namespace OMAF
{
    Rect makeRect(float32_t x, float32_t y, float32_t w, float32_t h)
    {
        Rect result = { x, y, w, h };
        
        return result;
    }
    
    Rect makeRect(const Point& origin, const Size& size)
    {
        Rect result = { origin, size };
        
        return result;
    }
    
    bool_t equals(const Rect& l, const Rect& r, float32_t epsilon)
    {
        return (fabsf(l.x - r.x) <= epsilon &&
                fabsf(l.y - r.y) <= epsilon &&
                fabsf(l.w - r.w) <= epsilon &&
                fabsf(l.h - r.h) <= epsilon);
    }

    bool_t isInside(const Rect& rect, const OMAF::Vector2& point)
    {
        return point.x >= rect.x &&
               point.x <= (rect.x + rect.w) &&
               point.y >= rect.y &&
               point.y <= (rect.y + rect.h);
    }
    
    
    bool_t operator == (const Rect& l, const Rect& r)
    {
        return equals(l, r);
    }
    
    bool_t operator != (const Rect& l, const Rect& r)
    {
        return !equals(l, r);
    }
}
