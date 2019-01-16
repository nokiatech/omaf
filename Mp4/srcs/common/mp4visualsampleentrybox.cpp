
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
#include "mp4visualsampleentrybox.hpp"
#include <string>
#include "log.hpp"

MP4VisualSampleEntryBox::MP4VisualSampleEntryBox()
    : VisualSampleEntryBox("mp4v", "MPEG4 Visual Coding")
    , mESDBox()
    , mIsStereoscopic3DPresent(false)
    , mStereoscopic3DBox()
    , mIsSphericalVideoV2BoxPresent(false)
    , mSphericalVideoV2Box()
{
}

MP4VisualSampleEntryBox::MP4VisualSampleEntryBox(const MP4VisualSampleEntryBox& box)
    : VisualSampleEntryBox(box)
    , mESDBox(box.mESDBox)
    , mIsStereoscopic3DPresent(box.mIsStereoscopic3DPresent)
    , mStereoscopic3DBox(box.mStereoscopic3DBox)
    , mIsSphericalVideoV2BoxPresent(box.mIsSphericalVideoV2BoxPresent)
    , mSphericalVideoV2Box(box.mSphericalVideoV2Box)
{
}

void MP4VisualSampleEntryBox::createStereoscopic3DBox()
{
    mIsStereoscopic3DPresent = true;
}

void MP4VisualSampleEntryBox::createSphericalVideoV2Box()
{
    mIsSphericalVideoV2BoxPresent = true;
}

ElementaryStreamDescriptorBox& MP4VisualSampleEntryBox::getESDBox()
{
    return mESDBox;
}

const Stereoscopic3D* MP4VisualSampleEntryBox::getStereoscopic3DBox() const
{
    return (mIsStereoscopic3DPresent ? &mStereoscopic3DBox : nullptr);
}

const SphericalVideoV2Box* MP4VisualSampleEntryBox::getSphericalVideoV2BoxBox() const
{
    return (mIsSphericalVideoV2BoxPresent ? &mSphericalVideoV2Box : nullptr);
}

void MP4VisualSampleEntryBox::writeBox(BitStream& bitstr)
{
    VisualSampleEntryBox::writeBox(bitstr);
    mESDBox.writeBox(bitstr);

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

void MP4VisualSampleEntryBox::parseBox(BitStream& bitstr)
{
    VisualSampleEntryBox::parseBox(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "esds")
        {
            mESDBox.parseBox(subBitstr);
        }
        else if (boxType == "st3d")
        {
            mStereoscopic3DBox.parseBox(subBitstr);
            mIsStereoscopic3DPresent = true;
        }
        else if (boxType == "sv3d")
        {
            mSphericalVideoV2Box.parseBox(subBitstr);
            mIsSphericalVideoV2BoxPresent = true;
        }
        else
        {
            logWarning() << "Skipping unknown box of type '" << boxType << "' inside MP4VisualSampleEntryBox"
                         << std::endl;
        }
    }
}

MP4VisualSampleEntryBox* MP4VisualSampleEntryBox::clone() const
{
    return CUSTOM_NEW(MP4VisualSampleEntryBox, (*this));
}

const Box* MP4VisualSampleEntryBox::getConfigurationBox() const
{
    logError() << "MP4VisualSampleEntryBox::getConfigurationBox() not impelmented " << std::endl;
    return nullptr;
}

const DecoderConfigurationRecord* MP4VisualSampleEntryBox::getConfigurationRecord() const
{
    logError() << "MP4VisualSampleEntryBox::getConfigurationRecord() not impelmented" << std::endl;
    return nullptr;
}
