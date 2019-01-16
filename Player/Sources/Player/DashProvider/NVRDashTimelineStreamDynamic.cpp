
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
#include "DashProvider/NVRDashTimelineStreamDynamic.h"

#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashTimelineStream);

    static const uint32_t MIN_DELAY_IN_MS = 6000;
    static const uint32_t SEGMENT_BUFFER_SIZE = 4;

    DashTimelineStreamDynamic::DashTimelineStreamDynamic(uint32_t bandwidth,
                                                       DashComponents dashComponents,
                                                       uint32_t aInitializationSegmentId,
                                                       DashSegmentStreamObserver *observer)
    : DashSegmentStream(bandwidth, dashComponents, aInitializationSegmentId, observer)
    , mSegments(mMemoryAllocator, 8192)
    , mCurrentSegmentIndex(INVALID_SEGMENT_INDEX)
    , mPrevCheckedIndex(INVALID_SEGMENT_INDEX)
    , mAvailabilityStartTimeUTCSec(0)
    {
        mSeekable = false;
        mMaxCachedSegments = SEGMENT_BUFFER_SIZE;
        mAvailabilityStartTimeUTCSec = Time::fromUTCString(dashComponents.mpd->GetAvailabilityStarttime().c_str());
        mPresentationTimeOffset = mDashComponents.segmentTemplate->GetPresentationTimeOffset();
        mPeriodStartTimeMS = DashUtils::getPeriodStartTimeMS(dashComponents);
        DashUtils::parseSegmentStartTimes(mSegments, dashComponents);
    }

    DashTimelineStreamDynamic::~DashTimelineStreamDynamic()
    {
        shutdownStream();
    }

    bool_t DashTimelineStreamDynamic::mpdUpdateRequired()
    {
        if (mSegments.isEmpty())
        {
            return true;
        }
        if (mCurrentSegmentIndex == mPrevCheckedIndex)
        {
            //To reduce the number of checks
            return false;
        }
        mPrevCheckedIndex = mCurrentSegmentIndex;
        time_t now = time(0);
        uint64_t startTimeMS = (((mSegments.at(mSegments.getSize() - 1).segmentStartTime * 1000ll) + mPresentationTimeOffset) / DashUtils::getTimescale(mDashComponents)) + (mAvailabilityStartTimeUTCSec* 1000ll) + mPeriodStartTimeMS;
        // the playing target time is now - MIN_DELAY_IN_MS. Now we check that we have information about segments in MIN_DELAY_IN_MS to future, so in practice we prepare to buffer for 2x MIN_DELAY_IN_MS
        if (startTimeMS < now * 1000 + MIN_DELAY_IN_MS)
        {
            return true;
        }
        return false;
    }

    Error::Enum DashTimelineStreamDynamic::updateMPD(DashComponents dashComponents)
    {
        int32_t oldcnt = mSegments.getSize();
        DashUtils::parseSegmentStartTimes(mSegments, dashComponents);

        time_t a = Time::fromUTCString(dashComponents.mpd->GetAvailabilityStarttime().c_str());
        uint64_t pto = dashComponents.segmentTemplate->GetPresentationTimeOffset();
        if (a != mAvailabilityStartTimeUTCSec)
        {
            OMAF_LOG_W("Availability time has changed, this might cause some issues");
        }
        if (pto != mPresentationTimeOffset)
        {
            OMAF_LOG_W("PresentationTimeOffset has changed, this might cause some issues");
        }
        OMAF_LOG_V("[%p] mSegments.size() now: %d was: %d", this, mSegments.getSize(), oldcnt);
        DashSegmentStream::updateMPD(dashComponents);
        return Error::OK;
    }

    dash::mpd::ISegment* DashTimelineStreamDynamic::getNextSegment(uint32_t &segmentId)
    {
        if (mOverrideSegmentId != INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = mOverrideSegmentId;
            mOverrideSegmentId = INVALID_SEGMENT_INDEX;
        }

        if (mCurrentSegmentIndex == INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = initializeDynamicSegmentIndex();
        }
        
        if (state() == DashSegmentStreamState::RETRY)
        {
            //We could reset the index here (based on time), but it might break sync etc. (ie. we might skip segments...)
            //So just retry the last index again.
            mCurrentSegmentIndex = mRetryIndex;
        }

        if (mCurrentSegmentIndex > mSegments.getSize())
        {
            //out-of-segments! (and basically this should NOT happen, because we should be doing waitForStreamHead....)
            segmentId = 0;//EOS, technically it's a "waitForStreamHead" unless we are at end of period/mpd/etc...
            return NULL;
        }
        // segment indices start from 1
        OMAF_ASSERT(mCurrentSegmentIndex > 0, "Segment index = 0!!");

        const DashTimelineSegment& seg = mSegments.at(mCurrentSegmentIndex-1);
        uint64_t segmentStartTime = seg.segmentStartTime;
        mSegmentDurationInMs = (seg.segmentDuration * 1000ll) / DashUtils::getTimescale(mDashComponents);
        
        OMAF_LOG_V("[%p] Downloading segment with start time: %llu and duration %d, %d/%d", this,segmentStartTime , mSegmentDurationInMs,mCurrentSegmentIndex,mSegments.getSize());
        
        dash::mpd::ISegment *segment = mDashComponents.segmentTemplate->GetMediaSegmentFromTime( mBaseUrls,
                                                                                 mDashComponents.representation->GetId(),
                                                                                 mDashComponents.representation->GetBandwidth(),
                                                                                 segmentStartTime);
        segmentId = mRetryIndex = mCurrentSegmentIndex;
        mCurrentSegmentIndex++;//mCurrentSegmentIndex is actually the NEXT segment index.
        return segment;
    }

    void_t DashTimelineStreamDynamic::downloadStopped()
    {
        mCurrentSegmentIndex = INVALID_SEGMENT_INDEX;
    }

    void_t DashTimelineStreamDynamic::downloadAborted()
    {
        mCurrentSegmentIndex--;
    }

    bool_t DashTimelineStreamDynamic::waitForStreamHead()
    {
        if (mCurrentSegmentIndex == INVALID_SEGMENT_INDEX)
        {
            //dont wait. start loading.
            return false;
        }

        uint32_t current = (mCurrentSegmentIndex + 1); //Start wait when stream head is at current+1 segment.

        if (mSegments.getSize() <= current)
        {
            return true;
        }
        return false;
    }

    uint32_t DashTimelineStreamDynamic::getSegmentIndexForTime(time_t time)
    {
        int64_t scl = DashUtils::getTimescale(mDashComponents);
        uint64_t ticksFromStartTime = (uint64_t)difftime(time, mAvailabilityStartTimeUTCSec);
        ticksFromStartTime *= scl;
        ticksFromStartTime += mPresentationTimeOffset;
        OMAF_ASSERT(mSegments.getSize()>0,"mSegments.getSize()<=0 in DashTimelineStreamDynamic::getSegmentIndexForTime");
        OMAF_LOG_V("[%p] SegmentIndexFromTime: %lld %lld %lld", this, mSegments[0].segmentStartTime,mSegments.back().segmentStartTime, ticksFromStartTime);
        
        int64_t index = mSegments.getSize() - 1;
        for (; index >= 0; index--)
        {
            uint64_t segmentStartTime = mSegments.at(index).segmentStartTime;
            if (segmentStartTime < ticksFromStartTime)
            {
                break;
            }
        }
        if (index < 0)
        {
            //or, should this be handled as invalid, since the 'time' is out of our segment list..
            OMAF_LOG_W("Can't find segment matching the time %d from the list!", (uint32_t)time);
            index = 0;
        }
        return index + 1;//Because we use 1 based id's for the segments.
    }
    
    uint32_t DashTimelineStreamDynamic::initializeDynamicSegmentIndex()
    {
        time_t now;
        if (mDownloadStartTime != INVALID_START_TIME)
        {
            now = mDownloadStartTime;
        }
        else
        {
            OMAF_LOG_W("Invalid start time, using current time");
            OMAF_ASSERT_UNREACHABLE();
            now = mDownloadStartTime;
        }
        time_t targetTime = now - (MIN_DELAY_IN_MS / 1000);
        uint32_t index = getSegmentIndexForTime(targetTime);
        if (index <= SEGMENT_BUFFER_SIZE)
        {
            index = 1;
        } 
        else
        {
            index -= SEGMENT_BUFFER_SIZE;
        }
        OMAF_LOG_V("[%p] Initialized segmentIndex to: %d from: %d",this, index,mSegments.getSize());
        return index;
    }

    uint32_t DashTimelineStreamDynamic::calculateSegmentId(uint64_t ptsUs)
    {
        uint64_t ticksFromStartTime = (ptsUs / 1e3 + getAvgDownloadTimeMs())/1e3 * DashUtils::getTimescale(mDashComponents);
        int64_t index = mSegments.getSize() - 1;
        for (; index >= 0; index--)
        {
            uint64_t segmentStartTime = mSegments.at(index).segmentStartTime;
            if (segmentStartTime < ticksFromStartTime)
            {
                break;
            }
        }
        if (index < 0)
        {
            //or, should this be handled as invalid, since the 'time' is out of our segment list..
            OMAF_LOG_E("Can't find a segment from Timeline so trying the oldest one...");
            index = 0;
        }
        return index + 1;//Because we use 1 based id's for the segments.
    }

    
OMAF_NS_END
