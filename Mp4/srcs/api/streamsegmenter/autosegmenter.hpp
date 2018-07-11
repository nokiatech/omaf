
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
#ifndef STREAMSEGMENTER_AUTOSEGMENTER_HPP
#define STREAMSEGMENTER_AUTOSEGMENTER_HPP

#include <list>
#include <map>

#include "optional.hpp"
#include "segmenterapi.hpp"
#include "track.hpp"

namespace StreamSegmenter
{
    class InvalidTrack : public std::runtime_error
    {
    public:
        InvalidTrack()
            : std::runtime_error("InvalidTrack")
        {
        }
    };

    class ExpectedIDRFrame : public std::runtime_error
    {
    public:
        ExpectedIDRFrame()
            : std::runtime_error("ExpectedIDRFrame")
        {
        }
    };

    struct AutoSegmenterConfig
    {
        bool checkIDR = false;  // check that the first frame is an IDR frame
        Segmenter::Duration segmentDuration;
        Utils::Optional<Segmenter::Duration> subsegmentDuration;
        size_t skipSubsegments = 0;  // number of subsegments to skip from saving
    };

    class AutoSegmenter
    {
    public:
        AutoSegmenter(AutoSegmenterConfig aConfig);

        virtual ~AutoSegmenter();

        enum class Action
        {
            KeepFeeding,
            ExtractSegment
        };

        // deprecated
        void addTrack(TrackId aTrackId, TrackMeta aTrackMeta);

        void addTrack(TrackMeta aTrackMeta);

        // Call this until it returns ExtractSegment, then it's a good
        // time to call extractSegments
        Action feed(TrackId aTrackId, FrameProxy aFrame);
        Action feedEnd(TrackId aTrackId);

        std::list<Segmenter::Segments> extractSegmentsWithSubsegments();
        Segmenter::Segments extractSegments();

    private:
        struct Impl;

        std::unique_ptr<Impl> mImpl;
    };

}  // namespace StreamSegmenter

#endif
