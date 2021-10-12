
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
#include "overlayandbackgroundgroupingbox.hpp"

OverlayAndBackgroundGroupingBox::OverlayAndBackgroundGroupingBox()
    : EntityToGroupBox("ovbg", 0, 0)
    , mSphereDistanceInMm(0)
{
}

uint32_t OverlayAndBackgroundGroupingBox::getSphereDistanceInMm() const
{
    return mSphereDistanceInMm;
}

void OverlayAndBackgroundGroupingBox::setSphereDistanceInMm(uint32_t mm)
{
    mSphereDistanceInMm = mm;
}

void OverlayAndBackgroundGroupingBox::addGroupingFlags(OverlayAndBackgroundGroupingBox::GroupingFlags flags)
{
    mEntityFlags.push_back(flags);
}

OverlayAndBackgroundGroupingBox::GroupingFlags OverlayAndBackgroundGroupingBox::getGroupingFlags(uint32_t index) const
{
    return mEntityFlags.at(index);
}

void OverlayAndBackgroundGroupingBox::writeBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::writeBox(bitstream);
    bitstream.write32Bits(mSphereDistanceInMm);
    if (mEntityFlags.size() != getEntityCount())
    {
        throw RuntimeError("Entity id count must match with entity flags count");
    }
    for (auto& flag : mEntityFlags)
    {
        bitstream.write8Bits(flag.flagsAsByte());
        if (flag.overlaySubset)
        {
            bitstream.write16Bits(flag.overlaySubset->size());
            for (auto& id : *flag.overlaySubset)
            {
                bitstream.write16Bits(id);
            }
        }
    }
    updateSize(bitstream);
}

void OverlayAndBackgroundGroupingBox::parseBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::parseBox(bitstream);
    mSphereDistanceInMm = bitstream.read32Bits();
    for (uint32_t i = 0; i < getEntityCount(); i++)
    {
        auto entityFlags = GroupingFlags(bitstream.read8Bits());
        if (entityFlags.overlaySubset)
        {
            auto numIds = bitstream.read16Bits();
            for (size_t n = 0; n < numIds; ++n)
            {
                entityFlags.overlaySubset->push_back(bitstream.read16Bits());
            }
        }
        mEntityFlags.push_back(entityFlags);
    }
}
