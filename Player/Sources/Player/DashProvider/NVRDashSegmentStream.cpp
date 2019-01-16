
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
static const uint32_t DEFAULT_SEGMENT_SIZE = 1000*1000;

// declare static members
float32_t DashSegmentStream::sAvgDownloadSpeedFactor;
DownloadSpeedFactors DashSegmentStream::sDownloadSpeedFactors;
size_t DashSegmentStream::sDownloadsSinceLastFactor;


DashSegmentStream::DashSegmentStream(
    uint32_t bandwidth, DashComponents dashComponents, uint32_t aInitializationSegmentId, DashSegmentStreamObserver* observer)
    : mMemoryAllocator(*MemorySystem::DefaultHeapAllocator())
    , mBandwidth(bandwidth)
    , mDashComponents(dashComponents)
    , mInitializeSegment(OMAF_NULL)
    , mMaxCachedSegments(4)
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
    , mInitialBuffering(false)
    , mTotalBytesDownloaded(0)
    , mTotalSegmentsDownloaded(0)
    , mAutoFillCache(true)
    , mDownloadByteRange({ 0,0 })
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

    // DashSegmentStreams are created when starting a new playback. Hence it is safe to reset the value here for a new playback instance, as all resets will be completed before any download takes place.
    // Value 0.0f would mean infite speed, so let's use a more realistic one as default value (2 ^= 1 sec segment downloaded in 0.5 sec).
    sAvgDownloadSpeedFactor = 2.0f;
    sDownloadSpeedFactors.clear();
    sDownloadsSinceLastFactor = 0;
}

DashSegmentStream::~DashSegmentStream()
{
    shutdownStream();
    DASH_LOG_SUM_D("Representation %s\tdownloaded\t%d\tsegments\t%llu\tbytes", mDashComponents.representation->GetId().c_str(), mTotalSegmentsDownloaded, mTotalBytesDownloaded);
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
    if ((mAutoFillCache && mCachedSegmentCount <= 1) || (mInitialBuffering))
    {
        OMAF_LOG_V("[%s] BUFFERING! cache: %d, initial %d", mDashComponents.representation->GetId().c_str(), mCachedSegmentCount, mInitialBuffering);
        return true;
    }
    else
    {
        return false;
    }
}

bool_t DashSegmentStream::isEndOfStream()
{
    if (mState == DashSegmentStreamState::END_OF_STREAM)
    {
        if (mAutoFillCache && mCachedSegmentCount <= 1) 
        {
            OMAF_LOG_V("DashSegmentStream is EoS");
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
    if (mRunning)
    {
        const HttpConnectionState::Enum& state = mSegmentConnection->getState().connectionState;
        OMAF_LOG_D("Running - httpconnection state = %d", state);
        mSegmentConnection->abortRequest();
        mSegmentConnection->waitForCompletion();
        if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
        {
            OMAF_DELETE_HEAP(mInitializeSegment);
            mInitializeSegment = NULL;
        }
        //only abortion of media segment download should be notified right? (since all the impls do subtract one from the current segment index, which does not contain the init segment.)            
        //mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted download.
        if (mNextSegment)
        {
            downloadAborted();
        }
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;

        //if make sure we go to correct state.. (if we have init segment we go to IDLE, if no init segment then UNINITIALIZED!)
        if (mInitializeSegment)
        {
            mState = DashSegmentStreamState::IDLE;
        }
        else
        {
            mState = DashSegmentStreamState::UNINITIALIZED;
        }
        mRunning = false;
    }
    else
    {
        OMAF_LOG_D("clearDownloadedSegments NOTRUNNING");
    }
}

void_t DashSegmentStream::stopDownload()
{
    if (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP)
    {
        processDownloadingMediaSegment();
    }
    if (mRunning)
    {
        //abort downloads..
        const HttpConnectionState::Enum& state = mSegmentConnection->getState().connectionState;
        mSegmentConnection->abortRequest();
        mSegmentConnection->waitForCompletion();
        if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
        {
            OMAF_DELETE_HEAP(mInitializeSegment);
            mInitializeSegment = NULL;
        }
        //only abortion of media segment download should be notified right? (since all the impls do subtract one from the current segment index, which does not contain the init segment.)            
        //mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted download.
        if (mNextSegment) downloadAborted();
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;

        mDownloadStartTime = INVALID_START_TIME;
        mState = DashSegmentStreamState::IDLE;//drop to idle..
        mRunning = false;
        mDownloadTimesInMs.clear();
    }
    else
    {
        OMAF_LOG_D("[%p] stopDownload NOTRUNNING", this);
    }
    downloadStopped();
}

void_t DashSegmentStream::stopDownloadAsync(bool_t aAbort)
{
    if (mRunning)
    {
        if (aAbort)
        {
            //abort downloads..
            mSegmentConnection->abortRequest();
            if (mSegmentConnection->hasCompleted())
            {
                OMAF_LOG_V("[%p] Abort completed", this);
                // completed
                if (mState == DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT)
                {
                    OMAF_DELETE_HEAP(mInitializeSegment);
                    mInitializeSegment = NULL;
                }

                //only abortion of media segment download should be notified right? (since all the impls do subtract one from the current segment index, which does not contain the init segment.)
                //mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted download.
                if (mNextSegment) downloadAborted();

                OMAF_DELETE_HEAP(mNextSegment);
                mNextSegment = OMAF_NULL;

                mDownloadStartTime = INVALID_START_TIME;
                mState = DashSegmentStreamState::IDLE;//drop to idle..
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
                    mState = DashSegmentStreamState::IDLE;//drop to idle..
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
            else
            {
                mState = DashSegmentStreamState::IDLE;//drop to idle..
                mRunning = false;
            }
        }
    }
    else
    {
        OMAF_LOG_D("[%p] stopDownload NOTRUNNING", this);
    }
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
    mInitialCachedSegments = max((uint32_t)(INITIAL_CACHE_DURATION_MS / mSegmentDurationInMs + 1), mMaxCachedSegments);
    OMAF_LOG_V("Initial cache %d", mInitialCachedSegments);
    mDownloadByteRange = { 0, 0 };
    startDownloader(downloadStartTime, INVALID_SEGMENT_INDEX, aIsIndependent);
}

void_t DashSegmentStream::startDownloadFromSegment(uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    startDownloadFrom(overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloadFrom(uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    // This call is done always when the playback is already ongoing, after ABR switch. Hence to minimize any buffering notes, use a smaller threshold for cache.
    mInitialCachedSegments = min((uint32_t)INITIAL_CACHE_SIZE_IN_ABR, mMaxCachedSegments);
    OMAF_LOG_V("Initial cache target: %d, current: %d", mInitialCachedSegments, mCachedSegmentCount);
    mDownloadByteRange = { 0, 0 };
    if (overrideSegmentId == 0)
    {
        OMAF_LOG_D("segment id is 0!!");
    }
    startDownloader(INVALID_START_TIME, overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloadWithByteRange(uint32_t overrideSegmentId, uint64_t startByte, uint64_t endByte, bool_t aIsIndependent)
{
    mDownloadByteRange = { startByte, endByte };
    startDownloader(INVALID_START_TIME, overrideSegmentId, aIsIndependent);
}

void_t DashSegmentStream::startDownloader(time_t downloadStartTime, uint32_t overrideSegmentId, bool_t aIsIndependent)
{
    if (mState == DashSegmentStreamState::ABORTING)
    {
        // do we have any other option than wait in this case?
        int32_t startTime = Time::getClockTimeMs();
        mSegmentConnection->waitForCompletion();
        OMAF_LOG_D("Restarting while abort in progress, waited %d ms", Time::getClockTimeMs() - startTime);

        if (mNextSegment) downloadAborted();
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;

        mDownloadStartTime = INVALID_START_TIME;

        mState = DashSegmentStreamState::IDLE;
    }
    else if (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP)
    {
        int32_t startTime = Time::getClockTimeMs();
        mSegmentConnection->waitForCompletion();
        OMAF_LOG_D("Restarting while async stop waiting for completion (not aborted), waited %d ms", Time::getClockTimeMs() - startTime);
        processDownloadingMediaSegment();
    }
    else if (mRunning)
    {
        // Download thread already started
        OMAF_ASSERT_UNREACHABLE();//technically should NEVER happen!
        OMAF_ASSERT(overrideSegmentId == mOverrideSegmentId, "startDownload called when already started AND with a different overridesegment!");
        OMAF_ASSERT(false, "startDownload called when already started!");
        return;
    }
    OMAF_ASSERT(overrideSegmentId != 0, "Trying to use 0 as override segment ID");//TODO this has happened
    mSegmentIndexState = DashSegmentIndexState::WAITING;
    mSegmentIndexConnection->abortRequest();
    mSegmentIndexConnection->waitForCompletion();
    OMAF_DELETE_HEAP(mSegmentIndex);
    mSegmentIndex = OMAF_NULL;

    mDownloadStartTime = downloadStartTime;
    // Reset the control flags
    mOverrideSegmentId = overrideSegmentId;
    if (!aIsIndependent)
    {
        // skip init segment, this is not an independent stream but a partial one. Go to idle state instead of uninitialized
        mState = DashSegmentStreamState::IDLE;
    }
    else if (mInitializeSegment)
    {
        //we have init segment already.. so just be idle.
        mState = DashSegmentStreamState::IDLE;
    }
    else
    {
        //no init segment, start from it.
        mState = DashSegmentStreamState::UNINITIALIZED;
    }
    mRunning = true;
    mInitialBuffering = true;
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
        mInitializeSegment = OMAF_NEW_HEAP(DashSegment)(DEFAULT_SEGMENT_SIZE/*estimated segmentsize in bytes*/, mInitializationSegmentId, mInitializationSegmentId, true, this);
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
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 && state.bytesDownloaded != 0)
    {
        mState = DashSegmentStreamState::IDLE;
    }
    else if (state.connectionState == HttpConnectionState::FAILED || state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_E("Initialization segment download failed");
        OMAF_DELETE_HEAP(mInitializeSegment);
        mInitializeSegment = OMAF_NULL;
        mState = DashSegmentStreamState::ERROR;
    }
    //TODO: should ABORTING and ABORTED be handled too? (should never be IDLE, and 99.999% of the time it should be IN_PROGRESS)

    /*
        IDLE,
        IN_PROGRESS,
        COMPLETED,  *
        FAILED,     *
        ABORTING,
        ABORTED,

    */
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
            mNextSegment = OMAF_NEW_HEAP(DashSegment)(mDownloadByteRange.startByte, (mDownloadByteRange.endByte - mDownloadByteRange.startByte) + 1, segmentId, mInitializationSegmentId, false, this);
            hl.setByteRange(mDownloadByteRange.startByte, mDownloadByteRange.endByte);
        }
        else
        {            
            mNextSegment = OMAF_NEW_HEAP(DashSegment)(DEFAULT_SEGMENT_SIZE/*estimated segment size in bytes*/, segmentId, mInitializationSegmentId, false, this);
        }
        mSegmentConnection->setHeaders(hl);
        mSegmentDownloadStartTime = Time::getClockTimeUs();
        if (HttpRequest::OK == mSegmentConnection->get(mNextSegment->getDataBuffer()))
        {
            DASH_LOG_D("\t%d\tstarted downloading repr\t%s\tsegment\t%d", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), segmentId);
            OMAF_LOG_V("%d started downloading repr\t%s\tsegment\t%d", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), segmentId);
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

void_t DashSegmentStream::processDownloadingMediaSegment()
{
    const HttpRequestState& state = mSegmentConnection->getState();
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 && state.bytesDownloaded != 0)
    {
        mDownloadRetryCounter = 0;
        mDownloadByteRange = { 0,0 };
        size_t bytesDownloaded = state.bytesDownloaded;
        int64_t downloadTimeMs = (Time::getClockTimeUs()- mSegmentDownloadStartTime)/1000;
        DashSegment* newSegment = mNextSegment;
        mNextSegment = OMAF_NULL;//ownership changes now..
        OMAF_LOG_D("Downloaded segment of %zd bytes took: %lld MS, bandwidth %u ", bytesDownloaded, downloadTimeMs, mBandwidth);
        mTotalBytesDownloaded += bytesDownloaded;
        mTotalSegmentsDownloaded++;

        mState = DashSegmentStreamState::IDLE;

        if (mObserver->onMediaSegmentDownloaded(newSegment, (float32_t)mSegmentDurationInMs/downloadTimeMs) != Error::OK)
        {
            // The segment could not be parsed. 
            // newSegment is deleted in observer
            OMAF_LOG_E("Segment parsing failed");
            mState = DashSegmentStreamState::ERROR;
            return;
        }
        newSegment->setIsCached();
        mCachedSegmentCount++;
        updateDownloadTime(downloadTimeMs);

        DASH_LOG_D("\t%d\tfinished downloading repr\t%s\tsegment\t%d\t\t%zd\tbytes, download time ms\t%d", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), newSegment->getSegmentId(), bytesDownloaded, downloadTime);

        OMAF_LOG_V("finished downloading repr %s segment %d, cached now %d", mDashComponents.representation->GetId().c_str(), newSegment->getSegmentId(), mCachedSegmentCount);
        if (!mAutoFillCache)
        {
            mInitialBuffering = false;
        }

    }
    else if (state.connectionState == HttpConnectionState::FAILED || state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_W("Failed to download segment for representation %s, http response %lld", mDashComponents.representation->GetId().c_str(), state.httpStatus);
        DASH_LOG_D("\t%d\failed downloading repr\t%s", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str());
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;
        if (state.httpStatus == 404 && isLastSegment())
        {
            // the last segment was not found => ignore the problem and continue without it. This is relevant with static template streams where indices are calculated based on total duration, but audio may have a slightly different duration than video
            mState = DashSegmentStreamState::END_OF_STREAM;
            return;
        }
        mState = DashSegmentStreamState::RETRY;
    }
}

void DashSegmentStream::processIdleOrRetrySegmentIndex(uint32_t segmentId)
{
    OMAF_LOG_D("SubSegment: preparing subsegment by downloading index, segmentId: %d", segmentId);
    dash::mpd::ISegment* segment = mDashComponents.segmentTemplate->GetMediaSegmentFromNumber(
                                        mBaseUrls,
                                        mDashComponents.representation->GetId(),
                                        mDashComponents.representation->GetBandwidth(),
                                        segmentId);
    if (segment == NULL)
    {
        OMAF_LOG_E("SubSegment: Segment creation failed");
        mSegmentIndexState = DashSegmentIndexState::SEGMENT_INDEX_ERROR;
    }
    else
    {
        mSegmentIndex= OMAF_NEW_HEAP(DashSegment)(SEGMENT_INDEX_RANGE_ENDBYTE+1/*estimated segmentsize in bytes*/, segmentId, mInitializationSegmentId, false, this);
        HttpHeaderList hl;
        hl.setByteRange(0, SEGMENT_INDEX_RANGE_ENDBYTE);
        mSegmentIndexConnection->setHeaders(hl);
        mSegmentIndexConnection->setUri(segment->AbsoluteURI().c_str());
        mSegmentIndexDownloadStartTime = Time::getClockTimeUs();
        if (HttpRequest::OK == mSegmentIndexConnection->get(mSegmentIndex->getDataBuffer()))
        {
            OMAF_LOG_D("SubSegment: Downloading segment index %d", segmentId);
            mSegmentIndexState = DashSegmentIndexState::DOWNLOADING_SEGMENT_INDEX;
            DASH_LOG_D("SubSegment: \t%d\tstarted downloading segment index repr\t%s\tsegment\t%d", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), segmentId);
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
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 && state.bytesDownloaded != 0)
    {
        mSegmentIndexRetryCounter = 0;
        mLastSegmentIndexId = mSegmentIndex->getSegmentId();
        size_t bytesDownloaded = state.bytesDownloaded;
        int64_t downloadTime = (Time::getClockTimeUs() - mSegmentIndexDownloadStartTime) / 1000;
        OMAF_LOG_V("SubSegment: Downloading segment index of %zd bytes took: %d MS, bandwidth %d ", bytesDownloaded, downloadTime, mBandwidth);

        
        DashSegment* newSegment = mSegmentIndex;
        mSegmentIndex = OMAF_NULL;//ownership changes now..
        DASH_LOG_D("SubSegment: \t%d\tfinished downloading repr\t%s\tsegment index\t%d\t\t%llu\tbytes, download time ms\t%d", Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), newSegmentIndex->getSegmentId(), bytesDownloaded, downloadTime);

        // will delete newSegment
        mObserver->onSegmentIndexDownloaded(newSegment);
        OMAF_LOG_V("SubSegment: Pushed new SegmentIndex");
        mSegmentIndexState = DashSegmentIndexState::WAITING;
    }
    else if (state.connectionState == HttpConnectionState::FAILED || state.connectionState == HttpConnectionState::COMPLETED)
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
            //if enough time has passed from last try...
            int64_t current = Time::getClockTimeMs();
            if (current < mLastRequest)
            {
                //wrap-around. (getClockTimeMs is 32 bit..)
                //not exactly perfect, but works.
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
            OMAF_LOG_E("Segment for %s download failed after %d retries", mDashComponents.representation->GetId().c_str(), mDownloadRetryCounter);
            mState = DashSegmentStreamState::ERROR;
        }
        break;
    }

    case DashSegmentStreamState::IDLE:
    {

        if (!aForcedCacheUpdate)
        {
            if (mInitialBuffering && mCachedSegmentCount >= mInitialCachedSegments)
            {
                mInitialBuffering = false;
            }

            if (mCachedSegmentCount >= mMaxCachedSegments)
            {
                //OMAF_LOG_E("%s cache full %d %d", mDashComponents.representation->GetId().c_str(), mCachedSegmentCount, mMaxCachedSegments);
                return;
            }
        }



        // Check if the stream is dynamic and requires a wait
        if (waitForStreamHead())
        {
            //if we get here we have downloaded ALL segments available, and we need to wait until MPD has been updated and new segments are available..
            return;
        }
        
        if (mAutoFillCache && !mInitialBuffering && mCachedSegmentCount <= 1)
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
            //only abortion of media segment download should be notified right? (since all the impls do subtract one from the current segment index, which does not contain the init segment.)
            //mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted download.
            OMAF_LOG_V("[%p] processSegmentDownload - aborting completed", this);
            if (mNextSegment) downloadAborted();
            OMAF_DELETE_HEAP(mNextSegment);
            mNextSegment = OMAF_NULL;

            mDownloadStartTime = INVALID_START_TIME;
            mState = DashSegmentStreamState::IDLE;//drop to idle..
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
        processDownloadingMediaSegment();
        mRunning = false;
        break;
    }
    default:
    {
        //what? fail now.
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
                //only abortion of media segment download should be notified right? (since all the impls do subtract one from the current segment index, which does not contain the init segment.)
                //mNextSegment is the currently downloading/downloaded(but not yet processed) ie. the aborted download.
                if (mNextSegment) downloadAborted();
                OMAF_DELETE_HEAP(mNextSegment);
                mNextSegment = OMAF_NULL;

                mDownloadStartTime = INVALID_START_TIME;
                mState = DashSegmentStreamState::IDLE;//drop to idle..
                mRunning = false;
            }
        }

        return; // Dont run segmentIndex logic if we are processing Media Segments
    }

    switch (mSegmentIndexState)
    {
        case DashSegmentIndexState::RETRY_SEGMENT_INDEX:
        {
            if (mSegmentIndexRetryCounter < MAX_RETRY_COUNT)
            {
                //if enough time has passed from last try...
                int64_t current = Time::getClockTimeMs();
                if (current < mLastSegmentIndexRequest)
                {
                    //wrap-around. (getClockTimeMs is 32 bit..)
                    //not exactly perfect, but works.
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
                return; // we have already downloaded this segmentId succesfully
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

    float32_t factor = (float32_t)mSegmentDurationInMs / newDownloadTime;
    if (sDownloadSpeedFactors.getSize() == sDownloadSpeedFactors.getCapacity())
    {
        sDownloadSpeedFactors.removeAt(0, 1);
    }
    sDownloadSpeedFactors.add(factor);
    sDownloadsSinceLastFactor++;
}

float32_t DashSegmentStream::getDownloadSpeed()
{
    if ((sDownloadSpeedFactors.getSize() > 1 && sDownloadSpeedFactors.getSize() <= 5) 
        || (sDownloadSpeedFactors.getSize() > 5 && sDownloadsSinceLastFactor > 5))
    {
        // sort the factors in increasing order, to get median of them
        DownloadSpeedFactors orderedFactors;
        for (DownloadSpeedFactors::ConstIterator it = sDownloadSpeedFactors.begin();
            it != sDownloadSpeedFactors.end(); ++it)
        {
            bool_t added = false;
            for (size_t i = 0; i < orderedFactors.getSize(); i++)
            {
                if ((*it) > orderedFactors.at(i))
                {
                    orderedFactors.add(*it, i);
                    added = true;
                    break;
                }
            }
            if (!added)
            {
                // add to the end
                orderedFactors.add(*it);
            }
        }
        // take one of the worst ones, excluding the very worst ones as outliers?
        size_t n = orderedFactors.getSize() - 4;
        sAvgDownloadSpeedFactor = orderedFactors.at(n);
        sDownloadsSinceLastFactor = 0;
        OMAF_LOG_V("New downloadSpeedFactor: %f", sAvgDownloadSpeedFactor);
    }
    else if (!sDownloadSpeedFactors.isEmpty())
    {
        sAvgDownloadSpeedFactor = sDownloadSpeedFactors.front();
    }
    return sAvgDownloadSpeedFactor;
}

uint32_t DashSegmentStream::getAvgDownloadTimeMs()
{
    uint32_t avgTimeMs = 0;
    uint32_t count = 0;
    for (DownloadTimes::ConstIterator it = mDownloadTimesInMs.begin();
        it != mDownloadTimesInMs.end(); ++it)
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
    avgTimeMs = max(MIN_DOWNLOAD_TIME, (uint32_t)(avgTimeMs / count) + 100); // add 100 ms safety margin, and use 500 ms as the minimum
    if (avgTimeMs > mMaxAvgDownloadTimeMs)
    {
        mMaxAvgDownloadTimeMs = avgTimeMs;
        mMaxCachedSegments = max(mMaxCachedSegments, (uint32_t)ceil(3.f*avgTimeMs / mSegmentDurationInMs));
        OMAF_LOG_V("Increase cache limit to %d, avg download time ms %d", mMaxCachedSegments, avgTimeMs);
    }
    return avgTimeMs;
}

uint32_t DashSegmentStream::getInitializationSegmentId()
{
    return mInitializationSegmentId;
}

Error::Enum DashSegmentStream::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
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
    return (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT || mState == DashSegmentStreamState::ABORTING || mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP);
}

bool_t DashSegmentStream::isCompletingDownload()
{
    return (mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT || mState == DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT_BEFORE_STOP);
}

bool_t DashSegmentStream::isActive()
{
    //if running.... i.e. between startDownload and stopDownload
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

void_t DashSegmentStream::setBufferingTime(uint32_t aExpectedPingTimeMs)
{
    // Set initial value based on round-trip delay measured from MPD; will be updated if average download time is more than expected or we run out of cache
    // 2 is min to have things running, as in practice there is 1 always in the parsing
    mMaxCachedSegments = max(2u, max(RUNTIME_CACHE_DURATION_MS/mSegmentDurationInMs, min(mMaxCachedSegments, (uint32_t)ceil(20.f*aExpectedPingTimeMs/mSegmentDurationInMs))));
    OMAF_LOG_V("MaxCachedSegments based on MPD ping time %d -> %d", aExpectedPingTimeMs, mMaxCachedSegments);
}

bool_t DashSegmentStream::hasFixedSegmentSize() const
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
    if (segment->getDataBuffer()->getDataPtr() != state.output->getDataPtr())
    {
        OMAF_LOG_D("Not processing segment as data pointer has changed!");
        return;
    }
}

bool_t DashSegmentStream::isLastSegment() const
{
    return false;
}

OMAF_NS_END

