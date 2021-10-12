
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
#include "moviebox.hpp"
#include "bitstream.hpp"
#include "log.hpp"

MovieBox::MovieBox()
    : Box("moov")
    , mMovieHeaderBox()
    , mTracks()
    , mMovieExtendsBox()
{
}

void MovieBox::clear()
{
    mMovieHeaderBox = {};
    mTracks.clear();
    mMovieExtendsBox = {};
}

MovieHeaderBox& MovieBox::getMovieHeaderBox()
{
    return mMovieHeaderBox;
}

const MovieHeaderBox& MovieBox::getMovieHeaderBox() const
{
    return mMovieHeaderBox;
}

Vector<TrackBox*> MovieBox::getTrackBoxes()
{
    Vector<TrackBox*> trackBoxes;
    for (auto& track : mTracks)
    {
        trackBoxes.push_back(track.get());
    }
    return trackBoxes;
}

TrackBox* MovieBox::getTrackBox(uint32_t trackId)
{
    for (auto& track : mTracks)
    {
        if (track.get()->getTrackHeaderBox().getTrackID() == trackId)
        {
            return track.get();
        }
    }
    return nullptr;
}

void MovieBox::addTrackBox(UniquePtr<TrackBox> trackBox)
{
    mTracks.push_back(std::move(trackBox));
}

bool MovieBox::isMovieExtendsBoxPresent() const
{
    return !!mMovieExtendsBox;
}

const MovieExtendsBox* MovieBox::getMovieExtendsBox() const
{
    return mMovieExtendsBox.get();
}

void MovieBox::addMovieExtendsBox(UniquePtr<MovieExtendsBox> movieExtendsBox)
{
    mMovieExtendsBox = std::move(movieExtendsBox);
}

bool MovieBox::isMetaBoxPresent() const
{
    return false;
}

void MovieBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);

    mMovieHeaderBox.writeBox(bitstr);

    for (auto& track : mTracks)
    {
        track->writeBox(bitstr);
    }

    if (mMovieExtendsBox)
    {
        mMovieExtendsBox->writeBox(bitstr);
    }

    updateSize(bitstr);
}

void MovieBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);

    while (bitstr.numBytesLeft() > 0)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);

        if (boxType == "mvhd")
        {
            mMovieHeaderBox.parseBox(subBitstr);
        }
        else if (boxType == "trak")
        {
            UniquePtr<TrackBox> trackBox(CUSTOM_NEW(TrackBox, ()));
            trackBox->parseBox(subBitstr);
            // Ignore box if the handler type is not video, audio or metadata
            FourCCInt handlerType = trackBox->getMediaBox().getHandlerBox().getHandlerType();
            if (handlerType == "vide" || handlerType == "soun" || handlerType == "meta")
            {
                mTracks.push_back(move(trackBox));
            }
        }
        else if (boxType == "mvex")
        {
            mMovieExtendsBox = makeCustomUnique<MovieExtendsBox, MovieExtendsBox>();
            mMovieExtendsBox->parseBox(subBitstr);
        }
        else
        {
            logWarning() << "Skipping an unsupported box '" << boxType << "' inside movie box." << std::endl;
        }
    }
}
