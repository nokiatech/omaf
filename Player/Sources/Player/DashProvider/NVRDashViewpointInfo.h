
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
#include "Metadata/NVRViewpointUserAction.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

namespace InitialViewpoint
{
    enum Enum
    {
        INVALID = -1,
        IS_INITIAL,
        IS_NOT_INITIAL,
        NOT_SPECIFIED,
        COUNT
    };
}

struct ViewpointGroupInfo
{
    uint8_t groupId;
    const char_t* description;
};

struct MpdViewpointInfo
{
    MpdViewpointInfo()
        : id(OMAF_NULL)
        , label(OMAF_NULL)
        , isInitial(InitialViewpoint::NOT_SPECIFIED)
        , groupInfo()
        , position()
        , gpsPosition()
        , geoMagneticInfo()
    {
    }
    const char_t* id;  // from the main level
    const char_t* label;
    InitialViewpoint::Enum isInitial;
    ViewpointGroupInfo groupInfo;
    ViewpointPosition position;
    ViewpointGpsPosition gpsPosition;
    ViewpointGeomagneticInfo geoMagneticInfo;
    ViewpointSwitchRegion switchRegion;
};


typedef FixedArray<MpdViewpointInfo, 64> Viewpoints;

OMAF_NS_END
