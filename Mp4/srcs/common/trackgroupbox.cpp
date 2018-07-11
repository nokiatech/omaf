
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
#include "trackgroupbox.hpp"

TrackGroupBox::TrackGroupBox()
    : Box("trgr")
{
}

const Vector<TrackGroupTypeBox>& TrackGroupBox::getTrackGroupTypeBoxes() const
{
    return mTrackGroupTypeBoxes;
}

void TrackGroupBox::addTrackGroupTypeBox(const TrackGroupTypeBox& trackGroupTypeBox)
{
    mTrackGroupTypeBoxes.push_back(trackGroupTypeBox);
}

void TrackGroupBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    for (unsigned int i = 0; i < mTrackGroupTypeBoxes.size(); i++)
    {
        mTrackGroupTypeBoxes.at(i).writeBox(bitstr);
    }

    // Update the size of the movie box
    updateSize(bitstr);
}

void TrackGroupBox::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);
    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        TrackGroupTypeBox tracktypebox = TrackGroupTypeBox(boxType);
        tracktypebox.parseBox(subBitstr);
        mTrackGroupTypeBoxes.push_back(tracktypebox);
    }
}
