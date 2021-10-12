
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
#include "subpictureregionbox.hpp"

using namespace std;

SubPictureRegionBox::SubPictureRegionBox()
    : FullBox("sprg", 0, 0)
    , mData({})
{
    // nothing
}

SubPictureRegionBox::SubPictureRegionBox(const ISOBMFF::SubPictureRegionData& aData)
    : FullBox("sprg", 0, 0)
    , mData(aData)
{
    // nothing
}

void SubPictureRegionBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    mData.write(bitstr);
}

void SubPictureRegionBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mData.parse(bitstr);
}

const ISOBMFF::SubPictureRegionData&
SubPictureRegionBox::get() const
{
    return mData;
}

ISOBMFF::SubPictureRegionData&
SubPictureRegionBox::get()
{
    return mData;
}
