
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

#include "Foundation/NVRFixedArray.h"
#include "NVREssentials.h"
#include "NVRNamespace.h"

OMAF_NS_BEGIN

struct ViewpointSwitching
{
    ViewpointSwitching()
        : transitionEffect(false)
        , switchRegion(false)
        , absoluteOffsetUsed(false)
        , absoluteOffset(0)
        , relativeOffset(0)
    {
    }

    uint32_t destinationViewpoint;
    // enum viewing orientation
    bool_t transitionEffect;
    bool_t switchRegion;

    bool_t absoluteOffsetUsed;
    uint32_t absoluteOffset;
    int32_t relativeOffset;
};

typedef FixedArray<ViewpointSwitching, 64> ViewpointSwitchingList;

OMAF_NS_END
