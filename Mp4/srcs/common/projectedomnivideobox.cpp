
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
#include <cstdint>

#include "bitstream.hpp"
#include "log.hpp"
#include "projectedomnivideobox.hpp"

ProjectedOmniVideoBox::ProjectedOmniVideoBox()
    : Box("povd")
    , mProjectionFormatBox()
    , mRegionWisePackingBox()
    , mCoverageInformationBox()
    , mOverlayConfigBox()
{
}

ProjectedOmniVideoBox::ProjectedOmniVideoBox(const ProjectedOmniVideoBox& box)
    : Box(box)
    , mProjectionFormatBox(box.mProjectionFormatBox)
    , mRegionWisePackingBox(box.mRegionWisePackingBox ? makeCustomUnique<RegionWisePackingBox, RegionWisePackingBox>(
                                                            *box.mRegionWisePackingBox)
                                                      : UniquePtr<RegionWisePackingBox>())
    , mCoverageInformationBox(
          box.mCoverageInformationBox
              ? makeCustomUnique<CoverageInformationBox, CoverageInformationBox>(*box.mCoverageInformationBox)
              : UniquePtr<CoverageInformationBox>())
    , mRotationBox(box.mRotationBox ? makeCustomUnique<RotationBox, RotationBox>(*box.mRotationBox)
                                    : UniquePtr<RotationBox>())
    , mOverlayConfigBox(box.mOverlayConfigBox
                            ? makeCustomUnique<OverlayConfigBox, OverlayConfigBox>(*box.mOverlayConfigBox)
                            : UniquePtr<OverlayConfigBox>())
{
}

ProjectionFormatBox& ProjectedOmniVideoBox::getProjectionFormatBox()
{
    return mProjectionFormatBox;
}

RegionWisePackingBox& ProjectedOmniVideoBox::getRegionWisePackingBox() const
{
    return *mRegionWisePackingBox;
}

void ProjectedOmniVideoBox::setRegionWisePackingBox(UniquePtr<RegionWisePackingBox> rwpkBox)
{
    mRegionWisePackingBox = std::move(rwpkBox);
}

bool ProjectedOmniVideoBox::hasRegionWisePackingBox() const
{
    return !!mRegionWisePackingBox;
}

CoverageInformationBox& ProjectedOmniVideoBox::getCoverageInformationBox() const
{
    return *mCoverageInformationBox;
}

void ProjectedOmniVideoBox::setCoverageInformationBox(UniquePtr<CoverageInformationBox> coviBox)
{
    mCoverageInformationBox = std::move(coviBox);
}

bool ProjectedOmniVideoBox::hasCoverageInformationBox() const
{
    return !!mCoverageInformationBox;
}

RotationBox& ProjectedOmniVideoBox::getRotationBox() const
{
    return *mRotationBox;
}

void ProjectedOmniVideoBox::setRotationBox(UniquePtr<RotationBox> rotnBox)
{
    mRotationBox = std::move(rotnBox);
}

bool ProjectedOmniVideoBox::hasRotationBox() const
{
    return !!mRotationBox;
}

OverlayConfigBox& ProjectedOmniVideoBox::getOverlayConfigBox() const
{
    return *mOverlayConfigBox;
}

void ProjectedOmniVideoBox::setOverlayConfigBox(UniquePtr<OverlayConfigBox> ovlyBox)
{
    mOverlayConfigBox = std::move(ovlyBox);
}

bool ProjectedOmniVideoBox::hasOverlayConfigBox() const
{
    return !!mOverlayConfigBox;
}

void ProjectedOmniVideoBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    mProjectionFormatBox.writeBox(bitstr);
    if (mRegionWisePackingBox)
    {
        mRegionWisePackingBox->writeBox(bitstr);
    }
    if (mCoverageInformationBox)
    {
        mCoverageInformationBox->writeBox(bitstr);
    }
    if (mRotationBox)
    {
        mRotationBox->writeBox(bitstr);
    }
    if (mOverlayConfigBox)
    {
        mOverlayConfigBox->writeBox(bitstr);
    }
    updateSize(bitstr);
}

void ProjectedOmniVideoBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    mProjectionFormatBox.parseBox(bitstr);
    if (mProjectionFormatBox.getType() != "prfr")
    {
        throw RuntimeError("Povd box must start with prfr box");
    }

    while (bitstr.numBytesLeft() > 0)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        BitStream subBitStream = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "rwpk")
        {
            mRegionWisePackingBox = makeCustomUnique<RegionWisePackingBox, RegionWisePackingBox>();
            mRegionWisePackingBox->parseBox(subBitStream);
        }
        else if (boxType == "covi")
        {
            mCoverageInformationBox = makeCustomUnique<CoverageInformationBox, CoverageInformationBox>();
            mCoverageInformationBox->parseBox(subBitStream);
        }
        else if (boxType == "rotn")
        {
            mRotationBox = makeCustomUnique<RotationBox, RotationBox>();
            mRotationBox->parseBox(subBitStream);
        }
        else if (boxType == "ovly")
        {
            mOverlayConfigBox = makeCustomUnique<OverlayConfigBox, OverlayConfigBox>();
            mOverlayConfigBox->parseBox(subBitStream);
        }
        else
        {
            logWarning() << "Ignoring unknown BoxType found from povd box: " << boxType << std::endl;
        }
    }
}
