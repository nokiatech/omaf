
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
#include "DashProvider/NVRDashSegmentStream.h"

#include "DashProvider/NVRDashLog.h"
#include "Foundation/NVRHttp.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashSegmentStream)
// Each retry takes one second
static const uint32_t MAX_RETRY_COUNT = 30;
static const uint32_t SEGMENT_INDEX_RANGE_ENDBYTE = 1023;
static const uint32_t DEFAULT_DOWNLOAD_TIME = 1000;
static const uint32_t MIN_DOWNLOAD_TIME = 500;
static const uint32_t DEFAULT_SEGMENT_SIZE = 1000 * 1000;
static const uint32_t ABS_MAX_CACHE_SIZE =
    15;  // with very short segments, too large cache derived from target buffering time can throttle the whole system
const uint32_t ABS_MIN_CACHE_BUFFERS =
    3u;  // In practice need to have at least 3 segments in cache to avoid buffering state: with 2 buffers at the
         // switching point one would be deleted before downloading a new one, and hence the download would take place
         // in buffering state
const uint32_t INVALID_SEGMENT_INDEX = OMAF_UINT32_MAX;
const time_t INVALID_START_TIME = 0;              // Start time is fetched with time and 0 marks EPOCH
const uint32_t RUNTIME_CACHE_DURATION_MS = 3000;  // Milliseconds
const uint32_t PREBUFFER_CACHE_LIMIT_MS = 2000;   // Milliseconds


DashSegmentStream::DashSegmentStream(uint32_t bandwidth,
                                     DashComponents dashComponents,
                                     uint32_t aInitializationSegmentId,
                                     DashSegmentStreamObserver* observer)
    : mMemoryAllocator(*MemorySystem::DefaultHeapAllocator())
    , mBandwidth(bandwidth)
    , mDashComponents(dashComponents)
    , mInitializeSegment(OMAF_NULL)
    , mMaxCachedSegments(4)
    , mPreBufferCachedSegments(ABS_MIN_CACHE_BUFFERS)
    , mAbsMaxCachedSegments(ABS_MAX_CACHE_SIZE)
    , mSegmentDurationInMs(0)
    , mTotalDurationMs(0)
    , mSeekable(false)
    , mOverrideSegmentId(0)
    , mState(DashSegmentStreamState::UNINITIALIZED)
    , mRunning(false)
    , mSegmentDownloadStartTime(0)
    , mSegmentIndexDownloadStartTime(0)
    , mSegmentConnection(OMAF_NULL)
    , mNextSegment(OMAF_NULL)
    , mSegmentIndexConnection(OMAF_NULL)
    , mSegmentIndex(OMAF_NULL)
    , mObserver(observer)
    , mCachedSegmentCount(0)
    , mDownloadRetryCounter(0)
    , mMaxAvgDownloadTimeMs(0)
    , mLastRequest(0)
    , mInitializationSegmentId(aInitializationSegmentId)
    , mSegmentIndexState(DashSegmentIndexState::WAITING)
    , mLastSegmentIndexId(0)
    , mSegmentIndexRetryCounter(0)
    , mLastSegmentIndexRequest(0)
    , mDownloadStartTime(INVALID_START_TIME)
    , mRetryIndex(INVALID_SEGMENT_INDEX)
    , mPreBuffering(false)
    , mTotalBytesDownloaded(0)
    , mTotalSegmentsDownloaded(0)
    , mAutoFillCache(true)
    , mDownloadByteRange({0, 0})
    , mBufferingTimeMs(0)
    , mIsEoS(false)
{
    OMAF_ASSERT(mDashComponents.mpd != OMAF_NULL, "");
    OMAF_ASSERT(mDashComponents.period != OMAF_NULL, "");
    OMAF_ASSERT(mDashComponents.adaptationSet != OMAF_NULL, "");
    OMAF_ASSERT(mDashComponents.representation != OMAF_NULL, "");
    OMAF_ASSERT(observer != OMAF_NULL, "");

    mBaseUrls = DashUtils::ResolveBaseUrl(mDashComponents);
    mSegmentConnection = Http::createHttpConnection(mMemoryAllocator);
    mSegmentConnection->setHttpDataProcessor(this);
    mSegmentIndexConnection = Http::createHttpConnection(mMemoryAllocator);
}

DashSegmentStream::~DashSegmentStream()
{
    shutdownStream();
    DASH_LOG_SUM_D("Representation %s\tdownloaded\t%d\tsegments\t%llu\tbytes",
                   mDashComponents.representation->GetId().c_str(), mTotalSegmentsDownloaded, mTotalBytesDownloaded);
    OMAF_DELETE(mMemoryAllocator, mSegmentConnection);
    OMAF_DELETE(mMemoryAllocator, mSegmentIndexConnection);

    OMAF_DELETE_HEAP(mNextSegment);
    mNextSegment = OMAF_NULL;
}

void_t DashSegmentStream::shutdownStream()
{
    stopDownload();
    stopSegmentIndexDownload();
    clearDownloadedSegments();
    OMAF_DELETE_HEAP(mInitializeSegment);
    mInitializeSegment = OMAF_NULL;
}

bool_t DashSegmentStream::mpdUpdateRequired()
{
    return false;
}

Error::Enum DashSegmentStream::updateMPD(DashComponents components)
{
    mDashComponents = components;
    mBaseUrls = DashUtils::ResolveBaseUrl(mDashComponents);

    return Error::OK;
}

uint32_t DashSegmentStream::cachedSegments() const
{
    return mCachedSegmentCount;
}

DashSegment* DashSegmentStream::getInitSegment() const
{
    return mInitializeSegment;
}

bool_t DashSegmentStream::isBuffering()
{
    if (isEndOfStream())
    {
        return false;
    }
    if ((mAutoFillCache && mCachedSegmentCount <= 1) || (mPreBuffering))
    {
        OMAF_LOG_V("[%s] BUFFERING! cache: %d, prebuffering %d", mDashComponents.representation->GetId().c_str(),
                   mCachedSegmentCount, mPreBuffering);
        return true;
    }
    else
    {
        return false;
    }
}

bool_t DashSegmentStream::isEndOfStream()
{
    if (mIsEoS)
    {
        return true;
    }
    if (mState == DashSegmentStreamState::END_OF_STREAM)
    {
        if (mAutoFillCache && mCachedSegmentCount <= 1)
        {
            OMAF_LOG_V("DashSegmentStream is EoS");
            mIsEoS = true;
            return true;
        }
        else
        {
            return false;
        }
    }
    return false;
}

bool_t DashSegmentStream::isError()
{
    return mState == DashSegmentStreamState::ERROR;
}

void_t DashSegmentStream::clearDownloadedSegments()
{
    OMAF_LOG_D("[%p] clearDownloadedSegments", this);
    if (mRunning || (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP))
    {
        const HttpConnectionState::Enum& state = mSegmentConnection->getState().connectionState;
        OMAF_LOG_D("Running - httpconnection state = %d", state);
        mSegmentConnection->abortRequest();
        mSegmentConnection->waitForCompletion();
        if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT ||
            mState == DashSegmentStreamState::UNINITIALIZED)
        {
            OMAF_DELETE_HEAP(mInitializeSegment);
            mInitializeSegment = NULL;
            mState = DashSegmentStreamState::UNINITIALIZED;
        }
        else
        {
            // only abortion of media segment download should be notified right? (since all the impls do subtract one
            // from the current segment index, which does not contain the init segment.) mNextSegment is the currently
            // downloading/downloaded(but not yet processed) ie. the aborted download.
            if (mNextSegment)
            {
                downloadAborted();
            }
            OMAF_DELETE_HEAP(mNextSegment);
            mNextSegment = OMAF_NULL;
            mState = DashSegmentStreamState::IDLE;
        }
        mRunning = false;
    }
    else
    {
        OMAF_LOG_D("clearDownloadedSegments NOTRUNNING");
        if (mState != DashSegmentStreamState::UNINITIALIZED)
        {
            mState = DashSegmentStreamState::IDLE;
        }
    }
}

void_t DashSegmentStream::stopDownload()
{
    if (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP)
    {
        processDownloadingMediaSegment(true);
    }
    if (mRunning)
    {
        // abort downloads..
        const HttpConnectionState::Enum& state = mSegmentConnection->getState().connectionState;
        mSegmentConnection->abortRequest();
        mSegmentConnection->waitForCompletion();
        if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT ||
            mState == DashSegmentStreamState::UNINITIALIZED)
        {
            OMAF_DELETE_HEAP(mInitializeSegment);
            mInitializeSegment = NULL;
            mState = DashSegmentStreamState::UNINITIALIZED;
        }
        else
        {
            // only abortion of media segment download should be notified right? (since all the impls do subtract one
            // from the current segment index, which does not contain the init segment.) mNextSegment is the currently
            // downloading/downloaded(but not yet processed) ie. the aborted download.
            if (mNextSegment)
            {
                downloadAborted();
            }
            OMAF_DELETE_HEAP(mNextSegment);
            mNextSegment = OMAF_NULL;
            mState = DashSegmentStreamState::IDLE;  // drop to idle..
        }

        mDownloadStartTime = INVALID_START_TIME;
        mRunning = false;
        mDownloadTimesInMs.clear();
    }
    else
    {
        OMAF_LOG_D("[%p] stopDownload NOTRUNNING", this);
    }
    downloadStopped();
    OMAF_LOG_V("[%p] stopDownload done", this);
}

void_t DashSegmentStream::stopDownloadAsync(bool_t aAbort)
{
    if (mRunning)
    {
        if (aAbort)
        {
            // abort downloads..
            mSegmentConnection->abortRequest();
            if (mSegmentConnection->hasCompleted())
            {
                OMAF_LOG_V("[%p] Abort completed", this);
                // completed
                if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT ||
                    mState == DashSegmentStreamState::UNINITIALIZED)
                {
                    OMAF_DELETE_HEAP(mInitializeSegment);
                    mInitializeSegment = NULL;
                    mState = DashSegmentStreamState::UNINITIALIZED;
                }
                else
                {
                    // only abortion of media segment download should be notified right? (since all the impls do
                    // subtract one from the current segment index, which does not contain the init segment.)
                    // mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted
                    // download.
                    if (mNextSegment)
                    {
                        downloadAborted();
                    }

                    OMAF_DELETE_HEAP(mNextSegment);
                    mNextSegment = OMAF_NULL;
                    mState = DashSegmentStreamState::IDLE;  // drop to idle..
                }
                mDownloadStartTime = INVALID_START_TIME;
                mRunning = false;
            }
            else
            {
                OMAF_LOG_V("[%p] Abort not completed", this);
                if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
                {
                    mSegmentConnection->waitForCompletion();
                    OMAF_DELETE_HEAP(mInitializeSegment);
                    mInitializeSegment = NULL;

                    mDownloadStartTime = INVALID_START_TIME;
                    mState = DashSegmentStreamState::UNINITIALIZED;
                    mRunning = false;
                }
                else
                {
                    OMAF_LOG_D("[%p] stopDownloadAsync - keep state in ABORTING", this);
                    mState = DashSegmentStreamState::ABORTING;
                }
            }
        }
        else
        {
            if (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT)
            {
                // else wait for the current download to complete
                OMAF_LOG_D("[%p] stopDownloadAsync - stop after download completed", this);
                mState = DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP;
            }
            else if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
            {
                OMAF_LOG_D("[%p] stopDownloadAsync while downloading init segment, keep it downloading", this);
                mRunning = false;
            }
            else if (mState != DashSegmentStreamState::UNINITIALIZED)
            {
                OMAF_LOG_V("[%p] drop to idle", this);
                mState = DashSegmentStreamState::IDLE;  // drop to idle..
                mRunning = false;
            }
            else
            {
                OMAF_LOG_V("stopDownloadAsync while in state %d", mState);
                mRunning = false;
            }
        }
    }
    else
    {
        OMAF_LOG_D("[%p] stopDownload NOTRUNNING", this);
    }
    OMAF_LOG_V("[%p] stopDownloadAsync done", this);
}

void_t DashSegmentStream::downloadStopped()
{
    // By default no need to do anything here
}

void_t DashSegmentStream::stopSegmentIndexDownload()
{
    mSegmentIndexState = DashSegmentIndexState::WAITING;
    mSegmentIndexConnection->abortRequest();
    mSegmentIndexConnection->waitForCompletion();
    OMAF_DELETE_HEAP(mSegmentIndex);
    mSegmentIndex = OMAF_NULL;
}

void_t DashSegmentStream::startDownload(time_t downloadStartTime, bool_t aIsIndependent)
{
    mDownloadByteRange = {0, 0};
    startDownloader(downloadStartTime, INVALID_SEGMENT_INDEX, aIsIndependent);
}

void_t DashSegmentStream::startDownloadFromSegment(uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    startDownloadFrom(overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloadFrom(uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    mDownloadByteRange = {0, 0};
    if (overrideSegmentId == 0)
    {
        OMAF_LOG_D("segment id is 0!!");
    }
    startDownloader(INVALID_START_TIME, overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloadWithByteRange(uint32_t overrideSegmentId,
                                                     uint64_t startByte,
                                                     uint64_t endByte,
                                                     bool_t aIsIndependent)
{
    mDownloadByteRange = {startByte, endByte};
    startDownloader(INVALID_START_TIME, overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloader(time_t downloadStartTime, uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    if (mState == DashSegmentStreamState::END_OF_STREAM)
    {
        OMAF_LOG_D("Trying to start a stream that has already reached EoS %s",
                   mDashComponents.representation->GetId().c_str());
        return;
    }
    else if (mState == DashSegmentStreamState::ERROR)
    {
        OMAF_LOG_D("Trying to start a stream that is in error state %s",
                   mDashComponents.representation->GetId().c_str());
        return;
    }
    else if (mState == DashSegmentStreamState::ABORTING)
    {
        // do we have any other option than wait in this case?
        int32_t startTime = Time::getClockTimeMs();
        mSegmentConnection->waitForCompletion();
        OMAF_LOG_D("Restarting %s while abort in progress, waited %d ms",
                   mDashComponents.representation->GetId().c_str(), Time::getClockTimeMs() - startTime);

        if (mNextSegment)
        {
            downloadAborted();
        }
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;

        mDownloadStartTime = INVALID_START_TIME;

        mState = DashSegmentStreamState::IDLE;
    }
    else if (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP)
    {
        int32_t startTime = Time::getClockTimeMs();
        mSegmentConnection->waitForCompletion();
        OMAF_LOG_D("Restarting %s while async stop waiting for completion (not aborted), waited %d ms",
                   mDashComponents.representation->GetId().c_str(), Time::getClockTimeMs() - startTime);
        // process, but ignore download time as it may have been waiting for a long time
        processDownloadingMediaSegment(true);
    }
    else if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
    {
        int32_t startTime = Time::getClockTimeMs();
        mSegmentConnection->waitForCompletion();
        OMAF_LOG_D(
            "Restarting %s while async stop waiting for completion of init segment download (not aborted), waited %d "
            "ms",
            mDashComponents.representation->GetId().c_str(), Time::getClockTimeMs() - startTime);
        processDownloadingInitSegment();
    }
    else if (mRunning)
    {
        // Download thread already started
        OMAF_ASSERT_UNREACHABLE();  // technically should NEVER happen!
        OMAF_ASSERT(overrideSegmentId == mOverrideSegmentId,
                    "startDownload called when already started AND with a different overridesegment!");
        OMAF_ASSERT(false, "startDownload called when already started!");
        return;
    }
    OMAF_ASSERT(overrideSegmentId != 0, "Trying to use 0 as override segment ID");
    mSegmentIndexState = DashSegmentIndexState::WAITING;
    mSegmentIndexConnection->abortRequest();
    mSegmentIndexConnection->waitForCompletion();
    OMAF_DELETE_HEAP(mSegmentIndex);
    mSegmentIndex = OMAF_NULL;

    mDownloadStartTime = downloadStartTime;
    // Reset the control flags
    mOverrideSegmentId = overrideSegmentId;
    if (!needInitSegment(aIsIndependent))
    {
        // skip init segment, this is not an independent stream but a partial one (except if it is ondemand =>
        // needInitSegment returns true). Go to idle state instead of uninitialized
        mState = DashSegmentStreamState::IDLE;
    }
    else if (mState == DashSegmentStreamState::IDLE && mInitializeSegment != OMAF_NULL)
    {
        // We have completed init segment downloaded already so just be idle.
        // Having mInitializeSegment != OMAF_NULL is not enough as it can also mean download is in progress)
        OMAF_LOG_V("we have init segment when starting downloader %s", mDashComponents.representation->GetId().c_str());
    }
    // else we should be either in uninitialized or downloading_init_segment state
    else
    {
        OMAF_ASSERT(mState == DashSegmentStreamState::UNINITIALIZED ||
                        mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT,
                    "Unexpected state");
    }
    mRunning = true;
    mPreBuffering = true;
}

void_t DashSegmentStream::onSegmentDeleted()
{
    if (mCachedSegmentCount > 0)
    {
        mCachedSegmentCount--;
        OMAF_LOG_V("Segment deleted, current cached segment count: %d", mCachedSegmentCount);
    }
    // else when init segment gets deleted, we skip the above, since mCachedSegmentCount does not include init segment
}

void_t DashSegmentStream::processUninitialized()
{
    // Initialization is still not downloaded. Create a new ISegment.
    dash::mpd::ISegment* initSegment = NULL;
    const dash::mpd::IURLType* init = mDashComponents.segmentTemplate->GetInitialization();
    if (init != NULL)
    {
        initSegment = init->ToSegment(mBaseUrls);
    }
    else
    {
        uint64_t bandwidth = mDashComponents.representation->GetBandwidth();
        const std::string& id = mDashComponents.representation->GetId();
        initSegment = mDashComponents.segmentTemplate->ToInitializationSegment(mBaseUrls, id, bandwidth);
    }
    if (initSegment)
    {
        mInitializeSegment = OMAF_NEW_HEAP(DashSegment)(DEFAULT_SEGMENT_SIZE /*estimated segmentsize in bytes*/,
                                                        mInitializationSegmentId, mInitializationSegmentId, true, this);
        HttpHeaderList hl;
        mSegmentConnection->setHeaders(hl);
        mSegmentConnection->setUri(initSegment->AbsoluteURI().c_str());
        if (HttpRequest::OK == mSegmentConnection->get(mInitializeSegment->getDataBuffer()))
        {
            OMAF_LOG_D("Stream bandwidth %d: Downloading init segment", mBandwidth);
            mState = DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT;
        }
        else
        {
            OMAF_LOG_E("Stream bandwidth %d: Could not start init segment download", mBandwidth);
            mState = DashSegmentStreamState::ERROR;
        }
        delete initSegment;
    }
    else
    {
        OMAF_LOG_E("Initialization segment creation failed");
        mState = DashSegmentStreamState::ERROR;
    }
}

void_t DashSegmentStream::processDownloadingInitSegment()
{
    const HttpRequestState& state = mSegmentConnection->getState();
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 &&
        state.bytesDownloaded != 0)
    {
        mState = DashSegmentStreamState::IDLE;
    }
    else if (state.connectionState == HttpConnectionState::FAILED ||
             state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_E("Initialization segment download failed");
        OMAF_DELETE_HEAP(mInitializeSegment);
        mInitializeSegment = OMAF_NULL;
        mState = DashSegmentStreamState::ERROR;
    }
    // IN_PROGRESS)

    /*
        IDLE,
        IN_PROGRESS,
        COMPLETED,  *
        FAILED,     *
        ABORTING,
        ABORTED,

    */
}

void_t DashSegmentStream::completePendingDownload()
{
    if (isCompletingDownload())
    {
        int32_t startTime = Time::getClockTimeMs();
        OMAF_LOG_V("completePendingDownload for %s start", mDashComponents.representation->GetId().c_str());
        mSegmentConnection->waitForCompletion();

        processDownloadingMediaSegment(true);
        OMAF_LOG_V("completePendingDownload done for %s, took %d", mDashComponents.representation->GetId().c_str(),
                   Time::getClockTimeMs() - startTime);
        mRunning = false;
    }
}
void_t DashSegmentStream::processIdleOrRetry()
{
    uint32_t segmentId = 0;
    dash::mpd::ISegment* segment = getNextSegment(segmentId);
    if (segment == NULL)
    {
        if (segmentId == 0)
        {
            OMAF_LOG_D("Reached EoS");
            mState = DashSegmentStreamState::END_OF_STREAM;
        }
        else
        {
            OMAF_LOG_E("Segment creation failed");
            mState = DashSegmentStreamState::ERROR;
        }
    }
    else
    {
        mSegmentConnection->setUri(segment->AbsoluteURI().c_str());
        HttpHeaderList hl;
        if (mDownloadByteRange.endByte != 0)
        {
            mNextSegment = OMAF_NEW_HEAP(DashSegment)(mDownloadByteRange.startByte,
                                                      (mDownloadByteRange.endByte - mDownloadByteRange.startByte) + 1,
                                                      segmentId, mInitializationSegmentId, false, this);
            hl.setByteRange(mDownloadByteRange.startByte, mDownloadByteRange.endByte);
        }
        else
        {
            mNextSegment = OMAF_NEW_HEAP(DashSegment)(DEFAULT_SEGMENT_SIZE /*estimated segment size in bytes*/,
                                                      segmentId, mInitializationSegmentId, false, this);
        }
        mSegmentConnection->setHeaders(hl);
        mSegmentDownloadStartTime = Time::getClockTimeUs();
        if (HttpRequest::OK == mSegmentConnection->get(mNextSegment->getDataBuffer()))
        {
            DASH_LOG_D("\t%d\tstarted downloading repr\t%s\tsegment\t%d", Time::getClockTimeMs(),
                       mDashComponents.representation->GetId().c_str(), segmentId);
            OMAF_LOG_V("%d started downloading repr\t%s\tsegment\t%d", Time::getClockTimeMs(),
                       mDashComponents.representation->GetId().c_str(), segmentId);
            mState = DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT;
        }
        else
        {
            OMAF_LOG_E("Stream bandwidth %d: Could not start segment download", mBandwidth);
            mState = DashSegmentStreamState::ERROR;
            OMAF_DELETE_HEAP(mNextSegment);
            mNextSegment = OMAF_NULL;
        }
        delete segment;
    }
    mLastRequest = Time::getClockTimeMs();
}

void_t DashSegmentStream::processDownloadingMediaSegment(bool_t aLateReception)
{
    const HttpRequestState& state = mSegmentConnection->getState();
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 &&
        state.bytesDownloaded != 0)
    {
        mDownloadRetryCounter = 0;
        mDownloadByteRange = {0, 0};
        size_t bytesDownloaded = state.bytesDownloaded;
        int64_t downloadTimeMs = (Time::getClockTimeUs() - mSegmentDownloadStartTime) / 1000;
        mTotalBytesDownloaded += bytesDownloaded;

        mState = DashSegmentStreamState::IDLE;
        if (aLateReception)
        {
            downloadTimeMs = 0;
        }
        handleDownloadedSegment(downloadTimeMs, bytesDownloaded);

        // DASH_LOG_D("\t%d\tfinished downloading repr\t%s\tsegment\t%d\t\t%zd\tbytes, download time ms\t%d",
        // Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), newSegment->getSegmentId(),
        // bytesDownloaded, downloadTimeMs);

        if (!mAutoFillCache)
        {
            mPreBuffering = false;
        }
    }
    else if (state.connectionState == HttpConnectionState::FAILED ||
             state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_W("Failed to download segment for representation %s, http response %lld",
                   mDashComponents.representation->GetId().c_str(), state.httpStatus);
        DASH_LOG_D("\t%d\failed downloading repr\t%s", Time::getClockTimeMs(),
                   mDashComponents.representation->GetId().c_str());
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;
        if (aLateReception)
        {
            // ignore failures
            OMAF_LOG_W("Pending http request ended with a failure, ignoring");
            mState = DashSegmentStreamState::IDLE;
        }
        else
        {
            if (state.httpStatus == 404 && isLastSegment())
            {
                // the last segment was not found => ignore the problem and continue without it. This is relevant with
                // static template streams where indices are calculated based on total duration, but audio may have a
                // slightly different duration than video
                mState = DashSegmentStreamState::END_OF_STREAM;
                return;
            }
            mState = DashSegmentStreamState::RETRY;
        }
    }
}

void_t DashSegmentStream::handleDownloadedSegment(int64_t aDownloadTimeMs, size_t aBytesDownloaded)
{
    DashSegment* newSegment = mNextSegment;
    mNextSegment = OMAF_NULL;  // ownership changes now..
    mTotalSegmentsDownloaded++;

    float32_t factor = 0.f;
    if (aDownloadTimeMs > 0)
    {
        factor = (float32_t) mSegmentDurationInMs / aDownloadTimeMs;
        updateDownloadTime(aDownloadTimeMs);
        OMAF_LOG_ABR("DOWNLOAD(Time/Repr/SegmentID/Bytes/DownloadTime/cached) %d %s %d %d %d %d",
                     Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(),
                     newSegment->getSegmentId(), aBytesDownloaded, aDownloadTimeMs, mCachedSegmentCount);
    }
    else
    {
        // the segment was pending for processing, the downloadtimeMs is not valid
        OMAF_LOG_ABR("DOWNLOAD(Time/Repr/SegmentID/Bytes/DownloadTime/cached) %d %s %d %d %d %d",
                     Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(),
                     newSegment->getSegmentId(), aBytesDownloaded, -1, mCachedSegmentCount);
    }
    if (mObserver->onMediaSegmentDownloaded(newSegment, factor) != Error::OK)
    {
        // The segment could not be parsed.
        // newSegment is deleted in observer
        OMAF_LOG_E("[%p] Segment parsing failed", this);
        mState = DashSegmentStreamState::ERROR;
        return;
    }
    newSegment->setIsCached();
    mCachedSegmentCount++;
    OMAF_LOG_V("finished downloading repr %s segment %d, cached now %d",
               mDashComponents.representation->GetId().c_str(), newSegment->getSegmentId(), mCachedSegmentCount);
}

void DashSegmentStream::processIdleOrRetrySegmentIndex(uint32_t segmentId)
{
    OMAF_LOG_D("SubSegment: preparing subsegment by downloading index, segmentId: %d", segmentId);
    dash::mpd::ISegment* segment = mDashComponents.segmentTemplate->GetMediaSegmentFromNumber(
        mBaseUrls, mDashComponents.representation->GetId(), mDashComponents.representation->GetBandwidth(), segmentId);
    if (segment == NULL)
    {
        OMAF_LOG_E("SubSegment: Segment creation failed");
        mSegmentIndexState = DashSegmentIndexState::SEGMENT_INDEX_ERROR;
    }
    else
    {
        mSegmentIndex = OMAF_NEW_HEAP(DashSegment)(SEGMENT_INDEX_RANGE_ENDBYTE + 1 /*estimated segmentsize in bytes*/,
                                                   segmentId, mInitializationSegmentId, false, this);
        HttpHeaderList hl;
        hl.setByteRange(0, SEGMENT_INDEX_RANGE_ENDBYTE);
        mSegmentIndexConnection->setHeaders(hl);
        mSegmentIndexConnection->setUri(segment->AbsoluteURI().c_str());
        mSegmentIndexDownloadStartTime = Time::getClockTimeUs();
        if (HttpRequest::OK == mSegmentIndexConnection->get(mSegmentIndex->getDataBuffer()))
        {
            OMAF_LOG_D("SubSegment: Downloading segment index %d", segmentId);
            mSegmentIndexState = DashSegmentIndexState::DOWNLOADING_SEGMENT_INDEX;
            DASH_LOG_D("SubSegment: \t%d\tstarted downloading segment index repr\t%s\tsegment\t%d",
                       Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), segmentId);
        }
        else
        {
            OMAF_LOG_E("SubSegment: Could not start segment index download");
            mSegmentIndexState = DashSegmentIndexState::SEGMENT_INDEX_ERROR;
            OMAF_DELETE_HEAP(mSegmentIndex);
            mSegmentIndex = OMAF_NULL;
        }
        delete segment;
    }
    mLastSegmentIndexRequest = Time::getClockTimeMs();
}

void_t DashSegmentStream::processDownloadingSegmentIndex(uint32_t segmentId)
{
    const HttpRequestState& state = mSegmentIndexConnection->getState();
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 &&
        state.bytesDownloaded != 0)
    {
        mSegmentIndexRetryCounter = 0;
        mLastSegmentIndexId = mSegmentIndex->getSegmentId();
        size_t bytesDownloaded = state.bytesDownloaded;
        int64_t downloadTime = (Time::getClockTimeUs() - mSegmentIndexDownloadStartTime) / 1000;
        OMAF_LOG_V("SubSegment: Downloading segment index of %zd bytes took: %d MS, bandwidth %d ", bytesDownloaded,
                   downloadTime, mBandwidth);


        DashSegment* newSegment = mSegmentIndex;
        mSegmentIndex = OMAF_NULL;  // ownership changes now..
        DASH_LOG_D(
            "SubSegment: \t%d\tfinished downloading repr\t%s\tsegment index\t%d\t\t%llu\tbytes, download time ms\t%d",
            Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), newSegmentIndex->getSegmentId(),
            bytesDownloaded, downloadTime);

        // will delete newSegment
        mObserver->onSegmentIndexDownloaded(newSegment);
        OMAF_LOG_V("SubSegment: Pushed new SegmentIndex");
        mSegmentIndexState = DashSegmentIndexState::WAITING;
    }
    else if (state.connectionState == HttpConnectionState::FAILED ||
             state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_E("SubSegment: FAILED to download segmentIndex: %d", mSegmentIndex->getSegmentId());
        OMAF_DELETE_HEAP(mSegmentIndex);
        mSegmentIndex = OMAF_NULL;
        mSegmentIndexState = DashSegmentIndexState::RETRY_SEGMENT_INDEX;
    }
}

void_t DashSegmentStream::processSegmentDownload(bool_t aForcedCacheUpdate)
{
    if (!mRunning)
    {
        return;
    }
    switch (mState)
    {
    case DashSegmentStreamState::UNINITIALIZED:
    {
        processUninitialized();
        break;
    }

    case DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT:
    {
        processDownloadingInitSegment();
        break;
    }

    case DashSegmentStreamState::RETRY:
    {
        if (mDownloadRetryCounter < MAX_RETRY_COUNT)
        {
            // if enough time has passed from last try...
            int64_t current = Time::getClockTimeMs();
            if (current < mLastRequest)
            {
                // wrap-around. (getClockTimeMs is 32 bit..)
                // not exactly perfect, but works.
                mLastRequest = Time::getClockTimeMs();
            }
            if ((current - mLastRequest) >= 1000)
            {
                mDownloadRetryCounter++;
                processIdleOrRetry();
            }
        }
        else
        {
            OMAF_LOG_E("Segment for %s download failed after %d retries",
                       mDashComponents.representation->GetId().c_str(), mDownloadRetryCounter);
            mState = DashSegmentStreamState::ERROR;
        }
        break;
    }

    case DashSegmentStreamState::IDLE:
    {
        if (!aForcedCacheUpdate)
        {
            if (mPreBuffering && mCachedSegmentCount >= mPreBufferCachedSegments)
            {
                mPreBuffering = false;
            }

            if (mCachedSegmentCount >= mMaxCachedSegments)
            {
                // OMAF_LOG_E("%s cache full %d %d", mDashComponents.representation->GetId().c_str(),
                // mCachedSegmentCount,
                //           mMaxCachedSegments);
                return;
            }
        }


        // Check if the stream is dynamic and requires a wait
        if (waitForStreamHead())
        {
            // if we get here we have downloaded ALL segments available, and we need to wait until MPD has been updated
            // and new segments are available..
            return;
        }

        if (mAutoFillCache && !mPreBuffering && (mCachedSegmentCount * mSegmentDurationInMs < mMaxAvgDownloadTimeMs))
        {
            // report buffering warning to bitrate controller
            mObserver->onCacheWarning();
            // check avg download time and update cache if necessary
            getAvgDownloadTimeMs();
            OMAF_LOG_W("Running out of cache!");
        }

        processIdleOrRetry();
        break;
    }
    case DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT:
    {
        processDownloadingMediaSegment();
        break;
    }
    case DashSegmentStreamState::ERROR:
    {
        OMAF_LOG_D("Error!");
        mRunning = false;
        break;
    }
    case DashSegmentStreamState::END_OF_STREAM:
    {
        mRunning = false;
        break;
    }
    case DashSegmentStreamState::ABORTING:
    {
        if (mSegmentConnection->hasCompleted())
        {
            // only abortion of media segment download should be notified right? (since all the impls do subtract one
            // from the current segment index, which does not contain the init segment.) mNextSegment is the currently
            // downloading/downloaded(but not yet processed) ie. the aborted download.
            OMAF_LOG_V("[%p] processSegmentDownload - aborting completed", this);
            if (mNextSegment)
            {
                downloadAborted();
            }
            OMAF_DELETE_HEAP(mNextSegment);
            mNextSegment = OMAF_NULL;

            mDownloadStartTime = INVALID_START_TIME;
            mState = DashSegmentStreamState::IDLE;  // drop to idle..
            mRunning = false;
        }
        else
        {
            OMAF_LOG_V("[%p] abort still not completed", this);
        }
        break;
    }
    case DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP:
    {
        if (mSegmentConnection->hasCompleted())
        {
            mRunning = false;
        }
        processDownloadingMediaSegment();
        break;
    }
    default:
    {
        // what? fail now.
        OMAF_LOG_D("Unknown Error from state: %d", mState);
        mState = DashSegmentStreamState::ERROR;
        mRunning = false;
        break;
    }
    }
}

void_t DashSegmentStream::processSegmentIndexDownload(uint32_t segmentId)
{
    if (mRunning)
    {
        // check if we are in aborting state
        if (mState == DashSegmentStreamState::ABORTING)
        {
            OMAF_LOG_D("Aborting, check if ready");
            if (mSegmentConnection->hasCompleted())
            {
                // only abortion of media segment download should be notified right? (since all the impls do subtract
                // one from the current segment index, which does not contain the init segment.) mNextSegment is the
                // currently downloading/downloaded(but not yet processed) ie. the aborted download.
                if (mNextSegment)
                {
                    downloadAborted();
                }
                OMAF_DELETE_HEAP(mNextSegment);
                mNextSegment = OMAF_NULL;

                mDownloadStartTime = INVALID_START_TIME;
                mState = DashSegmentStreamState::IDLE;  // drop to idle..
                mRunning = false;
            }
        }

        return;  // Dont run segmentIndex logic if we are processing Media Segments
    }

    switch (mSegmentIndexState)
    {
    case DashSegmentIndexState::RETRY_SEGMENT_INDEX:
    {
        if (mSegmentIndexRetryCounter < MAX_RETRY_COUNT)
        {
            // if enough time has passed from last try...
            int64_t current = Time::getClockTimeMs();
            if (current < mLastSegmentIndexRequest)
            {
                // wrap-around. (getClockTimeMs is 32 bit..)
                // not exactly perfect, but works.
                mLastSegmentIndexRequest = Time::getClockTimeMs();
            }
            if ((current - mLastSegmentIndexRequest) >= 1000)
            {
                mSegmentIndexRetryCounter++;
                processIdleOrRetrySegmentIndex(segmentId);
            }
        }
        else
        {
            // Just give up... fallback into downloading full segment later.
            mSegmentIndexState = DashSegmentIndexState::WAITING;
            mSegmentIndexRetryCounter = 0;
            mLastSegmentIndexId = segmentId;
        }
        break;
    }
    case DashSegmentIndexState::WAITING:
    {
        if (segmentId == mLastSegmentIndexId)
        {
            return;  // we have already downloaded this segmentId succesfully
        }
        processIdleOrRetrySegmentIndex(segmentId);
        break;
    }
    case DashSegmentIndexState::DOWNLOADING_SEGMENT_INDEX:
    {
        processDownloadingSegmentIndex(segmentId);
        break;
    }
    case DashSegmentIndexState::SEGMENT_INDEX_ERROR:
    {
        OMAF_LOG_D("SubSegment: Error! processSegmentIndexDownload");
        break;
    }
    default:
    {
        OMAF_LOG_D("SubSegment: Error! Unsupported state, DashSegmentIndexState: %d", mSegmentIndexState);
        mSegmentIndexState = DashSegmentIndexState::SEGMENT_INDEX_ERROR;
        break;
    }
    }
}


bool_t DashSegmentStream::waitForStreamHead()
{
    return false;
}

void_t DashSegmentStream::updateDownloadTime(int64_t newDownloadTime)
{
    if (mDownloadTimesInMs.getSize() == mDownloadTimesInMs.getCapacity())
    {
        mDownloadTimesInMs.removeAt(0, 1);
    }
    mDownloadTimesInMs.add(newDownloadTime);
}

uint32_t DashSegmentStream::getAvgDownloadTimeMs()
{
    uint32_t avgTimeMs = 0;
    uint32_t count = 0;
    for (DownloadTimes::ConstIterator it = mDownloadTimesInMs.begin(); it != mDownloadTimesInMs.end(); ++it)
    {
        avgTimeMs += (uint32_t)(*it);
        count++;
    }
    if (count < 3)
    {
        while (count < 3)
        {
            avgTimeMs += DEFAULT_DOWNLOAD_TIME;
            count++;
        }
    }
    // add 100 ms safety margin, and use 500 ms as the minimum
    avgTimeMs = max(MIN_DOWNLOAD_TIME, (uint32_t)(avgTimeMs / count) + 100);
    if (avgTimeMs > mMaxAvgDownloadTimeMs)
    {
        mMaxAvgDownloadTimeMs = avgTimeMs;
        mMaxCachedSegments = max(mMaxCachedSegments, (uint32_t) ceil(3.f * avgTimeMs / mSegmentDurationInMs));
        OMAF_LOG_V("Increase cache limit to %d, avg download time ms %d", mMaxCachedSegments, avgTimeMs);
    }
    return avgTimeMs;
}

uint32_t DashSegmentStream::getInitializationSegmentId()
{
    return mInitializationSegmentId;
}

Error::Enum DashSegmentStream::seekToMs(uint64_t& aSeekToTargetMs,
                                        uint64_t& aSeekToResultMs,
                                        uint32_t& aSeekSegmentIndex)
{
    return Error::NOT_SUPPORTED;
}

bool_t DashSegmentStream::isSeekable()
{
    return mSeekable;
}

uint64_t DashSegmentStream::durationMs()
{
    return mTotalDurationMs;
}

uint64_t DashSegmentStream::segmentDurationMs()
{
    return mSegmentDurationInMs;
}

const DashSegmentStreamState::Enum DashSegmentStream::state()
{
    return mState;
};

bool_t DashSegmentStream::isDownloading()
{
    return (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT || mState == DashSegmentStreamState::ABORTING ||
            mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP);
}

bool_t DashSegmentStream::isCompletingDownload()
{
    return (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT ||
            mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP);
}

bool_t DashSegmentStream::isActive()
{
    // if running.... i.e. between startDownload and stopDownload
    return mRunning;
}

uint64_t DashSegmentStream::getDownloadedBytes() const
{
    return mTotalBytesDownloaded;
}

void_t DashSegmentStream::setCacheFillMode(bool_t autoFill)
{
    mAutoFillCache = autoFill;
    if (!mAutoFillCache)
    {
        mMaxCachedSegments = 1;
    }
}

void_t DashSegmentStream::setBufferingTime(uint32_t aBufferingTimeMs)
{
    if (!mAutoFillCache)
    {
        return;
    }
    if (mSegmentDurationInMs > 0)
    {
        // Set initial value based on round-trip delay measured from MPD; will be updated if average download time is
        // more than expected or we run out of cache 2 is min to have things running, as in practice there is 1 always
        // in the parsing
        uint32_t cachedSegments = min(ABS_MAX_CACHE_SIZE, (uint32_t) ceil(aBufferingTimeMs / mSegmentDurationInMs));
        if (cachedSegments > mPreBufferCachedSegments)
        {
            mMaxCachedSegments = cachedSegments;
            OMAF_LOG_V("MaxCachedSegments = %d based on requested buffering time %d", aBufferingTimeMs,
                       mMaxCachedSegments);
        }
        else
        {
            OMAF_LOG_V("MaxCachedSegments not modified");
        }
    }
    else
    {
        // save the time and use it once we know the segment duration
        mBufferingTimeMs = aBufferingTimeMs;
    }
}

bool_t DashSegmentStream::hasFixedSegmentSize() const
{
    return false;
}

bool_t DashSegmentStream::setLatencyReq(LatencyRequirement::Enum aType)
{
    return false;
}

void_t DashSegmentStream::processHttpData()
{
    const HttpRequestState& state = mSegmentConnection->getState();
    DashSegment* segment = mNextSegment;
    if (segment == OMAF_NULL)
    {
        // Test if this is an init segment
        if (mInitializeSegment == OMAF_NULL)
        {
            return;
        }
        segment = mInitializeSegment;
    }
    if (state.connectionState == HttpConnectionState::COMPLETED &&
        segment->getDataBuffer()->getDataPtr() != state.output->getDataPtr())
    {
        OMAF_LOG_D("Not processing segment as data pointer has changed!");
        return;
    }
}

bool_t DashSegmentStream::isLastSegment() const
{
    return false;
}

bool_t DashSegmentStream::needInitSegment(bool_t aIsIndependent)
{
    return aIsIndependent;
}

void_t DashSegmentStream::setLoopingOn()
{
    // ignore the request
}

OMAF_NS_END
