
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
#include "moviefragmentbox.hpp"
#include "log.hpp"

MovieFragmentBox::MovieFragmentBox(Vector<MOVIEFRAGMENTS::SampleDefaults>& sampleDefaults)
    : Box("moof")
    , mMovieFragmentHeaderBox()
    , mTrackFragmentBoxes()
    , mSampleDefaults(sampleDefaults)
    , mFirstByteOffset(0)
{
}

MovieFragmentHeaderBox& MovieFragmentBox::getMovieFragmentHeaderBox()
{
    return mMovieFragmentHeaderBox;
}

const MovieFragmentHeaderBox& MovieFragmentBox::getMovieFragmentHeaderBox() const
{
    return mMovieFragmentHeaderBox;
}

void MovieFragmentBox::addTrackFragmentBox(UniquePtr<TrackFragmentBox> trackFragmentBox)
{
    mTrackFragmentBoxes.push_back(std::move(trackFragmentBox));
}

Vector<TrackFragmentBox*> MovieFragmentBox::getTrackFragmentBoxes()
{
    Vector<TrackFragmentBox*> trackFragmentBoxes;
    for (auto& trackFragmentBox : mTrackFragmentBoxes)
    {
        trackFragmentBoxes.push_back(trackFragmentBox.get());
    }
    return trackFragmentBoxes;
}

void MovieFragmentBox::setMoofFirstByteOffset(std::uint64_t moofFirstByteOffset)
{
    mFirstByteOffset = moofFirstByteOffset;
}

std::uint64_t MovieFragmentBox::getMoofFirstByteOffset()
{
    return mFirstByteOffset;
}

const ISOBMFF::Optional<MetaBox>& MovieFragmentBox::getMetaBox() const
{
    return mMetaBox;
}

MetaBox& MovieFragmentBox::addMetaBox()
{
    if (!mMetaBox)
    {
        mMetaBox = MetaBox();
    }
    return *mMetaBox;
}

void MovieFragmentBox::writeBox(BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    mMovieFragmentHeaderBox.writeBox(bitstr);
    for (auto& trackFragmentBox : mTrackFragmentBoxes)
    {
        trackFragmentBox->writeBox(bitstr);
    }
    if (mMetaBox)
    {
        mMetaBox->writeBox(bitstr);
    }
    updateSize(bitstr);
}

void MovieFragmentBox::parseBox(BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "mfhd")
        {
            mMovieFragmentHeaderBox.parseBox(subBitstr);
        }
        else if (boxType == "traf")
        {
            UniquePtr<TrackFragmentBox> trackFragmentBox(CUSTOM_NEW(TrackFragmentBox, (mSampleDefaults)));
            try
            {
                trackFragmentBox->parseBox(subBitstr);
                mTrackFragmentBoxes.push_back(std::move(trackFragmentBox));
            }
            catch (RuntimeError& error)
            {
                if (String(error.what()) == "TrackFragmentBox: default sample description index not found")
                {
                    // TOOD: nicer way to del with this situation (missing track in mSampleDefaults)

                    // we simply don't add these tracks; it may be the case that we don't have all the required other
                    // informtion for these tracks as we are handling with moof data without the matching trackbox
                }
                else
                {
                    throw;
                }
            }
        }
        else if (boxType == "meta")
        {
            MetaBox& meta = addMetaBox();
            meta.parseBox(subBitstr);
        }
        else
        {
            logWarning() << "Skipping an unsupported box '" << boxType << "' inside MovieFragmentBox." << std::endl;
        }
    }
}
