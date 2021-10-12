
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

#include <IMPD.h>

#include "NVRNamespace.h"

#include "NVRErrorCodes.h"

#include "DashProvider/NVRDashMediaSegment.h"
#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRFixedQueue.h"

#include "Foundation/NVRMutex.h"

#include "DashProvider/NVRDashOptions.h"
#include "Foundation/NVRHttpConnection.h"

OMAF_NS_BEGIN
class DashSegment;
extern const uint32_t INVALID_SEGMENT_INDEX;
extern const time_t INVALID_START_TIME;
extern const uint32_t INITIAL_CACHE_DURATION_MS;
extern const uint32_t RUNTIME_CACHE_DURATION_MS;
extern const uint32_t PREBUFFER_CACHE_LIMIT_MS;
extern const uint32_t ABS_MIN_CACHE_BUFFERS;

typedef FixedQueue<DashSegment*, 1024> DashSegments;

class DashSegmentStreamObserver
{
public:
    virtual Error::Enum onMediaSegmentDownloaded(DashSegment* segment, float32_t aSpeedFactor) = 0;
    virtual void_t onSegmentIndexDownloaded(DashSegment* segment) = 0;
    virtual void_t onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs) = 0;
    virtual void_t onCacheWarning() = 0;
};

namespace DashSegmentStreamState
{
    enum Enum
    {
        INVALID = -1,

        UNINITIALIZED,
        DOWNLOADING_INIT_SEGMENT,
        IDLE,
        DOWNLOADING_MEDIA_SEGMENT,
        RETRY,
        END_OF_STREAM,
        ERROR,
        ABORTING,
        DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP,
        COUNT
    };
}

namespace DashSegmentIndexState
{
    enum Enum
    {
        INVALID = -1,

        WAITING,
        DOWNLOADING_SEGMENT_INDEX,
        RETRY_SEGMENT_INDEX,
        SEGMENT_INDEX_ERROR,

        COUNT
    };
}

class DashSegmentStream : public DashSegmentObserver, public IHttpDataProcessor
{
public:
    DashSegmentStream(uint32_t bandwidth,
                      DashComponents dashComponents,
                      uint32_t aInitializationSegmentId,
                      DashSegmentStreamObserver* observer);

    virtual ~DashSegmentStream();

    void_t startDownload(time_t downloadStartTime, bool_t aIsIndependent);
    void_t startDownloadFromSegment(uint32_t overrideSegmentId, bool_t aIsIndependent);
    void_t startDownloadFrom(uint32_t overrideSegmentId, bool_t aIsIndependent);
    void_t
    startDownloadWithByteRange(uint32_t overrideSegmentId, uint64_t startByte, uint64_t endByte, bool_t aIsIndependent);

    virtual bool_t mpdUpdateRequired();

    void_t stopDownload();
    void_t stopDownloadAsync(bool_t aAbort);
    void_t stopSegmentIndexDownload();

    void_t clearDownloadedSegments();

    DashSegment* getInitSegment() const;

    uint32_t getAvgDownloadTimeMs();
    uint32_t getInitializationSegmentId();

    virtual bool_t isSeekable();
    virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs, uint32_t& aSeekSegmentIndex);
    uint64_t durationMs();
    uint64_t segmentDurationMs();

    bool_t isBuffering();
    bool_t isEndOfStream();
    bool_t isError();

    virtual Error::Enum updateMPD(DashComponents components);

    uint32_t cachedSegments() const;
    uint64_t getDownloadedBytes() const;

    bool_t isDownloading();
    bool_t isCompletingDownload();
    bool_t isActive();

    void_t setCacheFillMode(bool_t autoFill);
    void_t setBufferingTime(uint32_t aBufferingTimeMs);

    virtual uint32_t calculateSegmentId(uint64_t& ptsUs) = 0;
    virtual bool_t hasFixedSegmentSize() const;

    virtual bool_t setLatencyReq(LatencyRequirement::Enum aType);

    virtual void_t processHttpData();
    virtual void_t processSegmentDownload(bool_t aForcedCacheUpdate);
    virtual void_t processSegmentIndexDownload(uint32_t segmentId);
    void_t completePendingDownload();

    virtual void_t setLoopingOn();

public:  // DashSegmentObserver
    void_t onSegmentDeleted();


protected:
    virtual void downloadStopped();

    virtual bool_t waitForStreamHead();

    virtual dash::mpd::ISegment* getNextSegment(uint32_t& segmentId) = 0;
    virtual void_t downloadAborted() = 0;
    virtual bool_t isLastSegment() const;

    virtual void processUninitialized();
    virtual void processIdleOrRetry();
    virtual void processDownloadingInitSegment();
    virtual bool_t needInitSegment(bool_t aIsIndependent);

    void_t shutdownStream();

    const DashSegmentStreamState::Enum state();

    virtual void_t handleDownloadedSegment(int64_t aDownloadTimeMs, size_t aBytesDownloaded);
    void_t updateDownloadTime(int64_t newDownloadTime);

private:
    void_t startDownloader(time_t downloadStartTime, uint32_t overrideSegmentId, bool_t aIsIndependent);
    void_t processIdleOrRetrySegmentIndex(uint32_t segmentId);
    void_t processDownloadingSegmentIndex(uint32_t segmentId);
    void_t processDownloadingMediaSegment(bool_t aLateReception = false);

protected:
    MemoryAllocator& mMemoryAllocator;
    uint32_t mBandwidth;

    DashComponents mDashComponents;
    DashSegment* mInitializeSegment;
    std::vector<dash::mpd::IBaseUrl*> mBaseUrls;  // Not owned.
    uint32_t mMaxCachedSegments;
    uint32_t mPreBufferCachedSegments;
    uint32_t mAbsMaxCachedSegments;
    uint32_t mSegmentDurationInMs;
    uint64_t mTotalDurationMs;
    bool_t mSeekable;
    uint32_t mOverrideSegmentId;
    time_t mDownloadStartTime;

    uint32_t mRetryIndex;
    DashSegmentStreamState::Enum mState;

    HttpConnection* mSegmentConnection;
    struct ByteRange
    {
        uint64_t startByte;
        uint64_t endByte;
    } mDownloadByteRange;

    DashSegment* mNextSegment;
    int64_t mSegmentDownloadStartTime;
    DashSegmentStreamObserver* mObserver;
    int64_t mLastRequest;
    uint32_t mBufferingTimeMs;
    uint32_t mCachedSegmentCount;
    uint32_t mTotalSegmentsDownloaded;
    uint32_t mDownloadRetryCounter;
    uint64_t mTotalBytesDownloaded;
    bool_t mPreBuffering;

private:
    bool_t mRunning;
    int64_t mSegmentIndexDownloadStartTime;
    HttpConnection* mSegmentIndexConnection;
    DashSegment* mSegmentIndex;
    typedef FixedArray<int64_t, 3> DownloadTimes;
    DownloadTimes mDownloadTimesInMs;

    uint32_t mMaxAvgDownloadTimeMs;

    uint32_t mInitializationSegmentId;

    DashSegmentIndexState::Enum mSegmentIndexState;
    uint32_t mLastSegmentIndexId;
    uint32_t mSegmentIndexRetryCounter;
    int64_t mLastSegmentIndexRequest;

    bool_t mAutoFillCache;

    bool_t mIsEoS;
};
OMAF_NS_END
