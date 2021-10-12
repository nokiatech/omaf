
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
#include "streams.h"

namespace VDD
{
    Streams::Streams(std::initializer_list<Data> aStreams)
        : Streams(aStreams.begin(), aStreams.end())
    {
        // nothing
    }

    const Data& Streams::front() const
    {
        return mStreams[0];
    }

    size_t Streams::size() const
    {
        return mStreams.size();
    }

    void Streams::add(const Data& aData)
    {
        mStreams.push_back(aData);
        if (mStreams.size() == 1)
        {
            mEndOfStream = aData.isEndOfStream();
        }
        else
        {
            assert(mEndOfStream == aData.isEndOfStream());
        }
    }

    bool Streams::isEndOfStream() const
    {
        return mEndOfStream;
    }
}