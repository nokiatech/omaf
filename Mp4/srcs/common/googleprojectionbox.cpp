
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
#include "googleprojectionbox.hpp"
#include <cassert>


Projection::Projection()
    : Box("proj")
    , mProjectionHeaderBox()
    , mProjectionType(ProjectionType::UNKNOWN)
    , mCubemapProjectionBox()
    , mEquirectangularProjectionBox()
{
}

const ProjectionHeader& Projection::getProjectionHeaderBox() const
{
    return mProjectionHeaderBox;
}

void Projection::setProjectionHeaderBox(const ProjectionHeader& header)
{
    mProjectionHeaderBox = header;
}

Projection::ProjectionType Projection::getProjectionType() const
{
    return mProjectionType;
}

const CubemapProjection& Projection::getCubemapProjectionBox() const
{
    return mCubemapProjectionBox;
}

void Projection::setCubemapProjectionBox(const CubemapProjection& projection)
{
    mProjectionType       = ProjectionType::CUBEMAP;
    mCubemapProjectionBox = projection;
}

const EquirectangularProjection& Projection::getEquirectangularProjectionBox() const
{
    return mEquirectangularProjectionBox;
}

void Projection::setEquirectangularProjectionBox(const EquirectangularProjection& projection)
{
    mProjectionType               = ProjectionType::EQUIRECTANGULAR;
    mEquirectangularProjectionBox = projection;
}

void Projection::writeBox(BitStream& bitstr)
{
    // Write box headers
    writeBoxHeader(bitstr);

    mProjectionHeaderBox.writeBox(bitstr);
    if (mProjectionType == ProjectionType::CUBEMAP)
    {
        mCubemapProjectionBox.writeBox(bitstr);
    }
    else if (mProjectionType == ProjectionType::EQUIRECTANGULAR)
    {
        mEquirectangularProjectionBox.writeBox(bitstr);
    }
    else
    {
        // MESH PROJECTION NOT SUPPORTED
        assert(false);
    }

    updateSize(bitstr);
}

void Projection::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    while (bitstr.numBytesLeft() >= 8)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);
        if (boxType == "prhd")
        {
            mProjectionHeaderBox.parseBox(subBitstr);
        }
        else if (boxType == "cbmp")
        {
            mProjectionType = ProjectionType::CUBEMAP;
            mCubemapProjectionBox.parseBox(subBitstr);
        }
        else if (boxType == "equi")
        {
            mProjectionType = ProjectionType::EQUIRECTANGULAR;
            mEquirectangularProjectionBox.parseBox(subBitstr);
        }
        else if (boxType == "mshp")
        {
            mProjectionType = ProjectionType::MESH;
            // this projection type box parsing not supported so skip.
        }
        // unknown boxes are skipped.
    }
}
