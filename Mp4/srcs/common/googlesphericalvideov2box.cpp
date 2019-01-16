
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "googlesphericalvideov2box.hpp"

SphericalVideoV2Box::SphericalVideoV2Box()
    : Box("sv3d")
    , mSphericalVideoHeaderBox()
    , mProjectionBox()
{
}

const SphericalVideoHeader& SphericalVideoV2Box::getSphericalVideoHeaderBox() const
{
    return mSphericalVideoHeaderBox;
}

void SphericalVideoV2Box::setSphericalVideoHeaderBox(const SphericalVideoHeader& header)
{
    mSphericalVideoHeaderBox = header;
}

const Projection& SphericalVideoV2Box::getProjectionBox() const
{
    return mProjectionBox;
}

void SphericalVideoV2Box::setProjectionBox(const Projection& projectionBox)
{
    mProjectionBox = projectionBox;
}

void SphericalVideoV2Box::writeBox(ISOBMFF::BitStream& bitstr)
{
    // Write box headers
    writeBoxHeader(bitstr);

    mSphericalVideoHeaderBox.writeBox(bitstr);
    mProjectionBox.writeBox(bitstr);

    updateSize(bitstr);
}

void SphericalVideoV2Box::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    while (bitstr.numBytesLeft() >= 8)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);
        if (boxType == "svhd")
        {
            mSphericalVideoHeaderBox.parseBox(subBitstr);
        }
        else if (boxType == "proj")
        {
            mProjectionBox.parseBox(subBitstr);
        }
        // unknown boxes are skipped.
    }
}
