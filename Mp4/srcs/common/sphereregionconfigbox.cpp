
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
#include "sphereregionconfigbox.hpp"

SphereRegionConfigBox::SphereRegionConfigBox()
    : FullBox("rosc", 0, 0)
    , mSphereRegionConfig()
{
    mSphereRegionConfig.shapeType = ISOBMFF::SphereRegionShapeType::FourGreatCircles;
}

void SphereRegionConfigBox::setFrom(const ISOBMFF::SphereRegionConfigStruct& rosc)
{
    mSphereRegionConfig = rosc;
    setShapeType(static_cast<SphereRegionConfigBox::ShapeType>(rosc.shapeType));

    if (rosc.staticRange)
    {
        setDynamicRangeFlag(false);
        setStaticAzimuthRange(rosc.staticRange->azimuthRange);
        setStaticElevationRange(rosc.staticRange->elevationRange);
    }
}

ISOBMFF::SphereRegionConfigStruct SphereRegionConfigBox::get() const
{
    return mSphereRegionConfig;
}

void SphereRegionConfigBox::setShapeType(ShapeType shapeType)
{
    mSphereRegionConfig.shapeType = static_cast<ISOBMFF::SphereRegionShapeType>(shapeType);
}

SphereRegionConfigBox::ShapeType SphereRegionConfigBox::getShapeType()
{
    return static_cast<ShapeType>(mSphereRegionConfig.shapeType);
}

void SphereRegionConfigBox::setDynamicRangeFlag(bool rangeFlag)
{
    if (rangeFlag && mSphereRegionConfig.staticRange)
    {
        mSphereRegionConfig.staticRange.clear();
    }
    else if (!rangeFlag && !mSphereRegionConfig.staticRange)
    {
        mSphereRegionConfig.staticRange = ISOBMFF::SphereRegionRange();
    }
}

bool SphereRegionConfigBox::getDynamicRangeFlag()
{
    return !mSphereRegionConfig.staticRange;
}

void SphereRegionConfigBox::setStaticAzimuthRange(std::uint32_t range)
{
    mSphereRegionConfig.staticRange->azimuthRange = range;
}

std::uint32_t SphereRegionConfigBox::getStaticAzimuthRange()
{
    return mSphereRegionConfig.staticRange->azimuthRange;
}

void SphereRegionConfigBox::setStaticElevationRange(std::uint32_t range)
{
    mSphereRegionConfig.staticRange->elevationRange = range;
}

std::uint32_t SphereRegionConfigBox::getStaticElevationRange()
{
    return mSphereRegionConfig.staticRange->elevationRange;
}

void SphereRegionConfigBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    bitstr.write8Bits((uint8_t) getShapeType());
    bitstr.write8Bits(getDynamicRangeFlag() ? 0x1 : 0x0);
    if (!getDynamicRangeFlag())
    {
        bitstr.write32Bits(getStaticAzimuthRange());
        bitstr.write32Bits(getStaticElevationRange());
    }
    bitstr.write8Bits(1);  // numRegions

    updateSize(bitstr);
}

void SphereRegionConfigBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    setShapeType((ShapeType) bitstr.read8Bits());
    setDynamicRangeFlag(bitstr.read8Bits() & 0x1);
    if (!getDynamicRangeFlag())
    {
        setStaticAzimuthRange(bitstr.read32Bits());
        setStaticElevationRange(bitstr.read32Bits());
    }
    bitstr.read8Bits();  // numRegions
}
