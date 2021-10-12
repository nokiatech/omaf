
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
#include "mp4vrfilesegment.hpp"
#include "mp4vrfilereaderimpl.hpp"

namespace MP4VR
{
    Segments::Segments(MP4VRFileReaderImpl& impl, InitSegmentId initSegmentId)
        : mImpl(impl)
        , mInitSegmentId(initSegmentId)
    {
        // nothing
    }

    ConstSegments::ConstSegments(const MP4VRFileReaderImpl& impl, InitSegmentId initSegmentId)
        : mImpl(impl)
        , mInitSegmentId(initSegmentId)
    {
        // nothing
    }

    Segments::iterator Segments::begin()
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.begin());
    }

    Segments::iterator Segments::end()
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.end());
    }

    Segments::const_iterator Segments::begin() const
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return const_iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.begin());
    }

    Segments::const_iterator Segments::end() const
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return const_iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.end());
    }

    ConstSegments::const_iterator ConstSegments::begin() const
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return const_iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.begin());
    }

    ConstSegments::const_iterator ConstSegments::end() const
    {
        auto& sequenceToSegment = mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).sequenceToSegment;
        return const_iterator(mImpl, mInitSegmentId, sequenceToSegment, sequenceToSegment.end());
    }

    Segments::iterator::iterator(MP4VRFileReaderImpl& impl,
                                 InitSegmentId initSegmentId,
                                 SequenceToSegmentMap& sequenceToSegment,
                                 SequenceToSegmentMap::iterator theiterator)
        : mImpl(impl)
        , mInitSegmentId(initSegmentId)
        , mSequenceToSegment(sequenceToSegment)
        , mIterator(theiterator)
    {
        // nothing
    }

    SegmentProperties& Segments::iterator::operator*() const
    {
        return mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).segmentPropertiesMap.at(mIterator->second);
    }

    SegmentProperties* Segments::iterator::operator->() const
    {
        return &mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).segmentPropertiesMap.at(mIterator->second);
    }

    bool Segments::iterator::operator!=(const Segments::iterator& other) const
    {
        return &mImpl != &other.mImpl || mIterator != other.mIterator;
    }

    bool Segments::const_iterator::operator!=(const Segments::const_iterator& other) const
    {
        return &mImpl != &other.mImpl || mIterator != other.mIterator;
    }

    Segments::iterator& Segments::iterator::operator++()
    {
        SegmentId curSegmentId = mIterator->second;
        do
        {
            ++mIterator;
        } while (mIterator != mSequenceToSegment.end() && mIterator->second == curSegmentId);
        return *this;
    }

    Segments::const_iterator::const_iterator(const MP4VRFileReaderImpl& impl,
                                             const InitSegmentId initSegmentId,
                                             const SequenceToSegmentMap& sequenceToSegment,
                                             SequenceToSegmentMap::const_iterator iterator)
        : mImpl(impl)
        , mInitSegmentId(initSegmentId)
        , mSequenceToSegment(sequenceToSegment)
        , mIterator(iterator)
    {
        // nothing
    }

    const SegmentProperties& Segments::const_iterator::operator*() const
    {
        return mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).segmentPropertiesMap.at(mIterator->second);
    }

    const SegmentProperties* Segments::const_iterator::operator->() const
    {
        return &mImpl.mInitSegmentPropertiesMap.at(mInitSegmentId).segmentPropertiesMap.at(mIterator->second);
    }

    Segments::const_iterator& Segments::const_iterator::operator++()
    {
        SegmentId curSegmentId = mIterator->second;
        do
        {
            ++mIterator;
        } while (mIterator != mSequenceToSegment.end() && mIterator->second == curSegmentId);
        return *this;
    }
}
