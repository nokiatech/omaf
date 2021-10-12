
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
#include "overlaysampleentrybox.hpp"
#include "customallocator.hpp"

void OverlaySampleEntryBox::OverlayConfigSample::write(ISOBMFF::BitStream& bitstr) const
{
    bitstr.write16Bits((uint16_t)(activeOverlayIds.size() << 1) | (!!addlActiveOverlays ? 1 : 0));
    for (auto activeOverlayId : activeOverlayIds)
    {
        bitstr.write16Bits(activeOverlayId);
    }
    if (addlActiveOverlays)
    {
        addlActiveOverlays->write(bitstr);
    }
}

void OverlaySampleEntryBox::OverlayConfigSample::parse(ISOBMFF::BitStream& bitstr)
{
    auto firstWord              = bitstr.read16Bits();
    auto activatedOverlaysCount = firstWord >> 1;
    bool addlActiveOverlaysFlag = firstWord & 0x1;  // last bit tells it
    for (std::int32_t i = 0; i < activatedOverlaysCount; i++)
    {
        activeOverlayIds.push_back(bitstr.read16Bits());
    }
    if (addlActiveOverlaysFlag)
    {
        addlActiveOverlays = OverlayStruct();
        addlActiveOverlays->parse(bitstr);
    }
    else
    {
        addlActiveOverlays = {};
    }
}

OverlaySampleEntryBox::OverlaySampleEntryBox()
    : MetaDataSampleEntryBox("dyol")
{
}

OverlaySampleEntryBox* OverlaySampleEntryBox::clone() const
{
    return CUSTOM_NEW(OverlaySampleEntryBox, (*this));
}

OverlayConfigBox& OverlaySampleEntryBox::getOverlayConfig()
{
    return mOverlayConfig;
}

void OverlaySampleEntryBox::writeBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::writeBox(bitstr);
    BitStream subStream;
    mOverlayConfig.writeBox(subStream);
    bitstr.writeBitStream(subStream);
    updateSize(bitstr);
}

void OverlaySampleEntryBox::parseBox(BitStream& bitstr)
{
    MetaDataSampleEntryBox::parseBox(bitstr);
    FourCCInt boxType;
    auto overlayConfigBoxStream = bitstr.readSubBoxBitStream(boxType);
    mOverlayConfig.parseBox(overlayConfigBoxStream);
}
