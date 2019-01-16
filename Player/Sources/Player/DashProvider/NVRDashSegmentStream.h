
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

#include <IMPD.h>

#include "NVRNamespace.h"

#include "NVRErrorCodes.h"

#include "DashProvider/NVRDashMediaSegment.h"
#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRFixedQueue.h"

#include "Foundation/NVRMutex.h"

#include "Foundation/NVRHttpConnection.h"


OMAF_NS_BEGIN
    class DashSegment;
    static const uint32_t INVALID_SEGMENT_INDEX = OMAF_UINT32_MAX;
    static const time_t INVALID_START_TIME = 0; // Start time is fetched with time and 0 marks EPOCH
    static const uint32_t INITIAL_CACHE_DURATION_MS = 4000; //TODO configure based on the stream.
    static const uint32_t INITIAL_CACHE_SIZE_IN_ABR = 2;
    static const uint32_t RUNTIME_CACHE_DURATION_MS = 3000; //Milliseconds

    typedef FixedQueue<DashSegment*, 1024> DashSegments;

    class DashSegmentStreamObserver
    {
    public:
        virtual Error::Enum onMediaSegmentDownloaded(DashSegment *segment, float32_t aSpeedFactor) = 0;
        virtual void_t onSegmentIndexDownloaded(DashSegment *segment) = 0;
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

    typedef FixedArray<float32_t, 64> DownloadSpeedFactors;

    class DashSegmentStream :
        public DashSegmentObserver,
        public IHttpDataProcessor
    {
    public:

        DashSegmentStream(uint32_t bandwidth,
                          DashComponents dashComponents,
                          uint32_t aInitializationSegmentId,
                          DashSegmentStreamObserver *observer);

        virtual ~DashSegmentStream();

        void_t startDownload(time_t downloadStartTime, bool_t aIsIndependent);
        void_t startDownloadFromSegment(uint32_t overrideSegmentId, bool_t aIsIndependent);
        void_t startDownloadFrom(uint32_t overrideSegmentId, bool_t aIsIndependent);
        void_t startDownloadWithByteRange(uint32_t overrideSegmentId, uint64_t startByte, uint64_t endByte, bool_t aIsIndependent);

        virtual bool_t mpdUpdateRequired();

        void_t stopDownload();
        void_t stopDownloadAsync(bool_t aAbort);
        void_t stopSegmentIndexDownload();

        void_t clearDownloadedSegments();

        DashSegment* getInitSegment() const;

        static float32_t getDownloadSpeed();
        uint32_t getAvgDownloadTimeMs();
        uint32_t getInitializationSegmentId();

        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);
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
        void_t setBufferingTime(uint32_t aExpectedPingTimeMs);

        virtual uint32_t calculateSegmentId(uint64_t ptsUs) = 0;
        virtual bool_t hasFixedSegmentSize() const;

        virtual void_t processHttpData();
        virtual void_t processSegmentDownload(bool_t aForcedCacheUpdate);
        virtual void_t processSegmentIndexDownload(uint32_t segmentId);

    public: // DashSegmentObserver 
        void_t onSegmentDeleted();


    protected:

        virtual void downloadStopped();

        virtual bool_t waitForStreamHead();

        virtual dash::mpd::ISegment* getNextSegment(uint32_t &segmentId) = 0;
        virtual void_t downloadAborted() = 0;
        virtual bool_t isLastSegment() const;

        void_t shutdownStream();

    private:
        void_t updateDownloadTime(int64_t newDownloadTime);
        void_t startDownloader(time_t downloadStartTime, uint32_t overrideSegmentId, bool_t aIsIndependent);
        
    protected:
        MemoryAllocator &mMemoryAllocator;
        uint32_t mBandwidth;

        DashComponents mDashComponents;
        DashSegment* mInitializeSegment;
        std::vector<dash::mpd::IBaseUrl*> mBaseUrls; // Not owned.
        uint32_t mMaxCachedSegments;
        uint32_t mInitialCachedSegments;
        uint32_t mSegmentDurationInMs;
        uint64_t mTotalDurationMs;
        bool_t mSeekable;
        uint32_t mOverrideSegmentId;
        time_t mDownloadStartTime;

        uint32_t mRetryIndex;
        const DashSegmentStreamState::Enum state();
    private:
        DashSegmentStreamState::Enum mState;
        bool_t mRunning;
        int64_t mSegmentDownloadStartTime;
        int64_t mSegmentIndexDownloadStartTime;
        HttpConnection* mSegmentConnection;
        DashSegment* mNextSegment;
        HttpConnection* mSegmentIndexConnection;
        DashSegment* mSegmentIndex;
        DashSegmentStreamObserver *mObserver;
        typedef FixedArray<int64_t, 3> DownloadTimes;
        DownloadTimes mDownloadTimesInMs;

        uint32_t mCachedSegmentCount;
        uint32_t mDownloadRetryCounter;
        uint32_t mMaxAvgDownloadTimeMs;

        void processUninitialized();
        void processDownloadingInitSegment();
        void processIdleOrRetry();
        void processDownloadingMediaSegment();
        bool mInitialBuffering;
        int64_t mLastRequest;
        uint32_t mInitializationSegmentId;

        DashSegmentIndexState::Enum mSegmentIndexState;
        uint32_t mLastSegmentIndexId;
        uint32_t mSegmentIndexRetryCounter;
        int64_t mLastSegmentIndexRequest;
        void processIdleOrRetrySegmentIndex(uint32_t segmentId);
        void processDownloadingSegmentIndex(uint32_t segmentId);

        uint64_t mTotalBytesDownloaded;
        uint32_t mTotalSegmentsDownloaded;
        bool_t mAutoFillCache;

        struct ByteRange
        {
            uint64_t startByte;
            uint64_t endByte;
        } mDownloadByteRange;

        // static across all segment streams, to utilize global information rather than specific to this stream
        static DownloadSpeedFactors sDownloadSpeedFactors;
        static float32_t sAvgDownloadSpeedFactor;
        static size_t sDownloadsSinceLastFactor;
    };
OMAF_NS_END
