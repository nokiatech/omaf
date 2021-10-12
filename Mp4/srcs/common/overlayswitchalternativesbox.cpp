
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
#include "overlayswitchalternativesbox.hpp"

OverlaySwitchAlternativesBox::OverlaySwitchAlternativesBox()
    : EntityToGroupBox("oval", 0, 0)
{
}

void OverlaySwitchAlternativesBox::addRefOverlayId(uint16_t refOvlyId)
{
    mRefOverlayIds.push_back(refOvlyId);
}

uint16_t OverlaySwitchAlternativesBox::getRefOverlayId(uint32_t index) const
{
    return mRefOverlayIds.at(index);
}

void OverlaySwitchAlternativesBox::writeBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::writeBox(bitstream);
    if (mRefOverlayIds.size() != getEntityCount())
    {
        throw RuntimeError("Entity id count must match with reference overlay id count");
    }
    for (auto& ref : mRefOverlayIds)
    {
        bitstream.write16Bits(ref);
    }
    updateSize(bitstream);
}

void OverlaySwitchAlternativesBox::parseBox(ISOBMFF::BitStream& bitstream)
{
    EntityToGroupBox::parseBox(bitstream);
    for (std::uint32_t i = 0; i < getEntityCount(); i++)
    {
        mRefOverlayIds.push_back(bitstream.read16Bits());
    }
}
