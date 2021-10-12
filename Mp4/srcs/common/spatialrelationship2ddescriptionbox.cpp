
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
#include "spatialrelationship2ddescriptionbox.hpp"
#include "subpictureregionbox.hpp"

using namespace std;

SpatialRelationship2DDescriptionBox::SpatialRelationship2DDescriptionBox()
    : TrackGroupTypeBox("2dcc")
    , mData({})
{
    // nothing
}

void SpatialRelationship2DDescriptionBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    TrackGroupTypeBox::writeBox(bitstr);
    mData.spatialRelationship2DSourceData.write(bitstr);
    SubPictureRegionBox(mData.subPictureRegionData).writeBox(bitstr);
    updateSize(bitstr);
}

void SpatialRelationship2DDescriptionBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    TrackGroupTypeBox::parseBox(bitstr);
    mData.spatialRelationship2DSourceData.parse(bitstr);
    SubPictureRegionBox subPictureRegionBox;
    subPictureRegionBox.parseBox(bitstr);
    mData.subPictureRegionData = subPictureRegionBox.get();
}

SpatialRelationship2DDescriptionBox* SpatialRelationship2DDescriptionBox::clone() const
{
    return CUSTOM_NEW(SpatialRelationship2DDescriptionBox,(*this));
}

const ISOBMFF::SpatialRelationship2DDescriptionData&
SpatialRelationship2DDescriptionBox::get() const
{
    return mData;
}

ISOBMFF::SpatialRelationship2DDescriptionData&
SpatialRelationship2DDescriptionBox::get()
{
    return mData;
}
