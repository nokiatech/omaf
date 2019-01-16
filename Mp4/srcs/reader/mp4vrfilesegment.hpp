
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
#ifndef MP4VRFILESEGMENT_H
#define MP4VRFILESEGMENT_H

#include "mp4vrfiledatatypesinternal.hpp"

namespace MP4VR
{
    class MP4VRFileReaderImpl;

    class Segments
    {
    public:
        Segments(MP4VRFileReaderImpl& impl, InitSegmentId initSegmentId);

        class iterator
        {
        public:
            iterator(MP4VRFileReaderImpl& impl,
                     InitSegmentId initSegmentId,
                     SequenceToSegmentMap& sequenceToSegment,
                     SequenceToSegmentMap::iterator);
            SegmentProperties& operator*() const;
            SegmentProperties* operator->() const;
            iterator& operator++();
            MP4VRFileReaderImpl& mImpl;
            InitSegmentId mInitSegmentId;
            SequenceToSegmentMap& mSequenceToSegment;
            SequenceToSegmentMap::iterator mIterator;

            bool operator!=(const iterator& other) const;
        };

        class const_iterator
        {
        public:
            const_iterator(const MP4VRFileReaderImpl& impl,
                           const InitSegmentId initSegmentId,
                           const SequenceToSegmentMap& sequenceToSegment,
                           SequenceToSegmentMap::const_iterator);
            const SegmentProperties& operator*() const;
            const SegmentProperties* operator->() const;
            const_iterator& operator++();
            const MP4VRFileReaderImpl& mImpl;
            const InitSegmentId mInitSegmentId;
            const SequenceToSegmentMap& mSequenceToSegment;
            SequenceToSegmentMap::const_iterator mIterator;

            bool operator!=(const const_iterator& other) const;
        };

        iterator begin();
        const_iterator begin() const;
        iterator end();
        const_iterator end() const;

    private:
        MP4VRFileReaderImpl& mImpl;
        InitSegmentId mInitSegmentId;
    };

    class ConstSegments
    {
    public:
        ConstSegments(const MP4VRFileReaderImpl& impl, InitSegmentId initSegmentId);

        typedef Segments::const_iterator iterator;
        typedef Segments::const_iterator const_iterator;

        const_iterator begin() const;
        const_iterator end() const;

    private:
        const MP4VRFileReaderImpl& mImpl;
        InitSegmentId mInitSegmentId;
    };
}

#endif  // MP4VRFILESEGMENT_H
