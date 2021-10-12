
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

#include "NVREssentials.h"
#include "NVRNamespace.h"

OMAF_NS_BEGIN

struct ViewpointGlobalCoordinateSysRotation
{
    ViewpointGlobalCoordinateSysRotation()
        : yaw(0)
        , pitch(0)
        , roll(0){};

    int32_t yaw;
    int32_t pitch;
    int32_t roll;
};

OMAF_NS_END
