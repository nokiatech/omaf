
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
#include <algorithm>
#include <cassert>

#include "api/streamsegmenter/autosegmenter.hpp"
#include "autosegmenterimpl.hpp"
#include "debug.hpp"
#include "utils.hpp"

namespace StreamSegmenter
{
    namespace
    {
        using TimeSpan = std::pair<FrameTime, FrameTime>;

        TimeSpan framesTimeSpan(const std::list<FrameProxy>& aFrames)
        {
            FrameTime trackT0;
            FrameTime trackT1;

            bool first = true;
            for (auto& frame : aFrames)
            {
                for (const auto& cts : frame.getFrameInfo().cts)
                {
                    auto frameT0 = cts;
                    auto frameT1 = cts + frame.getFrameInfo().duration.cast<FrameTime>();

                    if (first || frameT0 < trackT0)
                    {
                        trackT0 = frameT0;
                    }
                    if (first || frameT1 > trackT1)
                    {
                        trackT1 = frameT1;
                    }
                    first = false;
                }
            }

            assert(!first);
            return {trackT0, trackT1};
        }

        FrameTime greatestDtsCtsDelta(const std::list<FrameProxy>& aFrames)
        {
            FrameTime delta;
            for (auto& frame : aFrames)
            {
                const FrameInfo& info = frame.getFrameInfo();
                if (info.dts && info.cts.size())
                {
                    delta = std::max(delta, *info.dts - info.cts.front());
                }
            }
            return delta;
        }

        // falls back to DTS if CTS is not available
        TimeSpan framesCtsTimeSpan(const std::list<FrameProxy>& aFrames)
        {
            FrameTime trackT0;
            FrameTime trackT1;

            bool first = true;
            for (auto& frame : aFrames)
            {
                const FrameInfo& info = frame.getFrameInfo();
                if (info.cts.size())
                {
                    for (const auto& cts : info.cts)
                    {
                        auto frameT0 = cts;
                        auto frameT1 = cts + info.duration.cast<FrameTime>();

                        if (first || frameT0 < trackT0)
                        {
                            trackT0 = frameT0;
                        }
                        if (first || frameT1 > trackT1)
                        {
                            trackT1 = frameT1;
                        }
                        first = false;
                    }
                }
                else
                {
                    if (auto dts = info.dts)
                    {
                        auto frameT0 = *dts;
                        auto frameT1 = *dts + info.duration.cast<FrameTime>();
                        if (first || frameT0 < trackT0)
                        {
                            trackT0 = frameT0;
                        }
                        if (first || frameT1 > trackT1)
                        {
                            trackT1 = frameT1;
                        }
                        first = false;
                    }
                }
            }

            assert(!first);
            return {trackT0, trackT1};
        }

        TimeSpan combinedRange(TimeSpan a, TimeSpan b)
        {
            return {std::min(a.first, b.first), std::max(a.second, b.second)};
        }

        // removed code for TimeSpan& operator+=(TimeSpan& aSpan, const FrameDuration& aDelta)
        // removed code for TimeSpan operator+(const TimeSpan& aSpan, const FrameDuration& aDelta)
        // removed code for FrameDuration totalFrameDuration(const std::list<FrameProxy>& aFrames)
    }  // anonymous namespace

    void AutoSegmenter::Impl::TrackState::feedEnd()
    {
        if (!end)
        {
            end = true;
            if (frames.size())
            {
                Subsegmentful subsegmentful{std::move(frames)};
                completeSubsegments.push_back(subsegmentful);
            }
            completeSegments.push_back(std::move(completeSubsegments));
            trackDurationForSubsegment = {0, 1};
            trackDurationForSegment    = {0, 1};
        }
    }

    void AutoSegmenter::Impl::TrackState::feed(FrameProxy aFrame)
    {
        assert(!end);

        if (hasSubsegmentful && aFrame.getFrameInfo().isIDR)
        {
            hasSubsegmentful = false;
            //            trackDurationForSegment += trackDurationForSubsegment;
            trackDurationForSubsegment -=
                ISOBMFF::coalesce(mImpl->mConfig.subsegmentDuration, mImpl->mConfig.segmentDuration)->cast<FrameTime>();
            if (mSeenSubsegments >= mImpl->mConfig.skipSubsegments)
            {
                Subsegmentful subsegmentful{std::move(frames)};
                completeSubsegments.push_back(subsegmentful);
                mSeenSubsegments = 0;
            }
            else
            {
                ++mSeenSubsegments;
            }
            // may produce empty segments, but that's ok
            while (trackDurationForSegment >= mImpl->mConfig.segmentDuration.cast<FrameTime>())
            {
                completeSegments.push_back(std::move(completeSubsegments));
                trackDurationForSegment -= mImpl->mConfig.segmentDuration.cast<FrameTime>();
            }
        }

        frames.push_back(aFrame);
        trackDurationForSegment += aFrame.getFrameInfo().duration.cast<FrameTime>();
        trackDurationForSubsegment += aFrame.getFrameInfo().duration.cast<FrameTime>();

        hasSubsegmentful =
            hasSubsegmentful ||
            trackDurationForSubsegment >=
                ISOBMFF::coalesce(mImpl->mConfig.subsegmentDuration, mImpl->mConfig.segmentDuration)->cast<FrameTime>();
    }

    std::list<Frames> AutoSegmenter::Impl::TrackState::pullSegment()
    {
        std::list<Frames> segmentFrames;   // multiple subsegments
        if (completeSegments.size() > 0u)  // may not be true when end is true and we have run out of segments
        {
            Subsegmentful subsegmentful{};
            auto completeSegment = std::move(completeSegments.front());
            completeSegments.pop_front();

            for (Subsegmentful& subsegment : completeSegment)
            {
                segmentFrames.push_back(std::move(subsegment.frames));
            }
        }
        return segmentFrames;
    }

    bool AutoSegmenter::Impl::TrackState::isFinished() const
    {
        return end && frames.size() == 0u && completeSubsegments.size() == 0 && completeSegments.size() == 0;
    }

    bool AutoSegmenter::Impl::TrackState::isIncomplete() const
    {
        return frames.size() > 0u;
    }

    bool AutoSegmenter::Impl::TrackState::canTakeSegment() const
    {
        return completeSegments.size() > 0u;
    }

    AutoSegmenter::AutoSegmenter(AutoSegmenterConfig aConfig)
        : mImpl(new Impl())
    {
        assert(aConfig.segmentDuration > FrameDuration(0, 1));
        mImpl->mConfig = aConfig;
        mImpl->mSequenceId =
            mImpl->mConfig.nextFragmentSequenceNumber ? (*mImpl->mConfig.nextFragmentSequenceNumber)() : 0;

        // mImpl->mTargetDuration = Utils::coalesce(mImpl->mConfig.subsegmentDuration,
        // mImpl->mConfig.segmentDuration)->cast<FrameTime>();
    }

    AutoSegmenter::~AutoSegmenter()
    {
        // nothing
    }

    void AutoSegmenter::addTrack(TrackId aTrackId, TrackMeta aTrackMeta)
    {
        assert(aTrackId == aTrackMeta.trackId);
        (void) aTrackId;  // for MSVC release build
        addTrack(aTrackMeta);
    }

    void AutoSegmenter::addTrack(TrackMeta aTrackMeta)
    {
        mImpl->mTrackState[aTrackMeta.trackId]           = Impl::TrackState(mImpl.get());
        mImpl->mTrackState[aTrackMeta.trackId].trackMeta = aTrackMeta;
    }

    AutoSegmenter::Action AutoSegmenter::feedEnd(TrackId aTrackId)
    {
        mImpl->mTrackState[aTrackId].feedEnd();
        return mImpl->areAllTracksReadyForSegment() ? Action::ExtractSegment : Action::KeepFeeding;
    }

    bool AutoSegmenter::Impl::isAnyTrackIncomplete() const
    {
        bool incomplete = false;
        for (auto& trackIdAndTrackFrames : mImpl->mTrackState)
        {
            auto& trackState = trackIdAndTrackFrames.second;
            incomplete       = incomplete || trackState.isIncomplete();
        }
        return incomplete;
    }

    bool AutoSegmenter::Impl::areAllTracksReadyForSegment() const
    {
        bool ready = true;
        for (auto& trackIdAndTrackState : mImpl->mTrackState)
        {
            auto& trackState = trackIdAndTrackState.second;
            ready            = ready && (trackState.end || trackState.canTakeSegment());
        }
        return ready;
    }

    bool AutoSegmenter::Impl::areAllTracksFinished() const
    {
        bool ready = true;
        for (auto& trackIdAndTrackState : mImpl->mTrackState)
        {
            auto& trackState = trackIdAndTrackState.second;
            ready            = ready && trackState.isFinished();
        }
        return ready;
    }

    AutoSegmenter::Action AutoSegmenter::feed(TrackId aTrackId, FrameProxy aFrame)
    {
        assert(!mImpl->mTrackState[aTrackId].end);

        if (!mImpl->mTrackState.count(aTrackId))
        {
            throw InvalidTrack();
        }

        mImpl->mTrackState.at(aTrackId).feed(aFrame);

        return mImpl->areAllTracksReadyForSegment() ? Action::ExtractSegment : Action::KeepFeeding;
    }

    std::list<Segmenter::Segments> AutoSegmenter::extractSegmentsWithSubsegments()
    {
        std::list<Segmenter::Segments> segments;
        while (mImpl->areAllTracksReadyForSegment() && !mImpl->areAllTracksFinished())
        {
            std::map<TrackId, std::list<Frames>> trackSegments;
            for (auto& trackAndState : mImpl->mTrackState)
            {
                TrackId trackId              = trackAndState.first;
                Impl::TrackState& trackState = trackAndState.second;
                trackSegments[trackId]       = trackState.pullSegment();
            }

            Segmenter::Segments subsegments;
            for (auto& trackFrames : trackSegments)
            {
                TrackId trackId                             = trackFrames.first;
                std::list<Frames>& frames                   = trackFrames.second;
                auto subsegmentIt                           = subsegments.begin();
                AutoSegmenter::Impl::TrackState& trackState = mImpl->mTrackState.at(trackId);

                if (mImpl->mFirstSegment && frames.size())
                {
                    for (auto& frame : *frames.begin())
                    {
                        FrameInfo info = frame.getFrameInfo();
                        for (auto cts : info.cts)
                        {
                            if (info.dts)
                            {
                                // Ensure the dts-cts offset is never less than zero
                                //trackState.mTrackOffset = std::max(trackState.mTrackOffset, *info.dts - cts);
                            }

                            // Ensure CTS is never negative.
                            // This doesn't ensure the CTS of the first sample is always zero, though. Should it?
                            //trackState.mTrackOffset = std::max(trackState.mTrackOffset, -cts);
                        }
                    }
                }

                for (Frames& subframes : frames)
                {
                    if (subsegmentIt == subsegments.end())
                    {
                        subsegments.push_back({});
                        subsegmentIt = subsegments.end();
                        --subsegmentIt;
                    }
                    Segmenter::TrackOfSegment& trackOfSegment = (*subsegmentIt).tracks[trackId];
                    trackOfSegment.frames                     = std::move(subframes);

                    if (trackState.mTrackOffset.num)
                    {
                        for (auto& frame : trackOfSegment.frames)
                        {
                            FrameInfo frameInfo = frame.getFrameInfo();
                            for (auto& x : frameInfo.cts)
                            {
                                x += trackState.mTrackOffset;
                            }
                            if (frameInfo.dts)
                            {
                                *frameInfo.dts += trackState.mTrackOffset;
                            }
                            frame.setFrameInfo(frameInfo);
                        }
                    }

                    ++subsegmentIt;
                    trackOfSegment.trackInfo.trackMeta = mImpl->mTrackState.at(trackId).trackMeta;
                    //FrameTime dtsCtsOffset             = greatestDtsCtsDelta(trackOfSegment.frames);
                    FrameTime dtsCtsOffset             = {0, 1};
                    if (auto dts = trackOfSegment.frames.front().getFrameInfo().dts)
                    {
                        trackOfSegment.trackInfo.t0 = *dts;
                    }
                    else
                    {
                        trackOfSegment.trackInfo.t0 = framesCtsTimeSpan(trackOfSegment.frames).first - dtsCtsOffset;
                    }
                    //trackOfSegment.trackInfo.dtsCtsOffset = dtsCtsOffset;
                    trackOfSegment.trackInfo.dtsCtsOffset = trackOfSegment.frames.front().getFrameInfo().cts.front() - trackOfSegment.trackInfo.t0;
;
                }
            }

            for (Segmenter::Segment& segment : subsegments)
            {
                TimeSpan segmentTimeSpan;
                Utils::TrueOnce firstSegmentSpan;
                for (auto trackIdSegment : segment.tracks)
                {
                    // TrackId trackId = trackIdSegment.first;
                    Segmenter::TrackOfSegment& trackOfSegment = trackIdSegment.second;

                    auto timeSpan = framesTimeSpan(trackOfSegment.frames);
                    if (firstSegmentSpan())
                    {
                        segmentTimeSpan = timeSpan;
                    }
                    else
                    {
                        segmentTimeSpan = combinedRange(timeSpan, segmentTimeSpan);
                    }
                }

                segment.sequenceId = mImpl->mSequenceId;
                segment.t0         = segmentTimeSpan.first;
                segment.duration   = (segmentTimeSpan.second - segmentTimeSpan.first).cast<Segmenter::Duration>();
                if (auto& nextSeq = mImpl->mConfig.nextFragmentSequenceNumber)
                {
                    mImpl->mSequenceId = (*nextSeq)();
                }
                else
                {
                    ++mImpl->mSequenceId;
                }
            }

            segments.push_back(subsegments);
            mImpl->mFirstSegment = false;
        }
        return segments;
    }

    // flattens the returned list of lists
    Segmenter::Segments AutoSegmenter::extractSegments()
    {
        std::list<Segmenter::Segments> subsegments = extractSegmentsWithSubsegments();
        Segmenter::Segments segments;
        for (auto& segment : subsegments)
        {
            segments.insert(segments.end(), segment.begin(), segment.end());
        }
        return segments;
    }
}  // namespace StreamSegmenter
