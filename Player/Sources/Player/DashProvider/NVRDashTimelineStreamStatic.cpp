
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
#include "DashProvider/NVRDashTimelineStreamStatic.h"

#include "DashProvider/NVRDashUtils.h"
#include "DashProvider/NVRDashMediaSegment.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashTimelineStream);

    DashTimelineStreamStatic::DashTimelineStreamStatic(uint32_t bandwidth,
                                           DashComponents dashComponents,
                                           uint32_t aInitializationSegmentId,
                                           DashSegmentStreamObserver *observer)
    : DashSegmentStream(bandwidth, dashComponents, aInitializationSegmentId, observer)
    , mSegments(mMemoryAllocator, 8192)
    , mCurrentSegmentIndex(INVALID_SEGMENT_INDEX)
    , mIndexForSeek(INVALID_SEGMENT_INDEX)
    , mAvailabilityStartTimeUTCSec(0)
    {
        mSeekable = true;
        mMaxCachedSegments = 4;
        mAvailabilityStartTimeUTCSec = Time::fromUTCString( dashComponents.mpd->GetAvailabilityStarttime().c_str() );

        DashUtils::parseSegmentStartTimes(mSegments, dashComponents);
        mTotalDurationMs = DashUtils::getStreamDurationInMilliSeconds(dashComponents);
        // If total duration doesn't exist, calculate it ourselves
        if (mTotalDurationMs == 0)
        {
            uint64_t segmentTimescale = dashComponents.segmentTemplate->GetTimescale();
            uint64_t firstSegmentStartTime = mSegments.at(0).segmentStartTime;
            uint64_t lastSegmentStartTime = mSegments.at(mSegments.getSize() - 1).segmentStartTime;
            uint64_t lastSegmentDuration = mSegments.at(mSegments.getSize() - 1).segmentDuration;
            //Should we do a "ceil" type rounding, currently flooring, so the total length might be off by ~millisecond :)
            mTotalDurationMs = ((lastSegmentStartTime + lastSegmentDuration - firstSegmentStartTime)*1000ll) / segmentTimescale;
        }
        mStartNumber=DashUtils::getStartNumber(dashComponents);
    }

    DashTimelineStreamStatic::~DashTimelineStreamStatic()
    {
        shutdownStream();
    }

    dash::mpd::ISegment* DashTimelineStreamStatic::getNextSegment(uint32_t &segmentId)
    {
        if (mCurrentSegmentIndex == INVALID_SEGMENT_INDEX)
        {
            //starting from beginning
            mCurrentSegmentIndex = 1;
        }

        if (mIndexForSeek != INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = mIndexForSeek;
            mIndexForSeek = INVALID_SEGMENT_INDEX;
        }

        if (mOverrideSegmentId != INVALID_SEGMENT_INDEX)
        {
            mCurrentSegmentIndex = mOverrideSegmentId;
            mOverrideSegmentId = INVALID_SEGMENT_INDEX;
        }

        if (state() == DashSegmentStreamState::RETRY)
        {
            mCurrentSegmentIndex = mRetryIndex;
        }

        if (mCurrentSegmentIndex > mSegments.getSize())
        {
            //end of stream. no more segments.
            segmentId = 0;//EOS
            return NULL;
        }
        const DashTimelineSegment& seg = mSegments.at(mCurrentSegmentIndex - 1);
        uint64_t segmentStartTime = seg.segmentStartTime;
        mSegmentDurationInMs = (seg.segmentDuration * 1000ll) / DashUtils::getTimescale(mDashComponents);
        
        int segmentNumber = mStartNumber + mCurrentSegmentIndex - 1;
        dash::mpd::ISegment *segment = mDashComponents.segmentTemplate->ToSegment(mDashComponents.segmentTemplate->Getmedia(), mBaseUrls, mDashComponents.representation->GetId(), mDashComponents.representation->GetBandwidth(), dash::metrics::MediaSegment, segmentNumber, segmentStartTime);
        
        OMAF_LOG_V("Downloading segment with start time: %llu and duration %d uri:%s", segmentStartTime, mSegmentDurationInMs,segment->AbsoluteURI().c_str());
        segmentId = mRetryIndex = mCurrentSegmentIndex;
        mCurrentSegmentIndex++;
        return segment;
    }

    void_t DashTimelineStreamStatic::downloadAborted()
    {
        mCurrentSegmentIndex--;
    }

    Error::Enum DashTimelineStreamStatic::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
    {
        OMAF_ASSERT(!mSegments.isEmpty(), "DashTimelineStreamStatic::seekToMs");

        uint64_t targetTime = (aSeekToTargetMs * DashUtils::getTimescale(mDashComponents))/1000ll;
        targetTime += mSegments.at(0).segmentStartTime;

        uint64_t index = 0;
        uint64_t currentStartTime = mSegments.at(index).segmentStartTime;
        for (;index < mSegments.getSize()-1; index++)
        {
            uint64_t nextStartTime= mSegments.at(index+1).segmentStartTime;
            if ((currentStartTime<=targetTime)&&(nextStartTime>targetTime))
            {
                break;
            }
            currentStartTime = nextStartTime;
        }

        mIndexForSeek = index + 1;
        aSeekToResultMs = targetTime;
        return Error::OK;
    }

    uint32_t DashTimelineStreamStatic::calculateSegmentId(uint64_t ptsUs)
    {
        uint64_t targetTimeMs = (ptsUs / 1e3) + getAvgDownloadTimeMs();
        uint64_t resultTimeMS = 0;
        seekToMs(targetTimeMs, resultTimeMS);
        return mIndexForSeek;
    }

OMAF_NS_END
