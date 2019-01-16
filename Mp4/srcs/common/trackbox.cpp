
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
#include "trackbox.hpp"
#include "googlesphericalvideov1box.hpp"
#include "log.hpp"

using namespace std;

TrackBox::TrackBox()
    : Box("trak")
    , mTrackHeaderBox()
    , mMediaBox()
    , mTrackReferenceBox()
    , mHasTrackReferences(false)
    , mTrackGroupBox()
    , mHasTrackGroupBox(false)
    , mHasTrackTypeBox(false)
    , mEditBox(nullptr)
    , mHasGoogleSphericalVideoV1Box(false)
    , mSphericalVideoV1Box()
{
}

void TrackBox::setHasTrackReferences(bool value)
{
    mHasTrackReferences = value;
}

bool TrackBox::getHasTrackReferences() const
{
    return mHasTrackReferences;
}

const TrackHeaderBox& TrackBox::getTrackHeaderBox() const
{
    return mTrackHeaderBox;
}

TrackHeaderBox& TrackBox::getTrackHeaderBox()
{
    return mTrackHeaderBox;
}

const MediaBox& TrackBox::getMediaBox() const
{
    return mMediaBox;
}

MediaBox& TrackBox::getMediaBox()
{
    return mMediaBox;
}

const TrackReferenceBox& TrackBox::getTrackReferenceBox() const
{
    return mTrackReferenceBox;
}

TrackReferenceBox& TrackBox::getTrackReferenceBox()
{
    return mTrackReferenceBox;
}

const TrackGroupBox& TrackBox::getTrackGroupBox() const
{
    return mTrackGroupBox;
}

TrackGroupBox& TrackBox::getTrackGroupBox()
{
    return mTrackGroupBox;
}

void TrackBox::setHasTrackGroup(bool value)
{
    mHasTrackGroupBox = value;
}

bool TrackBox::getHasTrackGroup() const
{
    return mHasTrackGroupBox;
}

const TrackTypeBox& TrackBox::getTrackTypeBox() const
{
    return mTrackTypeBox;
}

TrackTypeBox& TrackBox::getTrackTypeBox()
{
    return mTrackTypeBox;
}

void TrackBox::setHasTrackTypeBox(bool value)
{
    mHasTrackTypeBox = value;
}

bool TrackBox::getHasTrackTypeBox() const
{
    return mHasTrackTypeBox;
}

void TrackBox::setEditBox(const EditBox& editBox)
{
    if (mEditBox == nullptr)
    {
        mEditBox = makeCustomShared<EditBox>(editBox);
    }
    else
    {
        *mEditBox = editBox;
    }
}

std::shared_ptr<const EditBox> TrackBox::getEditBox() const
{
    return mEditBox;
}

const SphericalVideoV1Box& TrackBox::getSphericalVideoV1Box() const
{
    return mSphericalVideoV1Box;
}

SphericalVideoV1Box& TrackBox::getSphericalVideoV1Box()
{
    return mSphericalVideoV1Box;
}

void TrackBox::setHasSphericalVideoV1Box(bool value)
{
    mHasGoogleSphericalVideoV1Box = value;
}

bool TrackBox::getHasSphericalVideoV1Box() const
{
    return mHasGoogleSphericalVideoV1Box;
}

void TrackBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    // Write box headers
    writeBoxHeader(bitstr);

    // Write other boxes contained in the movie box
    // The TrackHeaderBox
    mTrackHeaderBox.writeBox(bitstr);

    if (mHasTrackReferences)
    {
        mTrackReferenceBox.writeBox(bitstr);
    }

    // The MediaBox
    mMediaBox.writeBox(bitstr);

    if (mHasTrackGroupBox)
    {
        mTrackGroupBox.writeBox(bitstr);
    }

    if (mEditBox)
    {
        mEditBox->writeBox(bitstr);
    }

    if (mHasGoogleSphericalVideoV1Box)
    {
        mSphericalVideoV1Box.writeBox(bitstr);
    }

    if (mHasTrackTypeBox)
    {
        mTrackTypeBox.writeBox(bitstr);
    }

    // Update the size of the movie box
    updateSize(bitstr);
}

void TrackBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    //  First parse the box header
    parseBoxHeader(bitstr);

    // if there a data available in the file
    while (bitstr.numBytesLeft() > 0)
    {
        // Extract contained box bitstream and type
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "tkhd")
        {
            mTrackHeaderBox.parseBox(subBitstr);
        }
        else if (boxType == "mdia")
        {
            mMediaBox.parseBox(subBitstr);
        }
        else if (boxType == "meta")
        {
            // @todo Implement this when reading meta box in tracks is supported
        }
        else if (boxType == "tref")
        {
            mTrackReferenceBox.parseBox(subBitstr);
            mHasTrackReferences = true;
        }
        else if (boxType == "trgr")
        {
            mHasTrackGroupBox = true;
            mTrackGroupBox.parseBox(subBitstr);
        }
        else if (boxType == "ttyp")
        {
            mHasTrackTypeBox = true;
            mTrackTypeBox.parseBox(subBitstr);
        }
        else if (boxType == "edts")
        {
            mEditBox = makeCustomShared<EditBox>();
            mEditBox->parseBox(subBitstr);
        }
        else if (boxType == "uuid")
        {
            Vector<uint8_t> extendedType;
            subBitstr.readBoxUUID(extendedType);

            Vector<uint8_t> comparison = GOOGLE_SPHERICAL_VIDEO_V1_GLOBAL_UUID;
            if (extendedType == comparison)
            {
                mSphericalVideoV1Box.parseBox(subBitstr);
                mHasGoogleSphericalVideoV1Box = true;
            }
            else
            {
                logWarning() << "Skipping an unsupported UUID box inside TrackBox." << std::endl;
            }
        }
        else
        {
            logWarning() << "Skipping an unsupported box '" << boxType << "' inside TrackBox." << std::endl;
        }
    }
}
