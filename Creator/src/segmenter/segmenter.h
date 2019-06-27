
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
#pragma once

#include <fstream>
#include <map>
#include <set>

#include <streamsegmenter/autosegmenter.hpp>
#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/track.hpp>

#include "common/optional.h"
#include "common/exceptions.h"
#include "processor/processor.h"
#include "log/log.h"

namespace VDD
{
    typedef StreamSegmenter::IdBase<std::int8_t, struct ScalTrefIndexTag> ScalTrefIndex;

    /** A hierarchical umbrella class for all Segmenter exceptions */
    class SegmenterException : public Exception
    {
    public:
        SegmenterException(std::string aWhat) : Exception("Segmenter/" + aWhat)
        {
        }
    };

    struct SegmentWriterDestructor
    {
        void operator()(StreamSegmenter::Writer* aWriter)
        {
            StreamSegmenter::Writer::destruct(aWriter);
        }
    };
    struct SegmentMovieWriterDestructor
    {
        void operator()(StreamSegmenter::MovieWriter* aWriter)
        {
            StreamSegmenter::MovieWriter::destruct(aWriter);
        }
    };

    /** Data for an unexpected track was received; set expected tracks
     * with Config.tracks */
    class InvalidTrack : public SegmenterException
    {
    public:
        InvalidTrack(TrackId);
    };

    /** checkIDR is on and an IDR frame was not found where expected */
    class ExpectedIDRFrame : public SegmenterException
    {
    public:
        ExpectedIDRFrame();
    };

    /** A Meta tag for informing about the role of a data segment; whether it's an init segment or
     * not */
    enum class SegmentRole
    {
        InitSegment,
        SegmentIndex, // used if Config.separateSidx is enabled, otherwise embedded in TrackRunSegment
        TrackRunSegment
    };

    using SegmentRoleTag = ValueTag<SegmentRole>;

    // First key is the extractor track id, second key is the track id to map; result
    // is obviously the final scal track reference index
    using TrackToScalTrafIndexMap = std::map<TrackId, std::map<TrackId, ScalTrefIndex>>;

    class Segmenter : public Processor
    {
    public:
        struct Config
        {
            /** Segment duration; the IDR-frames of the input material
             * must be at these exact points in the input data */
            StreamSegmenter::Segmenter::Duration segmentDuration;

            /** Subsegment duration; the IDR-frames of the input material
             * must be at these exact points in the input data */
            Optional<StreamSegmenter::Segmenter::Duration> subsegmentDuration;

            /** Check (and throw ExpectedIDRFrame on mismatch) that the
             * input has IDR frames at least exactly at segmentDuration
             * intervals */
            bool checkIDR = true;

            /** Provide information for each track. In particular, the
             * (output) track id and time scale of the track.  An entry
             * for each input track must exist here, or otherwise
             * InvalidTrack will be thrown */
            std::map<TrackId, StreamSegmenter::TrackMeta> tracks;

            /** Start counting segment numbers from this base value */
            StreamSegmenter::Segmenter::SequenceId baseSequenceId;

            /** Push out segment indices separately; also disables writing segment type header */
            bool separateSidx;

            std::list<StreamId> streamIds; // more than 1 used at least with mp4 single file output; stream ids is just for gatekeeping; what streams to process and what to skip. No need to have exact mapping to tracks

            std::shared_ptr<Log> log;

            // Special support for extractor tracks: map track ids to scal track reference indices
            TrackToScalTrafIndexMap trackToScalTrafIndexMap;
        };

        Segmenter(Config aConfig, bool aCreateWriter = true);
        virtual ~Segmenter() override;

        /** Segmenter requires the data to be available in CPU memory */
        StorageType getPreferredStorageType() const override;

        virtual std::vector<Views> process(const Views& data) override;

        virtual std::string getGraphVizDescription() override;

    protected:
        std::vector<Views> doProcess(const Views& data);

        /* Feed the frame, with complete with presentation and coding times, to the */
        void feed(TrackId aTrackId, const Data& aFrame, StreamSegmenter::FrameCts aCompositionCts);

        /* Keep track of frames by their presentation index */
        struct PendingFrameInfo
        {
            bool hasPreviousFrame; // the previous frame is available (or the frame is the first one)
            Data frame;
            StreamSegmenter::FrameTime codingTime;
            StreamSegmenter::FrameCts compositionTime;
        };

        struct TrackInfo
        {
            FrameTime nextFramePresTime; // used for constructing frame presentation time
            std::map<Index, PendingFrameInfo> pendingFrames;

            /** Is this the first frame ever? */
            bool firstFrame = true;

            /** if !mFirstFrame, this has the last presentation time; use to set determine hasPreviousFrame
             * when mPendingFrames.size() == 1 */
            Index lastPresIndex;

            /** coding time for the next frame */
            FrameTime nextCodingTime;

            /* increased when adding a new PendingFrameInfo or when updating PendingFrameInfo to have hasPreviousFrame = true
             * decreased when consuming a frame */
            size_t numConsecutiveFrames = 0;

            /* Has this stream reached its end? */
            bool end = false;
        };

        /** Information about tracks that varies at runtime */
        std::map<TrackId, TrackInfo> mTrackInfo;

        /** Configuration */
        const Config mConfig;

        /** The auto segmenter instance */
        StreamSegmenter::AutoSegmenter mAutoSegmenter;

        bool mWaitForInitSegment = false;

        bool detectNonRefFrame(const Data& frame);

        virtual void writeSegment(StreamSegmenter::Segmenter::Segments& aSegments, std::vector<Views>& aFrames);

        void packExtractors(const Data& aFrame, std::vector<Data>& aOutput);

    private:

        std::unique_ptr<StreamSegmenter::Writer, SegmentWriterDestructor> mSegmentWriter;

        StreamSegmenter::SidxWriter* mSidxWriter = nullptr;
    };
}

