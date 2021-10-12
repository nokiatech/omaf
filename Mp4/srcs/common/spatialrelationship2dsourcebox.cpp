
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
#include "spatialrelationship2dsourcebox.hpp"

using namespace std;

SpatialRelationship2DSourceBox::SpatialRelationship2DSourceBox()
    : FullBox("2dsr", 0, 0)
    , mData({})
{
    // nothing
}

void SpatialRelationship2DSourceBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    mData.write(bitstr);
}

void SpatialRelationship2DSourceBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mData.parse(bitstr);
}

const ISOBMFF::SpatialRelationship2DSourceData&
SpatialRelationship2DSourceBox::get() const
{
    return mData;
}

ISOBMFF::SpatialRelationship2DSourceData&
SpatialRelationship2DSourceBox::get()
{
    return mData;
}
