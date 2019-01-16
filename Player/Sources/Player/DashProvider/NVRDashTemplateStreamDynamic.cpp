
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
#include "DashProvider/NVRDashTemplateStreamDynamic.h"
#include "DashProvider/NVRDashUtils.h"
#include "Math/OMAFMathFunctions.h"
#include "Foundation/NVRLogger.h"

#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashDynamicTemplateStream);

    DashTemplateStreamDynamic::DashTemplateStreamDynamic(uint32_t bandwidth,
                                                       DashComponents dashComponents,
                                                       uint32_t aInitializationSegmentId,
                                                       DashSegmentStreamObserver *observer)
    : DashSegmentStream(bandwidth, dashComponents, aInitializationSegmentId, observer)
    , mStartIndex(0)
    , mCurrentSegmentIndex(INVALID_SEGMENT_INDEX)
    , mMinDelayInSegments(2)
    , mMinPlaybackDelayMs(0)
    , mMaxBufferSizeMs(0)
    , mAvailabilityStartTimeUTCSec(0)

    {
        mMaxCachedSegments = 4;
        mSeekable = false;
        mAvailabilityStartTimeUTCSec = Time::fromUTCString( dashComponents.mpd->GetAvailabilityStarttime().c_str());

        mStartIndex = DashUtils::getStartNumber(dashComponents);

        uint64_t segmentDuration = DashUtils::getDuration(dashComponents);
        uint64_t segmentTimeScale = DashUtils::getTimescale(dashComponents);
        mSegmentDurationInMs = (segmentDuration * 1000ll) / segmentTimeScale;

        mMinPlaybackDelayMs = mSegmentDurationInMs * mMinDelayInSegments;
        mMaxBufferSizeMs = mSegmentDurationInMs * mMaxCachedSegments;
    }

    DashTemplateStreamDynamic::~DashTemplateStreamDynamic()
    {
        shutdownStream();
    }

    dash::mpd::ISegment* DashTemplateStreamDynamic::getNextSegment(uint32_t &segmentId)
    {
        if (mOverrideSegmentId != INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = mOverrideSegmentId;
            mOverrideSegmentId = INVALID_SEGMENT_INDEX;
        }

        if (mCurrentSegmentIndex == INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = initializeSegmentIndex();
        }

        if (state() == DashSegmentStreamState::RETRY)
        {
            mCurrentSegmentIndex = mRetryIndex;
        }

        dash::mpd::ISegment *segment = mDashComponents.segmentTemplate->GetMediaSegmentFromNumber(
                mBaseUrls,
                mDashComponents.representation->GetId(),
                mDashComponents.representation->GetBandwidth(),
                mCurrentSegmentIndex);
        segmentId = mRetryIndex = mCurrentSegmentIndex;
        mCurrentSegmentIndex++;
        return segment;
    }

    void_t DashTemplateStreamDynamic::downloadStopped()
    {
        mCurrentSegmentIndex = INVALID_SEGMENT_INDEX;
    }

    bool_t DashTemplateStreamDynamic::waitForStreamHead()
    {
        time_t now = time(0);
        uint32_t currentServerSideSegmentIndex = getSegmentIndexForTime(now, false);
        int32_t minDelaySegments = mMinPlaybackDelayMs / mSegmentDurationInMs;
        int32_t currentSegmentDelay = currentServerSideSegmentIndex - mCurrentSegmentIndex;
        if (currentSegmentDelay < minDelaySegments)
        {
            // We are already pretty close to the streamc head. Need to give the server some time to create the files.
            OMAF_LOG_D(
                    "Stream bandwidth %d: Sleeping to increase delay from stream head. Local segment index = %d, server = %d, diff = %d (%d ms), min delay = %d (%d ms)",
                    mBandwidth,
                    mCurrentSegmentIndex,
                    currentServerSideSegmentIndex,
                    currentSegmentDelay,
                    currentSegmentDelay * mSegmentDurationInMs,
                    minDelaySegments,
                    minDelaySegments * mSegmentDurationInMs);
            return true;
        }
        else
        {
            return false;
        }
    }

    void_t DashTemplateStreamDynamic::downloadAborted()
    {
        mCurrentSegmentIndex--;
    }

    uint32_t DashTemplateStreamDynamic::initializeSegmentIndex()
    {
        uint32_t minDelaySegments = mMinPlaybackDelayMs / mSegmentDurationInMs;
        uint32_t maxBufferedSegments = mMaxBufferSizeMs / mSegmentDurationInMs;
        uint32_t delayMs = (maxBufferedSegments + minDelaySegments) * mSegmentDurationInMs;

        time_t now;
        if (mDownloadStartTime != INVALID_START_TIME)
        {
            now = mDownloadStartTime;
        }
        else
        {
            OMAF_LOG_W("Invalid start time, using current time");
            OMAF_ASSERT_UNREACHABLE();
            now = time(0);
        }
        time_t playTime = now - delayMs / 1000;
        uint32_t segmentIndex = getSegmentIndexForTime(playTime, true);

        OMAF_LOG_V("Stream bandwidth %d: Initializing stream position for live streaming", mBandwidth);
        OMAF_LOG_V("  Segment duration (ms): %d", mSegmentDurationInMs);
        OMAF_LOG_V("  Segment delay: %d ms -> %d segments", mMinPlaybackDelayMs, minDelaySegments);
        OMAF_LOG_V("  Segment buffering: %d ms -> %d segments", mMaxBufferSizeMs, maxBufferedSegments);
        OMAF_LOG_V("  Initial delay (ms): %d", delayMs);
        OMAF_LOG_V("  Current time (UTC): %ld", now);
        OMAF_LOG_V("  Play time (UTC): %ld", playTime);
        OMAF_LOG_V("  Segment startNumber: %d", mDashComponents.segmentTemplate->GetStartNumber());
        OMAF_LOG_V("  Current segment index: %ld", segmentIndex);
        return segmentIndex;
    }

    uint32_t DashTemplateStreamDynamic::getSegmentIndexForTime(time_t time, bool_t printLog)
    {
        uint64_t secondsFromStartTime = (uint64_t) max((int64_t)0, (int64_t)difftime(time, mAvailabilityStartTimeUTCSec));
        uint64_t milliSecondsFromStartTime = secondsFromStartTime * 1000ll;
        uint32_t segmentsSinceStart = static_cast<uint32_t>(milliSecondsFromStartTime / mSegmentDurationInMs);
        uint64_t segmentNumber = mDashComponents.segmentTemplate->GetStartNumber() + segmentsSinceStart;
        //Following logs cause considerable spam.
        if (printLog)
        {
            OMAF_LOG_V("getSegmentIDForTime(): Availability start time (seconds since Epoch): %ld", mAvailabilityStartTimeUTCSec);
            OMAF_LOG_V("getSegmentIDForTime(): Requested time = %ld, seconds from start time = %lld", time, secondsFromStartTime);
            OMAF_LOG_V("getSegmentIDForTime(): Segments since start: %lld", segmentsSinceStart);
            OMAF_LOG_V("getSegmentIDForTime(): Segment number for given time: %d", segmentNumber);
        }
        return segmentNumber;
    }

    uint32_t DashTemplateStreamDynamic::calculateSegmentId(uint64_t ptsUs)
    {
        // ptsUs is the play head position, so we don't need to consider availability start time
        uint64_t targetTimeMs = ptsUs / 1000 + getAvgDownloadTimeMs();
        uint32_t overrideSegmentId = (uint32_t)(targetTimeMs / segmentDurationMs());
        // + first segment id
        overrideSegmentId += mStartIndex;
        OMAF_LOG_D("Calculated segment id: %d, from time %lld", overrideSegmentId, ptsUs);

        return overrideSegmentId;
    }

    bool_t DashTemplateStreamDynamic::hasFixedSegmentSize() const
    {
        return true;
    }

OMAF_NS_END
