
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
#include "avcsampleentry.hpp"
#include "log.hpp"

AvcSampleEntry::AvcSampleEntry()
    : VisualSampleEntryBox("avc1", "AVC Coding")
    , mAvcConfigurationBox()
    , mIsStereoscopic3DPresent(false)
    , mStereoscopic3DBox()
    , mIsSphericalVideoV2BoxPresent(false)
    , mSphericalVideoV2Box()
{
}

AvcSampleEntry::AvcSampleEntry(const AvcSampleEntry& box)
    : VisualSampleEntryBox(box)
    , mAvcConfigurationBox(box.mAvcConfigurationBox)
    , mIsStereoscopic3DPresent(box.mIsStereoscopic3DPresent)
    , mStereoscopic3DBox(box.mStereoscopic3DBox)
    , mIsSphericalVideoV2BoxPresent(box.mIsSphericalVideoV2BoxPresent)
    , mSphericalVideoV2Box(box.mSphericalVideoV2Box)
{
}

AvcConfigurationBox& AvcSampleEntry::getAvcConfigurationBox()
{
    return mAvcConfigurationBox;
}

void AvcSampleEntry::createStereoscopic3DBox()
{
    mIsStereoscopic3DPresent = true;
}

void AvcSampleEntry::createSphericalVideoV2Box()
{
    mIsSphericalVideoV2BoxPresent = true;
}

const Stereoscopic3D* AvcSampleEntry::getStereoscopic3DBox() const
{
    return (mIsStereoscopic3DPresent ? &mStereoscopic3DBox : nullptr);
}

const SphericalVideoV2Box* AvcSampleEntry::getSphericalVideoV2BoxBox() const
{
    return (mIsSphericalVideoV2BoxPresent ? &mSphericalVideoV2Box : nullptr);
}

void AvcSampleEntry::writeBox(BitStream& bitstr)
{
    VisualSampleEntryBox::writeBox(bitstr);

    mAvcConfigurationBox.writeBox(bitstr);

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

void AvcSampleEntry::parseBox(ISOBMFF::BitStream& bitstr)
{
    VisualSampleEntryBox::parseBox(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        BitStream subBitStream = bitstr.readSubBoxBitStream(boxType);

        // Handle this box based on the type
        if (boxType == "avcC")
        {
            mAvcConfigurationBox.parseBox(subBitStream);
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
            logWarning() << "Skipping unknown box of type '" << boxType << "' inside AvcSampleEntry" << std::endl;
        }
    }

}

AvcSampleEntry* AvcSampleEntry::clone() const
{
    return CUSTOM_NEW(AvcSampleEntry, (*this));
}

const DecoderConfigurationRecord* AvcSampleEntry::getConfigurationRecord() const
{
    return &mAvcConfigurationBox.getConfiguration();
}

const Box* AvcSampleEntry::getConfigurationBox() const
{
    return &mAvcConfigurationBox;
}
