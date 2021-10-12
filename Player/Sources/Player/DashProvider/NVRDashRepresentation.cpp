
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
#include "DashProvider/NVRDashRepresentation.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashMediaStream.h"
#include "DashProvider/NVRDashSegmentStreamOnDemand.h"
#include "DashProvider/NVRDashTemplateStreamDynamic.h"
#include "DashProvider/NVRDashTemplateStreamStatic.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashRepresentation)

DashRepresentation::DashRepresentation()
    : mMemoryAllocator(*MemorySystem::DefaultHeapAllocator())
    , mParser(OMAF_NULL)
    , mAssociatedToRepresentation(OMAF_NULL)
    , mInitialized(0)
    , mSegmentStream(OMAF_NULL)
    , mStreamType(DashStreamType::INVALID)
    , mAudioStreams()
    , mVideoStreams()
    , mLastSegmentId(0)
    , mFirstSegmentId(1)
    , mObserver(OMAF_NULL)
    , mDownloading(false)
    , mMetadataCreated(false)
    , mVideoStreamId(OMAF_UINT8_MAX)
    , mSegmentContent{}
    , mWidth(0)
    , mHeight(0)
    , mFps(0.f)
    , mStreamMode(VideoStreamMode::BACKGROUND)
    , mRestarted(false)
    , mAutoFillCache(true)
    , mSubSegmentSupported(true)
    , mDownloadSubSegment(false)
    , mIsSupported(false)
    , mBitrate(0)
    , mSeekToUsWhenDLComplete(OMAF_UINT64_MAX)
    , mInitializeIndependently(true)
    , mQualityLevel(0)
{
}

DashRepresentation::~DashRepresentation()
{
    clearDownloadedContent();
    // delete all streams
    for (size_t i = 0; i < mVideoStreams.getSize(); i++)
    {
        OMAF_DELETE_HEAP(mVideoStreams[i]);
    }
    for (size_t i = 0; i < mAudioStreams.getSize(); i++)
    {
        OMAF_DELETE_HEAP(mAudioStreams[i]);
    }
    for (size_t i = 0; i < mMetadataStreams.getSize(); i++)
    {
        OMAF_DELETE_HEAP(mMetadataStreams[i]);
    }
    if (mParser != OMAF_NULL)
    {
        OMAF_DELETE_HEAP(mParser);
    }
    OMAF_DELETE_HEAP(mSegmentStream);
}

Error::Enum DashRepresentation::initialize(DashComponents dashComponents,
                                           uint32_t bitrate,
                                           MediaContent content,
                                           uint32_t aInitializationSegmentId,
                                           DashRepresentationObserver* observer,
                                           DashAdaptationSet* adaptationSet)
{
    mAdaptationSet = adaptationSet;
    mObserver = observer;
    mDashComponents = dashComponents;
    mBasicSourceInfo = dashComponents.basicSourceInfo;
    mStreamType = DashUtils::getDashStreamType(dashComponents.mpd);
    mWidth = dashComponents.representation->GetWidth();
    mHeight = dashComponents.representation->GetHeight();

    const char_t* fps = dashComponents.representation->GetFrameRate().c_str();
    if (StringFindFirst(fps, "/", 0) != Npos)
    {
        size_t dividerPlace = StringFindFirst(fps, "/", 0);
        mFps = float64_t(atoi(FixedString8(fps, dividerPlace))) / atoi(FixedString8(fps + dividerPlace + 1));
    }
    else
    {
        mFps = atof(fps);
    }

    mBitrate = bitrate;
    dash::mpd::ISegmentTemplate* segmentTemplate = getSegmentTemplate(dashComponents);
    dashComponents.segmentTemplate = segmentTemplate;
    if (segmentTemplate != OMAF_NULL)
    {
        const dash::mpd::ISegmentTimeline* timeline = segmentTemplate->GetSegmentTimeline();

        if (mStreamType == DashStreamType::STATIC)
        {
            mSegmentStream =
                OMAF_NEW_HEAP(DashTemplateStreamStatic)(bitrate, dashComponents, aInitializationSegmentId, this);
            OMAF_LOG_D("Stream is static");
        }
        else
        {
            mSegmentStream =
                OMAF_NEW_HEAP(DashTemplateStreamDynamic)(bitrate, dashComponents, aInitializationSegmentId, this);
        }
    }
    else if (dashComponents.adaptationSet->GetSegmentList() != OMAF_NULL ||
             dashComponents.representation->GetSegmentList() != OMAF_NULL)
    {
        OMAF_LOG_W("The MPD indicates segment list - not supported");
        return Error::NOT_SUPPORTED;
    }
    else
    {
        DashProfile::Enum profile = DashUtils::getDashProfile(dashComponents.mpd);
        if (profile == DashProfile::INDEXED_TILING || profile == DashProfile::ONDEMAND || profile == DashProfile::MAIN_FULL)
        {
            // ondemand stream - or main/full profile with ondemand characteristics (as template & segmentList were
            // checked above)
            mSegmentStream =
                OMAF_NEW_HEAP(DashSegmentStreamOnDemand)(bitrate, dashComponents, aInitializationSegmentId, this);
        }
    }

    if (mSegmentStream == OMAF_NULL)
    {
        // No segment stream created, initialize failed
        return Error::NOT_SUPPORTED;
    }

    mSegmentContent.type = content;
                                     // mainly codecs. If so need to start from here.
    mSegmentContent.representationId = dashComponents.representation->GetId().c_str();
    mSegmentContent.initializationSegmentId = aInitializationSegmentId;

    if (content.matches(MediaContent::Type::VIDEO_OVERLAY))
    {
        mStreamMode = VideoStreamMode::OVERLAY;
    }

	
    if (DashUtils::elementHasAttribute(dashComponents.representation, MPDAttribute::kAssociationIdKey))
    {
        const char_t* associationType =
            DashUtils::parseRawAttribute(dashComponents.representation, MPDAttribute::kAssociationTypeKey);
        if (StringCompare(MPDAttribute::kAssociationTypeValueCdsc, associationType) == ComparisonResult::EQUAL)
        {
            // the spec allows to have a whitespace-separated list of association ids, but we currently use it for a
            // single association only
            mSegmentContent.associatedToRepresentationId =
                DashUtils::parseRawAttribute(dashComponents.representation, MPDAttribute::kAssociationIdKey);
            mSegmentContent.associationType = associationType;
            // mSegmentContent.associatedInitializationSegmentId is filled later when we have gone through all
            // adaptationsets.
        }
        else if (StringCompare(MPDAttribute::kAssociationTypeValueAudio, associationType) == ComparisonResult::EQUAL)
        {
            // the spec allows to have a whitespace-separated list of association ids, but we currently use it for a
            // single association only
            mSegmentContent.associatedToRepresentationId =
                DashUtils::parseRawAttribute(dashComponents.representation, MPDAttribute::kAssociationIdKey);
            mSegmentContent.associationType = associationType;
        }
    }

    const std::string& mimeType = DashUtils::getMimeType(dashComponents);
    if (MPDAttribute::VIDEO_MIME_TYPE.compare(mimeType.c_str()) == ComparisonResult::EQUAL)
    {
        // generic (non-OMAF video), try parsing URI in case it gives some source parameters; currently support for live
        // profile only
        BasicSourceInfo sourceInfo;
        if (dashComponents.segmentTemplate != nullptr &&
            MetadataParser::parseUri(dashComponents.segmentTemplate->Getinitialization().c_str(), sourceInfo))
        {
            mBasicSourceInfo = sourceInfo;
        }
    }
    return Error::OK;
}

uint32_t DashRepresentation::getWidth() const
{
    return mWidth;
}

uint32_t DashRepresentation::getHeight() const
{
    return mHeight;
}

float64_t DashRepresentation::getFramerate() const
{
    return mFps;
}

uint32_t DashRepresentation::getBitrate() const
{
    return mBitrate;
}

void_t DashRepresentation::assignQualityLevel(uint8_t aQualityIndex)
{
    mQualityLevel = aQualityIndex;
}
uint8_t DashRepresentation::getQualityLevel() const
{
    return mQualityLevel;
}

bool_t DashRepresentation::setLatencyReq(LatencyRequirement::Enum aType)
{
    return mSegmentStream->setLatencyReq(aType);
}

const SegmentContent& DashRepresentation::getSegmentContent() const
{
    return mSegmentContent;
}

void DashRepresentation::setAssociatedToRepresentation(DashRepresentation* representation)
{
    mAssociatedToRepresentation = representation;
    mSegmentContent.associatedToInitializationSegmentId =
        mAssociatedToRepresentation->getSegmentContent().initializationSegmentId;
}

MP4VRParser* DashRepresentation::getParserInstance()
{
    if (mParser == OMAF_NULL)
    {
        if (mAssociatedToRepresentation != OMAF_NULL)
        {
            return mAssociatedToRepresentation->getParserInstance();
        }
        else
        {
            mParser = OMAF_NEW_HEAP(MP4VRParser)(*this, mDashComponents.parserCtx);
        }
    }

    return mParser;
}

dash::mpd::ISegmentTemplate* DashRepresentation::getSegmentTemplate(DashComponents dashComponents)
{
    // Try first from the representation
    dash::mpd::ISegmentTemplate* segmentTemplate = dashComponents.representation->GetSegmentTemplate();
    if (segmentTemplate == NULL)
    {
        // Try from the adaptationSet
        segmentTemplate = dashComponents.adaptationSet->GetSegmentTemplate();
    }
    return segmentTemplate;
}

bool_t DashRepresentation::mpdUpdateRequired()
{
    return mSegmentStream->mpdUpdateRequired();
}

Error::Enum DashRepresentation::updateMPD(DashComponents components)
{
	
    components.segmentTemplate = getSegmentTemplate(components);
    return mSegmentStream->updateMPD(components);
}


uint32_t DashRepresentation::cachedSegments() const
{
    return mSegmentStream->cachedSegments();
}

Error::Enum DashRepresentation::startDownload(time_t startDownloadTime)
{
    if (mDownloading)
    {
        return Error::OK;
    }
    OMAF_LOG_D("Start downloading first-time segment for repr %s; stream id %d",
               mSegmentContent.representationId.getData(), mVideoStreamId);
    mDownloading = true;
    mSegmentStream->startDownload(startDownloadTime, mInitializeIndependently);

    return Error::OK;
}

// startDownloadFromSegment is for cases where we switch between representations of the same adaptation set, or are
// otherwise synced in segment boundaries. No seeking.
Error::Enum DashRepresentation::startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                         uint32_t aNextToBeProcessedSegmentId)
{
    if (mDownloading)
    {
        return Error::OK;
    }

    // first flush possible pending previous downloads
    mSegmentStream->completePendingDownload();

    // check once again in case the flush revealed a new segment
    if (mLastSegmentId >= aTargetDownloadSegmentId)
    {
        OMAF_LOG_V("startDownloadFromSegment one more check after pending download, use %d +1", mLastSegmentId);
        aTargetDownloadSegmentId = mLastSegmentId + 1;
    }

    mDownloading = true;
    // clear too old segments that were loaded previously but never read (ABR switch was done before they were read)
    uint32_t id = 0;
    if (getParserInstance()->getNewestSegmentId(mSegmentContent.initializationSegmentId, id))
    {
        if (aNextToBeProcessedSegmentId == INVALID_SEGMENT_INDEX)
        {
            OMAF_LOG_V("releaseSegmentsUntil %d", aTargetDownloadSegmentId);
            getParserInstance()->releaseSegmentsUntil(aTargetDownloadSegmentId, mAudioStreams, mVideoStreams,
                                                      mMetadataStreams);
        }
        if (id >= aTargetDownloadSegmentId)
        {
            // we have useful segments
            /*OMAF_LOG_D("ABR: reuse existing segments, and start downloading next segment for repr %s; stream id %d",
               id + 1, mSegmentContent.representationId.getData(), mVideoStreamId);*/
            mSegmentStream->startDownloadFromSegment(id + 1, mInitializeIndependently);
            mFirstSegmentId = id + 1;
            return Error::OK;
        }
        // else all segments were too old and were released
    }
    // else there were no segments
    OMAF_LOG_D("startDownloadFromSegment: segment %d for repr %s; stream id %d", aTargetDownloadSegmentId,
               mSegmentContent.representationId.getData(), mVideoStreamId);
    mSegmentStream->startDownloadFromSegment(aTargetDownloadSegmentId, mInitializeIndependently);

    mFirstSegmentId = aTargetDownloadSegmentId;

    return Error::OK;
}


Error::Enum DashRepresentation::seekStreamsTo(uint64_t seekToPts)
{
    getParserInstance()->seekToUs(seekToPts, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS,
                                  SeekAccuracy::FRAME_ACCURATE);
    return Error::OK;
}

Error::Enum DashRepresentation::startDownloadFromTimestamp(uint32_t& aTargetTimeMs)
{
    OMAF_ASSERT(aTargetTimeMs != OMAF_UINT32_MAX, "startDownloadFromTimestamp invalid targetTime");

    if (mDownloading)
    {
        return Error::OK;
    }
    // calculate based on the time and segment duration
    OMAF_LOG_V("Calculate segment index from target time %d", aTargetTimeMs);
    uint64_t targetTimeUs = aTargetTimeMs * 1000;
    uint32_t overrideSegmentId = mSegmentStream->calculateSegmentId(targetTimeUs);
    // at this point when no data is downloaded
    aTargetTimeMs = targetTimeUs / 1000;

    mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
    mDownloading = true;

    if (!mVideoStreams.isEmpty())
    {
        mVideoStreams[0]->setMode(mStreamMode);
    }
    SubSegment subSegment;
    if (mDownloadSubSegment &&
        getParserInstance()->getSegmentIndexFor(overrideSegmentId, aTargetTimeMs, subSegment) == Error::OK)
    {
        OMAF_LOG_D("Start first time downloading sub segment %d for repr %s; stream id %d", overrideSegmentId,
                   mSegmentContent.representationId.getData(), mVideoStreamId);
        OMAF_LOG_D("Sub Segment id: %d, PTS(ms): %d ", subSegment.segmentId, subSegment.earliestPresentationTimeMs);
        OMAF_LOG_D("Sub Segment range, startByte: %d, endByte: %d ", subSegment.startByte, subSegment.endByte);
        mDownloadSubSegment = false;
        mSegmentStream->startDownloadWithByteRange(overrideSegmentId, subSegment.startByte, subSegment.endByte,
                                                   mInitializeIndependently);
    }
    else
    {
        OMAF_LOG_D("Start downloading first-time segment %d for repr %s; stream id %d", overrideSegmentId,
                   mSegmentContent.representationId.getData(), mVideoStreamId);
        mSegmentStream->startDownloadFrom(overrideSegmentId, mInitializeIndependently);
    }
    mFirstSegmentId = overrideSegmentId;
    return Error::OK;
}

Error::Enum DashRepresentation::stopDownload()
{
    {
        if (!mDownloading && !mSegmentStream->isCompletingDownload())
        {
            return Error::OK;
        }

        mDownloading = false;
    }

    mSegmentStream->stopDownload();

    mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
    return Error::OK;
}

Error::Enum DashRepresentation::stopDownloadAsync(bool_t aAbort, bool_t aReset)
{
    {
        if (mVideoStreams.getSize() > 0 && aReset)  // only 1 video stream per representation supported
        {
            OMAF_LOG_D("stopDownload %s stream %d", getId(), mVideoStreams[0]->getStreamId());
            mVideoStreams[0]->setMode(mStreamMode);
            mVideoStreams[0]->resetPackets();
        }

        if (!mDownloading)
        {
            return Error::OK;
        }

        mDownloading = false;
    }

    OMAF_LOG_V("stopDownloadAsync of %s", getId());
    mSegmentStream->stopDownloadAsync(aAbort);
    mSegmentStream->processSegmentDownload(true);

    return Error::OK;
}

void_t DashRepresentation::switchedToAnother()
{
    if (mSegmentStream->isCompletingDownload())
    {
        mSegmentStream->stopDownloadAsync(true);
    }
}

bool_t DashRepresentation::supportsSubSegments() const
{
    return mSubSegmentSupported;
}


Error::Enum DashRepresentation::hasSubSegmentsFor(uint64_t targetPtsUs, uint32_t overrideSegmentId)
{
    if (overrideSegmentId == 0)
    {
        overrideSegmentId = mSegmentStream->calculateSegmentId(targetPtsUs);
    }
    Error::Enum result = getParserInstance()->hasSegmentIndexFor(overrideSegmentId, targetPtsUs);
    if (result == Error::NOT_SUPPORTED)
    {
        mSubSegmentSupported = false;
        mSegmentStream->stopSegmentIndexDownload();
    }
    return result;
}

bool_t DashRepresentation::isBuffering()
{
    if (!mSegmentStream->isActive())
    {
        // segment stream is not downloading...
        return false;
    }
    if (!mDownloading)
    {
        return false;  // not downloading. so cant be buffering..
    }
    bool_t isBuffering = false;
    if (mAutoFillCache)
    {
        isBuffering = mSegmentStream->isBuffering();
    }
    else
    {
        if (mSegmentStream->cachedSegments() <= 1 && !mVideoStreams.isEmpty() &&
            mVideoStreams[0]->getSamplesLeft() == 0)
        {
            OMAF_LOG_D("There is no video data left in the current segment in repr: %s stream %d",
                       mSegmentContent.representationId.getData(), mVideoStreams[0]->getStreamId());
            isBuffering = true;
        }
    }

    return isBuffering;
}

bool_t DashRepresentation::isError()
{
    return mSegmentStream->isError();
}

void_t DashRepresentation::clearDownloadedContent(bool_t aResetInitialization)
{
    if (mSegmentStream != OMAF_NULL)
    {
        mSegmentStream->clearDownloadedSegments();
        if (mAssociatedToRepresentation == OMAF_NULL)
        {  // dependent representations share parser. Let owner clean up parser.
            getParserInstance()->releaseAllSegments(mAudioStreams, mVideoStreams, mMetadataStreams,
                                                    aResetInitialization);
        }
        if (aResetInitialization)
        {
            mInitialized = false;
        }
    }
}

Error::Enum DashRepresentation::onMediaSegmentDownloaded(DashSegment* segment, float32_t aSpeedFactor)
{
    OMAF_LOG_D("onMediaSegmentDownloaded representationId: %s, initSegmentId: %d, mediaSegmentId: %d ",
               mSegmentContent.representationId.getData(), segment->getInitSegmentId(), segment->getSegmentId());
    segment->setSegmentContent(mSegmentContent);
    if (mInitializeIndependently && !mInitialized)
    {
        DashSegment* initSegment = mSegmentStream->getInitSegment();
        initSegment->setSegmentContent(mSegmentContent);
        Error::Enum result = getParserInstance()->openSegmentedInput(
            initSegment,
            mVideoStreamId !=
                OMAF_UINT8_MAX);  // we set the video stream id, if we create video sources based on MPD; in all other
                                  // cases we let the parser to assign the id so that also sources get the ids right
        if (result != Error::OK)
        {
            return result;
        }
        {
            Spinlock::ScopeLock lock(mLock);
            bool_t readyForReading = false;
            Error::Enum result = handleInputSegment(segment, readyForReading);

            if (result != Error::OK && result != Error::OK_SKIPPED)
            {
                return result;
            }
            if (!mVideoStreams.isEmpty())
            {
                // only 1 video stream per representation supported - using the 1st one
                // Note! for extractors the video streams get created later, so the following is for non-extractor cases
                if (mVideoStreams[0]->getStreamId() == OMAF_UINT8_MAX)
                {
                    // overwrite
                    mVideoStreams[0]->setStreamId(mVideoStreamId);
                }

				OMAF_LOG_D("Got VideoStream with streamId: %d for adaptation set id: %d", mVideoStreams[0]->getStreamId(), mDashComponents.adaptationSet->GetId());

                initializeVideoSource();

                // only 1 video stream per representation supported, remove the other ones
                if (mVideoStreams.getSize() > 1)
                {
                    for (size_t i = 1; i < mVideoStreams.getSize(); i++)
                    {
                        OMAF_DELETE_HEAP(mVideoStreams[1]);
                        mVideoStreams.removeAt(1);
                    }
                }
                if (!mMetadataCreated)
                {
                    loadVideoMetadata();
                }
                if (readyForReading)
                {
                    mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources());
                }
                else
                {
                    mVideoStreams[0]->clearVideoSources();
                }
                mVideoStreams[0]->setMode(mStreamMode);
            }
        }

        mObserver->onNewStreamsCreated(mSegmentContent.type);
        mInitialized = true;
    }
    else
    {

        if (!mInitializeIndependently)
        {
            // we've now received the first segment, assume we've been initialized by now - is there a risk if we
            // haven't?
            mInitialized = true;
        }
        bool_t readyForReading = false;
        Error::Enum result = handleInputSegment(segment, readyForReading);

        if (result != Error::OK)
        {
            return result;
        }
        if (!mVideoStreams.isEmpty())
        {
            mVideoStreams[0]->dataAvailable();
            if (mRestarted)
            {
                OMAF_LOG_D("onSegmentDownloaded & restarted");
                mVideoStreams[0]->setMode(mStreamMode);
                mRestarted = false;
            }
        }
    }
    mLastSegmentId = segment->getSegmentId();

    OMAF_LOG_D("onSegmentDownloaded for stream %d", mVideoStreamId);

    mObserver->onSegmentDownloaded(this, aSpeedFactor);

    if (mSeekToUsWhenDLComplete != OMAF_UINT64_MAX)
    {
        seekWhenSegmentAvailable();
    }

    return Error::OK;
}

void_t DashRepresentation::onSegmentIndexDownloaded(DashSegment* segment)
{
    Error::Enum error =
        getParserInstance()->addSegmentIndex(segment, 2);  // at least 2 subsegments means there are really subsegments
    if (error == Error::NOT_SUPPORTED || error == Error::INVALID_DATA)
    {
        mSubSegmentSupported = false;
        mSegmentStream->stopSegmentIndexDownload();
    }
}

void_t DashRepresentation::onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs)
{
    if (mObserver != OMAF_NULL)
    {
        mObserver->onTargetSegmentLocated(aSegmentId, aSegmentTimeMs);
    }
}

void_t DashRepresentation::onCacheWarning()
{
    if (mObserver != OMAF_NULL)
    {
        mObserver->onCacheWarning();
    }
}

Error::Enum DashRepresentation::handleInputSegment(DashSegment* aSegment, bool_t& aReadyForReading)
{
    aReadyForReading = true;
    auto result = Error::OK;

    if (mAssociatedToRepresentation == OMAF_NULL)
    {       
        result = getParserInstance()->addSegment(aSegment, mAudioStreams, mVideoStreams, mMetadataStreams);
    }
    else
    {
        result = getParserInstance()->addSegment(aSegment, mAssociatedToRepresentation->getCurrentAudioStreams(),
                                               mAssociatedToRepresentation->getCurrentVideoStreams(), mMetadataStreams);
	}

	// add adaptation set references, which are needed and not set by MP4Parser
    for (auto video : mVideoStreams)
    {
        dynamic_cast<DashVideoStream*>(video)->setAdaptationSetReference(mAdaptationSet);
    }

	return result;
}

void_t DashRepresentation::initializeVideoSource()
{
}

void DashRepresentation::processSegmentDownload()
{
    bool_t forcedCacheUpdate = false;
    if (!mAutoFillCache && mSegmentStream->isActive() && !mSegmentStream->isDownloading() &&
        mSegmentStream->cachedSegments() <= 1)
    {
        if (!mVideoStreams.isEmpty())
        {
            uint64_t left = (uint64_t)(mVideoStreams[0]->getSamplesLeft() * 1000 / mFps);
            uint64_t avg = mSegmentStream->getAvgDownloadTimeMs();
            if (left < avg)
            {
                OMAF_LOG_D("Force cache update for %s (stream %d); left %lld, avg %lld",
                           mSegmentContent.representationId.getData(), mVideoStreams[0]->getStreamId(), left, avg);
                forcedCacheUpdate = true;
            }
        }
    }
    // run the segment stream here.
    mSegmentStream->processSegmentDownload(forcedCacheUpdate);
}

void_t DashRepresentation::processSegmentIndexDownload(uint32_t segmentId)
{
    // run the segment stream here.
    mSegmentStream->processSegmentIndexDownload(segmentId);
}

MP4VideoStreams& DashRepresentation::getCurrentVideoStreams()
{
    Spinlock::ScopeLock lock(mLock);
    return mVideoStreams;
}

MP4AudioStreams& DashRepresentation::getCurrentAudioStreams()
{
    Spinlock::ScopeLock lock(mLock);
    return mAudioStreams;
}

MP4MetadataStreams& DashRepresentation::getCurrentMetadataStreams()
{
    Spinlock::ScopeLock lock(mLock);
    return mMetadataStreams;
}
Error::Enum DashRepresentation::readNextVideoFrame(bool_t& segmentChanged, int64_t currentTimeUs)
{
	
	if (mVideoStreams.isEmpty())
    {
        return Error::ITEM_NOT_FOUND;
    }
    
	Error::Enum result = Error::OK;
    for (MP4VideoStreams::Iterator it = mVideoStreams.begin(); it != mVideoStreams.end(); ++it)
    {
        if (!(*it)->hasFilledPackets())
        {
            result = getParserInstance()->readFrame(*(*it), segmentChanged, currentTimeUs);
            if (result != Error::OK)
            {
                if (result == Error::END_OF_FILE)
                {
                    // End of file is detected in the next phase, when peeking packets from stream. In VAS case this can
                    // mean the stream is not yet available
                    getParserInstance()->releaseUsedSegments(mAudioStreams, mVideoStreams, mMetadataStreams);
                    result = Error::OK;
                }
                else
                {
                    OMAF_LOG_D("Stream %d failed reading video for repr %s result %d", mVideoStreams[0]->getStreamId(),
                               mSegmentContent.representationId.getData(), result);
                }
            }
            else
            {
                if (segmentChanged)
                {
                    OMAF_LOG_D("stream %d read first video frame %llu from a new segment",
                               mVideoStreams[0]->getStreamId(), (*it)->peekNextFilledPacket()->presentationTimeUs());
                }
            }
        }
    }

    return result;
}

Error::Enum DashRepresentation::readNextAudioFrame(bool_t& segmentChanged)
{
    Error::Enum result = Error::OK;

    for (MP4AudioStreams::Iterator it = mAudioStreams.begin(); it != mAudioStreams.end(); ++it)
    {
        MP4AudioStream* stream = *it;

        if (!stream->hasFilledPackets())
        {
            Error::Enum latestResult = getParserInstance()->readFrame(*stream, segmentChanged, -1);

            if (latestResult != Error::OK)
            {
                result = latestResult;
                continue;
            }

            if (segmentChanged)
            {
                OMAF_LOG_D("stream %d read first audio frame %llu from a new segment", stream->getStreamId(),
                           stream->peekNextFilledPacket()->presentationTimeUs());
            }
        }
    }

    return result;
}

Error::Enum DashRepresentation::readMetadataFrame(bool_t& aSegmentChanged, int64_t aCurrentTimeUs)
{
    Error::Enum result = Error::OK_SKIPPED;

    for (MP4MetadataStreams::Iterator it = mMetadataStreams.begin(); it != mMetadataStreams.end(); ++it)
    {
        MP4MediaStream* stream = *it;

        if (!(*it)->hasFilledPackets())
        {
            Error::Enum latestResult =
                getParserInstance()->readTimedMetadataFrame(*(*it), aSegmentChanged, aCurrentTimeUs);

            if (latestResult != result)
            {
                result = latestResult;
                continue;
            }
        }
    }
    return result;
}

Error::Enum DashRepresentation::updateMetadata(const MP4MediaStream& metadataStream,
                                               const MP4VideoStream& refStream,
                                               const MP4VRMediaPacket& metadataFrame)
{
    Error::Enum result = Error::OK;
    // handle overlay metadata packages
    if (StringCompare(metadataStream.getFormat()->getFourCC(), "dyol") == ComparisonResult::EQUAL)
    {
        MP4VR::OverlayConfigSample dyolSample((char*) metadataFrame.buffer(), metadataFrame.dataSize());

        MP4VR::OverlayConfigProperty ovlyConfig;
        MP4VR::OverlayConfigProperty* ovlyConfigPtr = OMAF_NULL;
        // Note: the sampleId needs to be a valid sample id, that the parser currently has. For metadata we can use the
        // id from the read metadataFrame
        if (((const DashMetadataStream*) &metadataStream)
                ->getPropertyOverlayConfig(ovlyConfig, metadataFrame.sampleId()) == Error::OK)
        {
            ovlyConfigPtr = &ovlyConfig;

			MP4VR::OverlayConfigProperty refTrackConfig;
            MP4VR::OverlayConfigProperty* refTrackConfigPtr = OMAF_NULL;
            
			// this is overlay, so the refStream must be DashVideoStream
            // Note: the sampleId needs to be a valid sample id, that the parser currently has. For video we
            // can use the next available sample id
            if (((DashVideoStream&) refStream).getPropertyOverlayConfig(refTrackConfig) == Error::OK)
            {
                refTrackConfigPtr = &refTrackConfig;
            }
            else
            {
                OMAF_LOG_W("No videostream overlayproperty");
                result = Error::OK_SKIPPED;
            }

            if (!getParserInstance()->getMetadataParser().updateOverlayMetadata(refStream, refTrackConfigPtr,
                                                                                ovlyConfigPtr, dyolSample))
            {
                result = Error::OK_SKIPPED;
            }
        }
        else
        {
            OMAF_LOG_W("No metastream overlayproperty");
            result = Error::OK_SKIPPED;
        }
    }

    // handle invo
    if (StringCompare(metadataStream.getFormat()->getFourCC(), "invo") == ComparisonResult::EQUAL)
    {
        MP4VR::InitialViewingOrientationSample invo((char*) metadataFrame.buffer(), metadataFrame.dataSize());
        if (!getParserInstance()->getMetadataParser().updateInitialViewingOrientationMetadata(refStream, invo))
        {
             result = Error::OK_SKIPPED;
        }
    }

    return result;
}

bool_t DashRepresentation::isDone()  // when all packets and segments are used up.. and not downloading more..
{
    if (mDownloading && !mSegmentStream->isEndOfStream())
    {
        return false;
    }

    if (mSegmentStream->isEndOfStream() && mVideoStreams.isEmpty())
    {
        // Stream has reached end of stream with 0 video streams, this suggests an EoS situation so mark as done
        return true;
    }

    for (size_t i = 0; i < mVideoStreams.getSize(); i++)
    {
        if (mVideoStreams[i]->isEoF())
        {
            return true;
        }
    }

    return false;
}

void_t DashRepresentation::createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum role)
{
    MetadataParser& metadataParser = getParserInstance()->getMetadataParser();
    if (mVideoStreamId == OMAF_UINT8_MAX)
    {
        // when creating sources from MPD data, we need the stream id before streams have been generated
        mVideoStreamId = VideoDecoderManager::getInstance()->generateUniqueStreamID();
    }

	OMAF_LOG_D("Creating video source mVideoStreamId %d sourceId: %d, MP4Parser: %d metadata parser: %d", mVideoStreamId, sourceId, getParserInstance(), &metadataParser);

    if (sourceType == SourceType::EQUIRECTANGULAR_PANORAMA)
    {
        // creating a partially dummy config can be risky if metadataparser is changed to ask for more data. The API
        // should only ask for the data it needs
        DecoderConfig config;
        config.streamId = mVideoStreamId;
        config.height = mHeight;
        config.width = mWidth;
        if (mBasicSourceInfo.sourceDirection == SourceDirection::DUAL_TRACK)
        {
            // separate left / right
            SourcePosition::Enum position = SourcePosition::MONO;
            if (role == StereoRole::LEFT)
            {
                position = SourcePosition::LEFT;
            }
            else if (role == StereoRole::RIGHT)
            {
                position = SourcePosition::RIGHT;
            }

            metadataParser.setVideoMetadataPackage1ChStereo(sourceId, config, position, mBasicSourceInfo.sourceType);
            sourceId++;
        }
        else
        {
            metadataParser.setVideoMetadata(mBasicSourceInfo, sourceId, config);
        }
    }
    else if (sourceType == SourceType::CUBEMAP)
    {
        DecoderConfig config;
        config.streamId = mVideoStreamId;
        config.height = mHeight;
        config.width = mWidth;

        metadataParser.setVideoMetadata(mBasicSourceInfo, sourceId, config);
    }
    else if (sourceType == SourceType::IDENTITY)
    {
        // for example 2D overlay source
        DecoderConfig config;
        config.streamId = mVideoStreamId;
        config.height = mHeight;
        config.width = mWidth;

        metadataParser.setVideoMetadata(mBasicSourceInfo, sourceId, config);
    }

    mMetadataCreated = true;
}

void_t DashRepresentation::createVideoSource(sourceid_t& sourceId,
                                             SourceType::Enum sourceType,
                                             StereoRole::Enum role,
                                             const VASTileViewport& viewport)
{
    if (role == StereoRole::LEFT || role == StereoRole::RIGHT)
    {
        mBasicSourceInfo.sourceDirection = SourceDirection::DUAL_TRACK;
    }

    MetadataParser& metadataParser = getParserInstance()->getMetadataParser();
    if (mVideoStreamId == OMAF_UINT8_MAX)
    {
        // when creating sources from MPD data, we need the stream id before streams have been generated
        mVideoStreamId = VideoDecoderManager::getInstance()->generateUniqueStreamID();
    }

    if (sourceType == SourceType::EQUIRECTANGULAR_TILES)
    {
        // creating a partially dummy config can be risky if metadataparser is changed to ask for more data. The API
        // should only ask for the data it needs
        DecoderConfig config;
        config.streamId = mVideoStreamId;
        config.height = mHeight;
        config.width = mWidth;
        metadataParser.setVideoMetadataPackageEquirectTile(sourceId, mBasicSourceInfo.sourceDirection, role, viewport,
                                                           config);
    }

    mMetadataCreated = true;
}

void_t DashRepresentation::updateVideoSourceTo(SourcePosition::Enum aPosition)
{
    MetadataParser& metadataParser = getParserInstance()->getMetadataParser();
    if (metadataParser.forceVideoTo(aPosition) && mVideoStreams.getSize() > 0)
    {
        uint32_t segmentIndex = 0;
        mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources(),
                                          getParserInstance()->getReadPositionUs(mVideoStreams[0], segmentIndex));
    }
    // if no videostreams exist yet, the new source is set when creating a stream
}

const CoreProviderSources& DashRepresentation::getVideoSources()
{
    return getParserInstance()->getVideoSources();
}

const CoreProviderSourceTypes& DashRepresentation::getVideoSourceTypes()
{
    return getParserInstance()->getVideoSourceTypes();
}

void_t DashRepresentation::clearVideoSources()
{
    if (!mVideoStreams.isEmpty())
    {
        mVideoStreams[0]->clearVideoSources();
    }
}

Error::Enum DashRepresentation::seekToMs(uint64_t& aSeekToTargetMs,
                                         uint64_t& aSeekToResultMs,
                                         uint32_t& aSeekSegmentIndex)
{
    Error::Enum result = mSegmentStream->seekToMs(aSeekToTargetMs, aSeekToResultMs, aSeekSegmentIndex);
    if (!mVideoStreams.isEmpty())
    {
        OMAF_ASSERT(mVideoStreams.getSize() <= 1, "Only 1 video stream per representation supported");
        mVideoStreams.at(0)->resetPackets();
    }
    else if (!mAudioStreams.isEmpty())
    {
        OMAF_ASSERT(mAudioStreams.getSize() <= 1, "Only 1 audio stream per representation supported");
        mAudioStreams.at(0)->resetPackets();
    }

    clearDownloadedContent(false);

    mSeekToUsWhenDLComplete = aSeekToResultMs * 1000;
    return result;
}

bool_t DashRepresentation::isSeekable()
{
    return mSegmentStream->isSeekable();
}

uint64_t DashRepresentation::durationMs()
{
    return mSegmentStream->durationMs();
}
void_t DashRepresentation::releaseOldSegments()
{
    getParserInstance()->releaseUsedSegments(mAudioStreams, mVideoStreams, mMetadataStreams);
}

void_t DashRepresentation::releaseSegmentsUntil(uint32_t aSegmentId)
{
    getParserInstance()->releaseSegmentsUntil(aSegmentId, mAudioStreams, mVideoStreams, mMetadataStreams);
}

// this returns now not the current read position, but next useful segment index. It may be a bit misleading wrt
// function name
uint64_t DashRepresentation::getReadPositionUs(uint32_t& segmentIndex)
{
    if (mSeekToUsWhenDLComplete != OMAF_UINT64_MAX)
    {
        OMAF_LOG_V("getReadPositionUs - use seeking position %lld", mSeekToUsWhenDLComplete);
        segmentIndex = mSegmentStream->calculateSegmentId(mSeekToUsWhenDLComplete);
        return mSeekToUsWhenDLComplete;
    }
    else if (!mVideoStreams.isEmpty())
    {
        uint64_t pos = getParserInstance()->getReadPositionUs(mVideoStreams, segmentIndex);
        uint64_t timeLeft = mVideoStreams[0]->getTimeLeftInCurrentSegmentMs();
        if (timeLeft < mSegmentStream->getAvgDownloadTimeMs())
        {
            segmentIndex++;
        }
        return pos;
    }
    else
    {
        segmentIndex = INVALID_SEGMENT_INDEX;
        return 0;
    }
}

uint32_t DashRepresentation::getLastSegmentId(bool_t aIncludeSegmentInDownloading)
{
    if (aIncludeSegmentInDownloading && mSegmentStream->isCompletingDownload())
    {
        // OMAF_LOG_V("getLastSegmentId of %s is %d + 1", getId(), mLastSegmentId);
        return mLastSegmentId + 1;
    }
    else
    {
        if (mLastSegmentId > 0)
        {
            // OMAF_LOG_V("getLastSegmentId of %s is %d", getId(), mLastSegmentId);
        }
        return mLastSegmentId;
    }
}
uint32_t DashRepresentation::getFirstSegmentId()
{
    return mFirstSegmentId;
}

uint32_t DashRepresentation::getCurrentSegmentId()
{
    uint32_t segmentIndex = 0;
    getParserInstance()->getReadPositionUs(mVideoStreams, segmentIndex);
    return segmentIndex;
}

bool_t DashRepresentation::isSegmentDurationFixed(uint64_t& segmentDurationMs)
{
    if (mSegmentStream->hasFixedSegmentSize())
    {
        segmentDurationMs = mSegmentStream->segmentDurationMs();
        return true;
    }
    // getting segment duration does not in general make sense with timeline
    return false;
}

bool_t DashRepresentation::isEndOfStream()
{
    return mSegmentStream->isEndOfStream();
}

const char_t* DashRepresentation::getId() const
{
    return mSegmentContent.representationId.getData();
}

const char_t* DashRepresentation::isAssociatedTo(const char_t* aAssociationType) const
{
    if (StringCompare(mSegmentContent.associationType, aAssociationType) == ComparisonResult::EQUAL)
    {
        return mSegmentContent.associatedToRepresentationId.getData();
    }
    return OMAF_NULL;
}

void_t DashRepresentation::loadVideoMetadata()
{
    if (mVideoStreams.isEmpty() || mMetadataCreated)
    {
        // No video streams so this is audio - OR metadata comes from MPD and is returned from higher layers
        mMetadataCreated = true;
        return;
    }
    OMAF_ASSERT(mVideoStreams.getSize() <= 1, "Only 1 video stream per representation supported");
    MP4VideoStream* videoStream = mVideoStreams.at(0);
    MetadataParser& metadataParser = getParserInstance()->getMetadataParser();

    mMetadataCreated = true;

    if (videoStream->getMetadataStreams().isEmpty())
    {
        sourceid_t sourceId = 0;
        mMetadataCreated = metadataParser.setVideoMetadata(mBasicSourceInfo, sourceId, videoStream->getDecoderConfig());
    }
}

void_t DashRepresentation::setCacheFillMode(bool_t autoFill)
{
    mAutoFillCache = autoFill;
    if (!mAutoFillCache)
    {
        mStreamMode = VideoStreamMode::INVALID;
    }
    mSegmentStream->setCacheFillMode(autoFill);
}

void_t DashRepresentation::setBufferingTime(uint32_t aBufferingTimeMs)
{
    mSegmentStream->setBufferingTime(aBufferingTimeMs);
}

// this representation is too demanding for this device (e.g. 8k res). By default all are supported
void_t DashRepresentation::setNotSupported()
{
    mIsSupported = false;
}
// can this device use this representation?
bool_t DashRepresentation::getIsSupported()
{
    return mIsSupported;
}

bool_t DashRepresentation::getIsInitialized() const
{
    return mInitialized;
}

DashSegment* DashRepresentation::peekSegment() const
{
    OMAF_ASSERT(false, "Not available in this class");
    return OMAF_NULL;
}
size_t DashRepresentation::getNrSegments(uint32_t aNextNeededSegment) const
{
    OMAF_ASSERT(false, "Not available in this class");
    return 0;
}
DashSegment* DashRepresentation::getSegment()
{
    OMAF_ASSERT(false, "Not available in this class");
    return OMAF_NULL;
}
void_t DashRepresentation::cleanUpOldSegments(uint32_t aNextSegmentId)
{
    OMAF_ASSERT(false, "Not available in this class");
}
bool_t DashRepresentation::hasSegment(const uint32_t aSegmentId, uint32_t& aOldestSegmentId, size_t& aSegmentSize)
{
    OMAF_ASSERT(false, "Not available in this class");
    return false;
}

bool_t DashRepresentation::readyForSegment(uint32_t aId)
{
    OMAF_ASSERT(false, "Not available in this class");
    return false;
}
Error::Enum DashRepresentation::parseConcatenatedMediaSegment(DashSegment* aSegment)
{
    OMAF_ASSERT(false, "Not available in this class");
    return Error::INVALID_STATE;
}

void_t DashRepresentation::seekWhenSegmentAvailable()
{
    OMAF_LOG_D("Stream %d, seek in the downloaded segment to %lld", mVideoStreamId, mSeekToUsWhenDLComplete);
    if (getParserInstance()->seekToUs(mSeekToUsWhenDLComplete, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS,
                                      SeekAccuracy::FRAME_ACCURATE))
    {
        OMAF_LOG_D("Stream %d, seeking downloaded segment to %lld with success", mVideoStreamId,
                   mSeekToUsWhenDLComplete);
        mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
    }
}

MP4MediaStream* DashRepresentation::createStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(DashMetadataStream)(aFormat, *mParser);
}
MP4VideoStream* DashRepresentation::createVideoStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(DashVideoStream)(aFormat, *mParser);
}
MP4AudioStream* DashRepresentation::createAudioStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(MP4AudioStream)(aFormat);
}

void_t DashRepresentation::destroyStream(MP4MediaStream* aStream) const
{
    OMAF_DELETE_HEAP(aStream);
}

void_t DashRepresentation::setLoopingOn()
{
    mSegmentStream->setLoopingOn();
}

void_t DashRepresentation::setVideoStreamId(streamid_t aVideoStreamId)
{
    mVideoStreamId = aVideoStreamId;
}

streamid_t DashRepresentation::getVideoStreamId() const
{
    return mVideoStreamId;
}


OMAF_NS_END
