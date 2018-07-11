
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
    , mNumRegions(1)
{
}

void SphereRegionConfigBox::setShapeType(ShapeType shapeType)
{
    mShapeType = shapeType;
}

SphereRegionConfigBox::ShapeType SphereRegionConfigBox::getShapeType()
{
    return mShapeType;
}

void SphereRegionConfigBox::setDynamicRangeFlag(bool rangeFlag)
{
    mDynamicRangeFlag = rangeFlag;
}

bool SphereRegionConfigBox::getDynamicRangeFlag()
{
    return mDynamicRangeFlag;
}

void SphereRegionConfigBox::setStaticAzimuthRange(std::uint32_t range)
{
    mStaticAzimuthRange = range;
}

std::uint32_t SphereRegionConfigBox::getStaticAzimuthRange()
{
    return mStaticAzimuthRange;
}

void SphereRegionConfigBox::setStaticElevationRange(std::uint32_t range)
{
    mStaticElevationRange = range;
}

std::uint32_t SphereRegionConfigBox::getStaticElevationRange()
{
    return mStaticElevationRange;
}

void SphereRegionConfigBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);

    bitstr.write8Bits((uint8_t) mShapeType);
    bitstr.write8Bits(mDynamicRangeFlag ? 0x1 : 0x0);
    if (!mDynamicRangeFlag)
    {
        bitstr.write32Bits(mStaticAzimuthRange);
        bitstr.write32Bits(mStaticElevationRange);
    }
    bitstr.write8Bits(mNumRegions);

    updateSize(bitstr);
}

void SphereRegionConfigBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mShapeType        = (ShapeType) bitstr.read8Bits();
    mDynamicRangeFlag = bitstr.read8Bits() & 0x1;
    if (!mDynamicRangeFlag)
    {
        mStaticAzimuthRange   = bitstr.read32Bits();
        mStaticElevationRange = bitstr.read32Bits();
    }
    mNumRegions = bitstr.read8Bits();
}
