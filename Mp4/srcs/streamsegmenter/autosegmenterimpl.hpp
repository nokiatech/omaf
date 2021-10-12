
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
#pragma once

#include "api/streamsegmenter/autosegmenter.hpp"

namespace StreamSegmenter
{
    struct AutoSegmenter::Impl
    {
        struct TrackState
        {
            Impl* mImpl;
            TrackMeta trackMeta;
            Frames frames;

            struct Subsegmentful
            {
                Frames frames;
            };

            // once a subsegment is deemed complete, the related frames are moved here
            std::list<Subsegmentful> completeSubsegments;

            // once a segment is deemed complete, the related subsegments are moved here
            std::list<std::list<Subsegmentful>> completeSegments;

            // can be negative due to subsegment duration adjustments
            FrameTime trackDurationForSegment;
            FrameTime trackDurationForSubsegment;

            // No more input will arrive.
            bool end = false;

            // There is enough data for one subsegment; but more for that segment could still arrive.
            bool hasSubsegmentful = false;

            // How many subsegments would have we inserted to completeSubsegments, if we accepted
            // them all?
            size_t mSeenSubsegments = 0;

            FrameTime mTrackOffset;

            TrackState(Impl* aImpl = nullptr)
                : mImpl(aImpl)
            {
                // nothing
            }

            void feed(FrameProxy aFrame);

            void feedEnd();

            bool isFinished() const;

            bool isIncomplete() const;

            bool canTakeSegment() const;

            // not really useful for higher level functionality, as we only provide complete segments to the user
            //bool canTakeSubsegment() const;

            // returns multiple subsegments (or just one, if not using subsegmenting)
            std::list<Frames> pullSegment();
        };

        // Makes a segment of whatever collected subsegments
        // void makeSegment();

        // Are all tracks done and done
        bool areAllTracksFinished() const;

        // Are all tracks ready for a (sub)segment (or finished)
        bool areAllTracksReadyForSegment() const;

        // Is any of the tracks with content to create a new segment, even a partial?
        bool isAnyTrackIncomplete() const;

        Impl* const mImpl;
        AutoSegmenterConfig mConfig;
        std::map<TrackId, TrackState> mTrackState;
        FrameTime mSegmentT0;
        bool mFirstSegment = true;

        // set to a positive value if the DTS of first frame is negative; adjusts th cts/dts of
        // all frames of all tracks
        FrameTime mGlobalOffset;

        Segmenter::SequenceId mSequenceId;

        Impl()
            : mImpl(this)
        {
            // the rest of the fields are initialized in AutoSegmenter::AutoSegmenter
        }
    };
}
