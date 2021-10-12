
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

/*
 * Data structures that are relevant for representing the viewpoint for user interaction
 */

struct ViewpointPosition
{
    ViewpointPosition()
        : exists(false)
    {
    }
    bool_t exists;
    int32_t posX;
    int32_t posY;
    int32_t posZ;
};

struct ViewpointGpsPosition
{
    ViewpointGpsPosition()
        : exists(false)
    {
    }
    bool_t exists;
    int32_t longitude;  // -180*2^23 ... 180*2^23-1
    int32_t latitude;
    int32_t altitude;
};

struct ViewpointGeomagneticInfo
{
    ViewpointGeomagneticInfo()
        : exists(false)
    {
    }
    bool_t exists;
    int32_t yaw;  // -180*65536 ... 180*65536-1
    int32_t pitch;
    int32_t roll;
};

namespace ViewpointSwitchRegionType
{
    enum Enum
    {
        INVALID = -1,
        VIEWPORT_POSITION,
        SPHERE_REGION,
        OVERLAY,
        COUNT
    };
}

struct SphereRegion
{
    int32_t cAzimuth;
    int32_t cElevation;
    int32_t cTilt;
    uint32_t rAzimuth;
    uint32_t rElevation;
};

struct ViewportPosition
{
    uint16_t rectLeftPercent;
    uint16_t rectTopPercent;
    uint16_t rectWidthPercent;
    uint16_t rectHeightPercent;
};

struct ViewpointSwitchRegion
{
    ViewpointSwitchRegionType::Enum type;
    union {
        ViewportPosition viewportPosition;
        SphereRegion sphereRegion;
        uint16_t refOverlayId;
    };
};

struct ViewpointSwitchControl
{
    uint32_t destinationViewpointId;
    ViewpointSwitchRegion viewpointSwitchRegion;
    bool_t switchActivationStart;
    uint32_t switchActivationStartTime;
    bool_t switchActivationEnd;
    uint32_t switchActivationEndTime;
};

typedef FixedArray<ViewpointSwitchControl, 64> ViewpointSwitchControls;

OMAF_NS_END
