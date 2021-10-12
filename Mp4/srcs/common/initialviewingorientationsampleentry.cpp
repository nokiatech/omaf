
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
#include "initialviewingorientationsampleentry.hpp"

InitialViewingOrientation::InitialViewingOrientation()
    : SphereRegionSampleEntryBox("invo")
{
    auto& sphRegCfg = getSphereRegionConfig();
    sphRegCfg.setShapeType(SphereRegionConfigBox::ShapeType::FourGreatCircles);
    sphRegCfg.setDynamicRangeFlag(false);
    sphRegCfg.setStaticAzimuthRange(0);
    sphRegCfg.setStaticElevationRange(0);
}

InitialViewingOrientation* InitialViewingOrientation::clone() const
{
    return CUSTOM_NEW(InitialViewingOrientation, (*this));
}

void InitialViewingOrientation::writeBox(BitStream& bitstr)
{
    SphereRegionSampleEntryBox::writeBox(bitstr);
}

void InitialViewingOrientation::parseBox(BitStream& bitstr)
{
    SphereRegionSampleEntryBox::parseBox(bitstr);
}

InitialViewingOrientation::InitialViewingOrientationSample::InitialViewingOrientationSample()
    : SphereRegionSample()
{
}
