
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
#ifndef COMMONTYPES_HPP
#define COMMONTYPES_HPP

#include <cstdint>

enum class ViewIdcType : std::uint8_t
{
    MONOSCOPIC     = 0,
    LEFT           = 1,
    RIGHT          = 2,
    LEFT_AND_RIGHT = 3,
    INVALID        = 0xff
};


struct SphereRegion
{
    std::int32_t centreAzimuth;
    std::int32_t centreElevation;
    std::int32_t centreTilt;
    std::uint32_t azimuthRange;    // not always used
    std::uint32_t elevationRange;  // not always used
    bool interpolate;
};

#endif
