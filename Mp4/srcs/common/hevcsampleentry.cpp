
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
#include "hevcsampleentry.hpp"
#include "log.hpp"

HevcSampleEntry::HevcSampleEntry()
    : VisualSampleEntryBox("hvc1", "HEVC Coding")
    , mHevcConfigurationBox()
    , mIsStereoscopic3DPresent(false)
    , mStereoscopic3DBox()
    , mIsSphericalVideoV2BoxPresent(false)
    , mSphericalVideoV2Box()
{
}

HevcSampleEntry::HevcSampleEntry(const HevcSampleEntry& box)
    : VisualSampleEntryBox(box)
    , mHevcConfigurationBox(box.mHevcConfigurationBox)
    , mIsStereoscopic3DPresent(box.mIsStereoscopic3DPresent)
    , mStereoscopic3DBox(box.mStereoscopic3DBox)
    , mIsSphericalVideoV2BoxPresent(box.mIsSphericalVideoV2BoxPresent)
    , mSphericalVideoV2Box(box.mSphericalVideoV2Box)
{
}

HevcConfigurationBox& HevcSampleEntry::getHevcConfigurationBox()
{
    return mHevcConfigurationBox;
}

void HevcSampleEntry::createStereoscopic3DBox()
{
    mIsStereoscopic3DPresent = true;
}

void HevcSampleEntry::createSphericalVideoV2Box()
{
    mIsSphericalVideoV2BoxPresent = true;
}

const Stereoscopic3D* HevcSampleEntry::getStereoscopic3DBox() const
{
    return (mIsStereoscopic3DPresent ? &mStereoscopic3DBox : nullptr);
}

const SphericalVideoV2Box* HevcSampleEntry::getSphericalVideoV2BoxBox() const
{
    return (mIsSphericalVideoV2BoxPresent ? &mSphericalVideoV2Box : nullptr);
}

void HevcSampleEntry::writeBox(ISOBMFF::BitStream& bitstr)
{
    VisualSampleEntryBox::writeBox(bitstr);

    mHevcConfigurationBox.writeBox(bitstr);

    if (mIsStereoscopic3DPresent)
    {
        mStereoscopic3DBox.writeBox(bitstr);
    }

    if (mIsSphericalVideoV2BoxPresent)
    {
        mSphericalVideoV2Box.writeBox(bitstr);
    }

    // Update the size of the movie box
    updateSize(bitstr);
}

void HevcSampleEntry::parseBox(ISOBMFF::BitStream& bitstr)
{
    VisualSampleEntryBox::parseBox(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        BitStream subBitStream = bitstr.readSubBoxBitStream(boxType);

        // Handle this box based on the type
        if (boxType == "hvcC")
        {
            mHevcConfigurationBox.parseBox(subBitStream);
        }
        else if (boxType == "st3d")
        {
            mStereoscopic3DBox.parseBox(subBitStream);
            mIsStereoscopic3DPresent = true;
        }
        else if (boxType == "sv3d")
        {
            mSphericalVideoV2Box.parseBox(subBitStream);
            mIsSphericalVideoV2BoxPresent = true;
        }
        else
        {
            logWarning() << "Skipping unknown box of type '" << boxType << "' inside HevcSampleEntry" << std::endl;
        }
    }
}

HevcSampleEntry* HevcSampleEntry::clone() const
{
    return CUSTOM_NEW(HevcSampleEntry, (*this));
}

const Box* HevcSampleEntry::getConfigurationBox() const
{
    return &mHevcConfigurationBox;
}

const DecoderConfigurationRecord* HevcSampleEntry::getConfigurationRecord() const
{
    return &mHevcConfigurationBox.getConfiguration();
}
