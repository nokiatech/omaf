
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
#include "overlayconfigbox.hpp"

OverlayConfigBox::OverlayConfigBox()
    : FullBox("ovly", 0, 0)
    , mOverlayStruct()
{
}

ISOBMFF::OverlayStruct& OverlayConfigBox::getOverlayStruct()
{
    return mOverlayStruct;
}

const ISOBMFF::OverlayStruct& OverlayConfigBox::getOverlayStruct() const
{
    return mOverlayStruct;
}

void OverlayConfigBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    mOverlayStruct.write(bitstr);
    updateSize(bitstr);
}

void OverlayConfigBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mOverlayStruct.parse(bitstr);
}
