
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
#include "DashProvider/NVRDashSegmentStreamOnDemand.h"

#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRTime.h"
#include "Media/NVRMP4Parser.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashSegmentStreamOnDemand)

static const uint32_t SEGMENT_INDEX_LENGTH = 5000;
// ftyp+moov+sidx (+free); typical moov sizes 16-25k on OMAF HEVC VD extractor streams, but 1-2K for media;
// moov size depends on # of tracks, not on track duration; doesn't make sense to buffer too much media as it increases
// viewport switch latency

class DummyCreator : public MP4StreamCreator
{
public:
    MP4MediaStream* createStream(MediaFormat* aFormat) const
    {
        return OMAF_NULL;
    };
    MP4VideoStream* createVideoStream(MediaFormat* aFormat) const
    {
        return OMAF_NULL;
    };

    MP4AudioStream* createAudioStream(MediaFormat* aFormat) const
    {
        return OMAF_NULL;
    };
    void_t destroyStream(MP4MediaStream* aStream) const {};
};

static DummyCreator sCreator;

DashSegmentStreamOnDemand::DashSegmentStreamOnDemand(uint32_t bandwidth,
                                                     DashComponents dashComponents,
                                                     uint32_t aInitializationSegmentId,
                                                     DashSegmentStreamObserver* observer)
    : DashSegmentStream(bandwidth, dashComponents, aInitializationSegmentId, observer)
    , mIndexForSeek(INVALID_SEGMENT_INDEX)
    , mInitSegmentSize(SEGMENT_INDEX_LENGTH)
    , mLoopingOn(false)
    , mLatencyReq(LatencyRequirement::LOW_LATENCY)
    , mLoopedSegmentIndex(0)
    , mOverridePositionUs(0)
{
    mSeekable = true;
    mStartIndex = 1;
    mCurrentSegmentIndex = mStartIndex;
    mTotalDurationMs = DashUtils::getStreamDurationInMilliSeconds(dashComponents);
    mSidxParser = OMAF_NEW_HEAP(MP4VRParser)(sCreator, dashComponents.parserCtx);  // for segment index parsing only
    // The default value for segments merging is 1, so no merging. It always has to be explicitly turned on
    // If initial phase segments wanted to be merged for all stream types, need to make sure the foreground tiles get
    // shorter segments later, even if viewport is not changed
    mNrSegmentsMerged = mNrSegmentsMergedTarget = 1;
}

DashSegmentStreamOnDemand::~DashSegmentStreamOnDemand()
{
    shutdownStream();

    OMAF_DELETE_HEAP(mSidxParser);
}

void_t DashSegmentStreamOnDemand::processUninitialized()
{
    mInitializeSegment =
        OMAF_NEW_HEAP(DashSegment)(mInitSegmentSize /*estimated segmentsize in bytes*/, 1, 0, true, this);
    HttpHeaderList hl;
    hl.setByteRange(0, mInitSegmentSize - 1);
    mSegmentConnection->setHeaders(hl);
    // Combine the base URL (e.g. the URL of the MPD) + the last found URL (last = representation level) ==
    // representation URL
    mFullBaseUrl = (mBaseUrls.front()->GetUrl() + mBaseUrls.back()->GetUrl()).c_str();
    mSegmentConnection->setUri(mFullBaseUrl);
    if (HttpRequest::OK == mSegmentConnection->get(mInitializeSegment->getDataBuffer()))
    {
        OMAF_LOG_D("SubSegment: Downloading init-section of on-demand stream");
        mState = DashSegmentStreamState::DOWNLOADING_INIT_SEGMENT;
    }
    else
    {
        OMAF_LOG_E("SubSegment: Could not start init section download");
        mState = DashSegmentStreamState::ERROR;
    }
}

void_t DashSegmentStreamOnDemand::processDownloadingInitSegment()
{
    const HttpRequestState& state = mSegmentConnection->getState();
    if (state.connectionState == HttpConnectionState::COMPLETED && state.httpStatus >= 200 && state.httpStatus < 300 &&
        state.bytesDownloaded != 0)
    {
        mState = DashSegmentStreamState::IDLE;

        DashSegment* indexSegment =
            OMAF_NEW_HEAP(DashSegment)(mInitializeSegment->getDataBuffer()->getSize(), 1, 0, false, this);
        
		indexSegment->getDataBuffer()->setData(mInitializeSegment->getDataBuffer()->getDataPtr(),
                                               mInitializeSegment->getDataBuffer()->getSize());
        Error::Enum result = mSidxParser->addSegmentIndex(indexSegment);
        if (result != Error::OK)
        {
            if (mInitSegmentSize < 25 * SEGMENT_INDEX_LENGTH)
            {
                OMAF_DELETE_HEAP(mInitializeSegment);
                mInitializeSegment = OMAF_NULL;
                mInitSegmentSize *= 5;
                OMAF_LOG_D("Segment index parsing failed, retry with a larger segment %u", mInitSegmentSize);
                mState = DashSegmentStreamState::UNINITIALIZED;
                processUninitialized();
                return;
            }
            OMAF_LOG_E("Segment index parsing failed for %s", mDashComponents.representation->GetId().c_str());
            mState = DashSegmentStreamState::ERROR;
            return;
        }
        OMAF_LOG_V("Segment index parsing succeeded %s", mDashComponents.representation->GetId().c_str());
        uint32_t totalDurationMs = 0;
        mSidxParser->getSubSegmentInfo(mSegmentDurationInMs, mSegmentCount, totalDurationMs);
        if (mOverridePositionUs > 0)
        {
            uint64_t positionUs = mOverridePositionUs;
            mCurrentSegmentIndex = calculateSegmentId(positionUs);
            // signal the found segment id & segment duration for possible tile sets (they don't have initialization
            // segment), and timestamp
            mObserver->onTargetSegmentLocated(mCurrentSegmentIndex, positionUs / 1000);
            OMAF_LOG_V("%s onTargetSegmentLocated %d for %d ms", mDashComponents.representation->GetId().c_str(),
                       mCurrentSegmentIndex, mOverridePositionUs / 1000);
            mOverrideSegmentId = INVALID_SEGMENT_INDEX;
            mOverridePositionUs = 0;
        }

        if (totalDurationMs != mTotalDurationMs)
        {
            mTotalDurationMs = totalDurationMs;
        }
        mAbsMaxCachedSegments = min(mAbsMaxCachedSegments, mSegmentCount);
        if (mBufferingTimeMs > 0)
        {
            mMaxCachedSegments = min(mAbsMaxCachedSegments,
                                     max(ABS_MIN_CACHE_BUFFERS,
                                         max((uint32_t) ceil(RUNTIME_CACHE_DURATION_MS / mSegmentDurationInMs),
                                             (uint32_t) ceil(mBufferingTimeMs / mSegmentDurationInMs))));
            OMAF_LOG_V("MaxCachedSegments based on MPD ping time %d -> %d", mBufferingTimeMs, mMaxCachedSegments);
        }
        else
        {
            mMaxCachedSegments =
                min(mAbsMaxCachedSegments,
                    max(ABS_MIN_CACHE_BUFFERS, (uint32_t) ceil(RUNTIME_CACHE_DURATION_MS / mSegmentDurationInMs)));
        }
        switch (mLatencyReq)
        {
        case LatencyRequirement::NOT_LATENCY_CRITICAL:
            // ~ 3 seconds
            mNrSegmentsMergedTarget = (uint32_t) ceil(3000.f / mSegmentDurationInMs);
            break;
        case LatencyRequirement::MEDIUM_LATENCY:
            // ~ 1 second
            mNrSegmentsMergedTarget = (uint32_t) ceil(1000.f / mSegmentDurationInMs);
            break;
        default:
            // LOW_LATENCY => no merging
            break;
        }

        OMAF_LOG_V("%s target segments to be merged %d", mDashComponents.representation->GetId().c_str(),
                   mNrSegmentsMergedTarget);

        uint32_t timestampBase = 0;
        if (!doLoop(timestampBase))
        {
            return;
        }
        mPreBufferCachedSegments =
            min(mAbsMaxCachedSegments,
                max(ABS_MIN_CACHE_BUFFERS, (uint32_t) ceil(PREBUFFER_CACHE_LIMIT_MS / mSegmentDurationInMs)));

        // check the first subsegment, to ensure the init segment is aligned with the media segments
        SubSegment subSegment;
        result = mSidxParser->getSubSegmentForIndex(mLoopedSegmentIndex, subSegment);
        OMAF_ASSERT(subSegment.endByte != 0, "no subsegments?");
        OMAF_ASSERT(result == Error::OK, "Can't find subsegments, is the index too large?");

        // check if we already have downloaded some subsegments as part of the initialization segment
        if (mInitializeSegment->getDataBuffer()->getSize() > subSegment.endByte)
        {
            uint64_t mdatStartByte = subSegment.startByte;
            // ensure the init segment and first segment form a continuous package, so ignore the remaining mdat
            const size_t nrBytesInBuffer = mInitializeSegment->getDataBuffer()->getSize();
            mInitializeSegment->getDataBuffer()->setSize(mdatStartByte);
            while (nrBytesInBuffer > subSegment.endByte)
            {
                DashSegment* newSegment =
                    OMAF_NEW_HEAP(DashSegment)(subSegment.startByte, (subSegment.endByte - subSegment.startByte) + 1,
                                               mCurrentSegmentIndex, 0, false, this);
                newSegment->getDataBuffer()->setData(
                    mInitializeSegment->getDataBuffer()->getDataPtr() + subSegment.startByte,
                    (subSegment.endByte - subSegment.startByte) + 1);
                newSegment->setTimestampBase(timestampBase);
                mTotalSegmentsDownloaded++;
                OMAF_LOG_D("First mdat segment was downloaded as part of the initial segment, copy and skip download");
                mState = DashSegmentStreamState::IDLE;
                if (mObserver->onMediaSegmentDownloaded(newSegment, 0.f) != Error::OK)
                {
                    // The segment could not be parsed.
                    // newSegment is deleted in observer
                    OMAF_LOG_E("[%p] Segment parsing failed", this);
                    mState = DashSegmentStreamState::ERROR;
                    return;
                }
                newSegment->setIsCached();
                mCachedSegmentCount++;
                mCurrentSegmentIndex++;
                mSidxParser->getSubSegmentForIndex(mCurrentSegmentIndex, subSegment);
            }
            return;
        }
        else if (mInitializeSegment->getDataBuffer()->getSize() >= subSegment.startByte)
        {
            // ensure the init segment and first segment form a continuous package
            // there is some mdat in init segment, but not a full media segment. Just ignore the mdat
            mInitializeSegment->getDataBuffer()->setSize(subSegment.startByte);
        }
        else
        {
            // sidx parsing passed, but we don't have any part of the mdat in the buffer? It may be just filling between
            // sidx and mdat, but is it safer to reload the init segment?

            OMAF_DELETE_HEAP(mSidxParser);
            mSidxParser = OMAF_NULL;
            mSidxParser = OMAF_NEW_HEAP(MP4VRParser)(sCreator, mDashComponents.parserCtx);

            OMAF_DELETE_HEAP(mInitializeSegment);
            mInitializeSegment = OMAF_NULL;
            mInitSegmentSize = subSegment.startByte;
            OMAF_LOG_D("Segment index parsed, but some part of moov seem to be missing, retry with a larger segment %u",
                       mInitSegmentSize);
            mState = DashSegmentStreamState::UNINITIALIZED;
            processUninitialized();
            return;
        }
    }
    else if (state.connectionState == HttpConnectionState::FAILED ||
             state.connectionState == HttpConnectionState::COMPLETED)
    {
        OMAF_LOG_E("Initialization segment download failed for %s", mDashComponents.representation->GetId().c_str());
        OMAF_DELETE_HEAP(mInitializeSegment);
        mInitializeSegment = OMAF_NULL;
        mState = DashSegmentStreamState::ERROR;
    }
}

void_t DashSegmentStreamOnDemand::processIdleOrRetry()
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
    uint32_t timestampBase = 0;
    if (!doLoop(timestampBase))
    {
        return;
    }
    mNrSegmentsMerged = mNrSegmentsMergedTarget;
    if (mLoopedSegmentIndex + mNrSegmentsMerged > mSegmentCount)
    {
        mNrSegmentsMerged = mSegmentCount - mLoopedSegmentIndex + 1;
    }
    OMAF_LOG_V("processIdleOrRetry merge: %d index %d", mNrSegmentsMerged, mCurrentSegmentIndex);

    SubSegment targetSegment;
    mSidxParser->getSubSegmentForIndex(mLoopedSegmentIndex, targetSegment);
    OMAF_ASSERT(targetSegment.endByte != 0, "no segments?");
    if (mNrSegmentsMerged > 1)
    {
        SubSegment lastSegment;
        mSidxParser->getSubSegmentForIndex(mLoopedSegmentIndex + mNrSegmentsMerged - 1, lastSegment);
        targetSegment.endByte = lastSegment.endByte;
    }
    OMAF_LOG_V("[%p] Download byte range %d -> %d", this, targetSegment.startByte, targetSegment.endByte);

    mSegmentConnection->setUri(mFullBaseUrl);
    HttpHeaderList hl;
    OMAF_ASSERT(targetSegment.startByte < (uint64_t) OMAF_UINT32_MAX, "Segment index out of range");
    mNextSegment =
        OMAF_NEW_HEAP(DashSegment)(targetSegment.startByte, (targetSegment.endByte - targetSegment.startByte) + 1,
                                   mCurrentSegmentIndex, 0, false, this);
    hl.setByteRange(targetSegment.startByte, targetSegment.endByte);
    mNextSegment->setTimestampBase(timestampBase);
    mRetryIndex = mCurrentSegmentIndex;
    mSegmentConnection->setHeaders(hl);
    mSegmentDownloadStartTime = Time::getClockTimeUs();
    HttpRequest::Enum result = mSegmentConnection->get(mNextSegment->getDataBuffer());
    if (HttpRequest::OK == result)
    {
        OMAF_LOG_D("%d started downloading repr\t%s\tsegment\t%d\tcached %d, byte range %d...%d",
                   Time::getClockTimeMs(), mDashComponents.representation->GetId().c_str(), mCurrentSegmentIndex,
                   mCachedSegmentCount, targetSegment.startByte, targetSegment.endByte);
        mState = DashSegmentStreamState::DOWNLOADING_MEDIA_SEGMENT;
    }
    else
    {
        OMAF_LOG_E("Could not start segment download for %s, result %d",
                   mDashComponents.representation->GetId().c_str(), result);
        mState = DashSegmentStreamState::ERROR;
        OMAF_DELETE_HEAP(mNextSegment);
        mNextSegment = OMAF_NULL;
    }
    mLoopedSegmentIndex += mNrSegmentsMerged;
    mCurrentSegmentIndex += mNrSegmentsMerged;
    mLastRequest = Time::getClockTimeMs();
}

void_t DashSegmentStreamOnDemand::handleDownloadedSegment(int64_t aDownloadTimeMs, size_t aBytesDownloaded)
{
    mTotalSegmentsDownloaded += mNrSegmentsMerged;

    float32_t factor = 0.f;
    if (aDownloadTimeMs > 0)
    {
        factor = (float32_t) mSegmentDurationInMs * mNrSegmentsMerged / aDownloadTimeMs;
        updateDownloadTime(aDownloadTimeMs / mNrSegmentsMerged);
    }

    for (uint32_t i = 0, j = mNrSegmentsMerged; i < mNrSegmentsMerged; i++, j--)
    {
        SubSegment subSegment;
        if (mSidxParser->getSubSegmentForIndex(mLoopedSegmentIndex - j, subSegment) != Error::OK)
        {
            break;
        }

        DashSegment* newSegment =
            OMAF_NEW_HEAP(DashSegment)(subSegment.startByte, (subSegment.endByte - subSegment.startByte) + 1,
                                       mCurrentSegmentIndex - j, 0, false, this);
        newSegment->getDataBuffer()->setData(
            mNextSegment->getDataBuffer()->getDataPtr() + subSegment.startByte - mNextSegment->rangeStartByte(),
            (subSegment.endByte - subSegment.startByte) + 1);

        newSegment->setTimestampBase(mNextSegment->getTimestampBase());
        if (aDownloadTimeMs > 0)
        {
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
        factor = 2.f;
    }

    OMAF_LOG_V("finished downloading repr %s segment %d, cached now %d",
               mDashComponents.representation->GetId().c_str(), mNextSegment->getSegmentId(), mCachedSegmentCount);
    if (mPreBuffering && mCachedSegmentCount >= mPreBufferCachedSegments)
    {
        OMAF_LOG_V("Prebuffering done, now %d max %d", mCachedSegmentCount, mMaxCachedSegments);
        mPreBuffering = false;
    }

    OMAF_DELETE_HEAP(mNextSegment);  // reusing it is a bit tricky, but should be studied further
    mNextSegment = OMAF_NULL;
}


dash::mpd::ISegment* DashSegmentStreamOnDemand::getNextSegment(uint32_t& segmentId)
{
    return OMAF_NULL;
}

void_t DashSegmentStreamOnDemand::downloadAborted()
{
    mCurrentSegmentIndex -= mNrSegmentsMerged;
}

Error::Enum DashSegmentStreamOnDemand::seekToMs(uint64_t& aSeekToTargetMs,
                                                uint64_t& aSeekToResultMs,
                                                uint32_t& aSeekSegmentIndex)
{
    mIndexForSeek = mStartIndex + (aSeekToTargetMs / mSegmentDurationInMs);
    OMAF_LOG_D("Seeking to %lld ms which means segment index: %d", aSeekToTargetMs, mIndexForSeek);
    aSeekToResultMs = aSeekToTargetMs;
    aSeekSegmentIndex = mIndexForSeek;
    return Error::OK;
}

uint32_t DashSegmentStreamOnDemand::calculateSegmentId(uint64_t& ptsUs)
{
    if (mSegmentDurationInMs == 0)
    {
        // not known yet. We need to save the target time and process the segment id later on
        mOverridePositionUs = ptsUs;
        return 1;
    }
    uint64_t targetTimeMs = ptsUs / 1000;
    uint32_t overrideSegmentId = (uint32_t)(targetTimeMs / segmentDurationMs());
    // + first segment id
    overrideSegmentId += mStartIndex;
    OMAF_LOG_D("Calculated segment id: %d, from time %lld", overrideSegmentId, ptsUs);
    ptsUs = (overrideSegmentId - mStartIndex) * segmentDurationMs() * 1000;

    return overrideSegmentId;
}

bool_t DashSegmentStreamOnDemand::hasFixedSegmentSize() const
{
    return true;
}

bool_t DashSegmentStreamOnDemand::setLatencyReq(LatencyRequirement::Enum aType)
{
    mLatencyReq = aType;
    if (mState != DashSegmentStreamState::UNINITIALIZED)
    {
        switch (mLatencyReq)
        {
        case LatencyRequirement::NOT_LATENCY_CRITICAL:
            // ~ 3 seconds
            mNrSegmentsMergedTarget = (uint32_t) ceil(3000.f / mSegmentDurationInMs);
            break;
        case LatencyRequirement::MEDIUM_LATENCY:
            // ~ 1 second
            mNrSegmentsMergedTarget = (uint32_t) ceil(1000.f / mSegmentDurationInMs);
            break;
        default:
            // LOW_LATENCY => no merging
            break;
        }
        OMAF_LOG_V("%s target segments to be merged %d", mDashComponents.representation->GetId().c_str(),
                   mNrSegmentsMergedTarget);
    }
    return true;
}

bool_t DashSegmentStreamOnDemand::isLastSegment() const
{
    if (mLoopingOn)
    {
        return false;
    }
    if (mCurrentSegmentIndex >= mSegmentCount - 1)
    {
        return true;
    }
    return false;
}
bool_t DashSegmentStreamOnDemand::needInitSegment(bool_t aIsIndependent)
{
    return true;  // need it for the sidx
}

void_t DashSegmentStreamOnDemand::setLoopingOn()
{
    mLoopingOn = true;
}

bool_t DashSegmentStreamOnDemand::doLoop(uint32_t& aTimestampBase)
{
    mLoopedSegmentIndex = mCurrentSegmentIndex;
    if (mCurrentSegmentIndex > mSegmentCount)
    {
        if (mLoopingOn)
        {
            // We can't use seeking locally here, since we need to ensure the timestamps continue to grow.
            // We can't use seeking in upper layers, since this behaviour should be only some DASH streams (background,
            // not e.g. for overlays) Seeking only some streams doesn't work, since rendering is based on common clock.
            // If we adjust the clock like in normal seeking, we need to seek all streams
            do
            {
                mLoopedSegmentIndex -= mSegmentCount;
                aTimestampBase += mTotalDurationMs;
            } while (mLoopedSegmentIndex > mSegmentCount);
            OMAF_LOG_V("Stream %s looped; timestampBase %d, next download segment %d running segment index %d",
                       mDashComponents.representation->GetId().c_str(), aTimestampBase, mLoopedSegmentIndex,
                       mCurrentSegmentIndex);
        }
        else
        {
            // end of stream
            OMAF_LOG_D("Reached EoS");
            mState = DashSegmentStreamState::END_OF_STREAM;
            return false;
        }
    }
    return true;
}


OMAF_NS_END
