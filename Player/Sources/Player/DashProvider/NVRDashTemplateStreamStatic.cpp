
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
#include "DashProvider/NVRDashTemplateStreamStatic.h"

#include "DashProvider/NVRDashUtils.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashStaticTemplateStream)

    static uint32_t kBufferingTime(10000);
    static uint32_t kAbsMaxCacheSize(15);    // with very short segments, too large cache derived from target buffering time can throttles the whole system
    
    DashTemplateStreamStatic::DashTemplateStreamStatic(uint32_t bandwidth,
                                                       DashComponents dashComponents,
                                                       uint32_t aInitializationSegmentId,
                                                       DashSegmentStreamObserver *observer)
        : DashSegmentStream(bandwidth, dashComponents, aInitializationSegmentId, observer)
        , mIndexForSeek(INVALID_SEGMENT_INDEX)
    {
        mSeekable = true;
        mStartIndex = DashUtils::getStartNumber(dashComponents);
        mCurrentSegmentIndex = mStartIndex;
        uint64_t segmentTimescale = DashUtils::getTimescale(dashComponents);
        uint64_t segmentDuration = DashUtils::getDuration(dashComponents);
        mSegmentDurationInMs = (segmentDuration * 1000ll) / segmentTimescale;
        mTotalDurationMs = DashUtils::getStreamDurationInMilliSeconds(dashComponents);
        mSegmentCount = (mTotalDurationMs+(mSegmentDurationInMs-1)) / mSegmentDurationInMs; //round up.
        mMaxCachedSegments = min(mSegmentCount, 
                                    min(kAbsMaxCacheSize,
                                        max(INITIAL_CACHE_SIZE,(uint32_t)(kBufferingTime / mSegmentDurationInMs) + 1))); // buffer for at least kBufferingTime seconds (round up) or 2 buffers (buffering state is entered if only 1 segment is cached)
    }

    DashTemplateStreamStatic::~DashTemplateStreamStatic()
    {
        shutdownStream();
    }

    dash::mpd::ISegment* DashTemplateStreamStatic::getNextSegment(uint32_t &segmentId)
    {
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
        if (mCurrentSegmentIndex > mSegmentCount)
        {
            //end of stream
            segmentId = 0;
            return NULL;
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

    void_t DashTemplateStreamStatic::downloadAborted()
    {
        mCurrentSegmentIndex--;
    }

    Error::Enum DashTemplateStreamStatic::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
    {
        mIndexForSeek = mStartIndex + (aSeekToTargetMs / mSegmentDurationInMs);
        OMAF_LOG_D("Seeking to %lld ms which means segment index: %d", aSeekToTargetMs, mIndexForSeek);
        aSeekToResultMs = aSeekToTargetMs;
        return Error::OK;
    }

    uint32_t DashTemplateStreamStatic::calculateSegmentId(uint64_t ptsUs)
    {
        uint64_t targetTimeMs = ptsUs / 1000 + getAvgDownloadTimeMs();
        uint32_t overrideSegmentId = (uint32_t)(targetTimeMs / segmentDurationMs());
        // + first segment id
        overrideSegmentId += mStartIndex;
        OMAF_LOG_D("Calculated segment id: %d, from time %lld", overrideSegmentId, ptsUs);

        return overrideSegmentId;
    }

    bool_t DashTemplateStreamStatic::hasFixedSegmentSize() const
    {
        return true;
    }
    bool_t DashTemplateStreamStatic::isLastSegment() const
    {
        if (mCurrentSegmentIndex >= mSegmentCount - 1)
        {
            return true;
        }
        return false;
    }

OMAF_NS_END
