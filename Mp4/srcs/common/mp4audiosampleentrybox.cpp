
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "mp4audiosampleentrybox.hpp"
#include <string>

MP4AudioSampleEntryBox::MP4AudioSampleEntryBox()
    : AudioSampleEntryBox("mp4a")
    , mESDBox()
    , mHasSpatialAudioBox(false)
    , mSpatialAudioBox()
    , mHasNonDiegeticAudioBox(false)
    , mNonDiegeticAudioBox()
    , mRecord(mESDBox)
{
}

ElementaryStreamDescriptorBox& MP4AudioSampleEntryBox::getESDBox()
{
    return mESDBox;
}

const ElementaryStreamDescriptorBox& MP4AudioSampleEntryBox::getESDBox() const
{
    return mESDBox;
}

bool MP4AudioSampleEntryBox::hasSpatialAudioBox() const
{
    return mHasSpatialAudioBox;
}

const SpatialAudioBox& MP4AudioSampleEntryBox::getSpatialAudioBox() const
{
    return mSpatialAudioBox;
}

void MP4AudioSampleEntryBox::setSpatialAudioBox(const SpatialAudioBox& spatialAudioBox)
{
    mHasSpatialAudioBox = true;
    mSpatialAudioBox    = spatialAudioBox;
}

bool MP4AudioSampleEntryBox::hasNonDiegeticAudioBox() const
{
    return mHasNonDiegeticAudioBox;
}

const NonDiegeticAudioBox& MP4AudioSampleEntryBox::getNonDiegeticAudioBox() const
{
    return mNonDiegeticAudioBox;
}

void MP4AudioSampleEntryBox::setNonDiegeticAudioBox(const NonDiegeticAudioBox& nonDiegeticAudioBox)
{
    mHasNonDiegeticAudioBox = true;
    mNonDiegeticAudioBox    = nonDiegeticAudioBox;
}

void MP4AudioSampleEntryBox::writeBox(BitStream& bitstr)
{
    AudioSampleEntryBox::writeBox(bitstr);
    mESDBox.writeBox(bitstr);

    if (mHasSpatialAudioBox)
    {
        mSpatialAudioBox.writeBox(bitstr);
    }

    if (mHasNonDiegeticAudioBox)
    {
        mNonDiegeticAudioBox.writeBox(bitstr);
    }

    updateSize(bitstr);
}

void MP4AudioSampleEntryBox::parseBox(BitStream& bitstr)
{
    AudioSampleEntryBox::parseBox(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "esds")
        {
            mESDBox.parseBox(subBitstr);
        }
        else if (boxType == "SA3D")
        {
            mHasSpatialAudioBox = true;
            mSpatialAudioBox.parseBox(subBitstr);
        }
        else if (boxType == "SAND")
        {
            mHasNonDiegeticAudioBox = true;
            mNonDiegeticAudioBox.parseBox(subBitstr);
        }
    }
}

MP4AudioSampleEntryBox* MP4AudioSampleEntryBox::clone() const
{
    return CUSTOM_NEW(MP4AudioSampleEntryBox, (*this));
}

const Box* MP4AudioSampleEntryBox::getConfigurationBox() const
{
    return &mESDBox;
}

const DecoderConfigurationRecord* MP4AudioSampleEntryBox::getConfigurationRecord() const
{
    return &mRecord;
}
