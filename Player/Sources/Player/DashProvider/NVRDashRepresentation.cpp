
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
#include "DashProvider/NVRDashRepresentation.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "DashProvider/NVRDashTimelineStreamDynamic.h"
#include "DashProvider/NVRDashTimelineStreamStatic.h"
#include "DashProvider/NVRDashTemplateStreamStatic.h"
#include "DashProvider/NVRDashTemplateStreamDynamic.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "DashProvider/NVRDashLog.h"

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
        , mObserver(OMAF_NULL)
        , mDownloading(false)
        , mMetadataCreated(false)
        , mVideoStreamId(OMAF_UINT8_MAX)
        , mSegmentContent{}
        , mWidth(0)
        , mHeight(0)
        , mFps(0.f)
        , mStreamMode(VideoStreamMode::BASE)
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
        //delete all streams
        for (size_t i = 0; i < mVideoStreams.getSize(); i++)
        {
            OMAF_DELETE_HEAP(mVideoStreams[i]);
        }
        for (size_t i = 0; i < mAudioStreams.getSize(); i++)
        {
            OMAF_DELETE_HEAP(mAudioStreams[i]);
        }
        if (mParser != OMAF_NULL)
        {
            OMAF_DELETE_HEAP(mParser);
        }
        OMAF_DELETE_HEAP(mSegmentStream);
    }

    Error::Enum DashRepresentation::initialize(DashComponents dashComponents, uint32_t bitrate, MediaContent content, uint32_t aInitializationSegmentId, DashRepresentationObserver* observer)
    {
        mObserver = observer;

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
        dash::mpd::ISegmentTemplate *segmentTemplate = getSegmentTemplate(dashComponents);
        dashComponents.segmentTemplate = segmentTemplate;
        if (segmentTemplate != OMAF_NULL)
        {
            const dash::mpd::ISegmentTimeline *timeline = segmentTemplate->GetSegmentTimeline();
            if (timeline != OMAF_NULL)
            {
                if (mStreamType == DashStreamType::STATIC)
                {
                    mSegmentStream = OMAF_NEW_HEAP(DashTimelineStreamStatic)(bitrate, dashComponents, aInitializationSegmentId, this);
                }
                else
                {
                    mSegmentStream = OMAF_NEW_HEAP(DashTimelineStreamDynamic)(bitrate, dashComponents, aInitializationSegmentId, this);
                }
            }
            else if (mStreamType == DashStreamType::STATIC)
            {
                mSegmentStream = OMAF_NEW_HEAP(DashTemplateStreamStatic)(bitrate, dashComponents, aInitializationSegmentId, this);
                OMAF_LOG_D("Stream is static");
            }
            else
            {
                mSegmentStream = OMAF_NEW_HEAP(DashTemplateStreamDynamic)(bitrate, dashComponents, aInitializationSegmentId, this);
            }
        }

        if (mSegmentStream == OMAF_NULL)
        {
            // No segment stream created, initialize failed
            return Error::NOT_SUPPORTED;
        }

        mSegmentContent.type = content; // TODO: is there case we need to handle where representation content differs? mainly codecs. If so need to start from here.
        mSegmentContent.representationId = dashComponents.representation->GetId().c_str();
        mSegmentContent.initializationSegmentId = aInitializationSegmentId;

        if (content.matches(MediaContent::Type::VIDEO_ENHANCEMENT))
        {
            mStreamMode = VideoStreamMode::ENHANCEMENT_IDLE;
        }

        if (DashUtils::elementHasAttribute(dashComponents.representation, MPDAttribute::kAssociationIdKey))
        {
            const char_t* associationType = DashUtils::parseRawAttribute(dashComponents.representation, MPDAttribute::kAssociationTypeKey);
            if (StringCompare(MPDAttribute::kAssociationTypeValue, associationType) == ComparisonResult::EQUAL)
            {
                // the spec allows to have a whitespace-separated list of association ids, but we currently use it for a single association only
                mSegmentContent.associatedToRepresentationId = DashUtils::parseRawAttribute(dashComponents.representation, MPDAttribute::kAssociationIdKey);
                // mSegmentContent.associatedInitializationSegmentId is filled later when we have gone through all adaptationsets.
            }
        }

        const std::string& mimeType = DashUtils::getMimeType(dashComponents);
        if (MPDAttribute::VIDEO_MIME_TYPE.compare(mimeType.c_str()) == ComparisonResult::EQUAL)
        {
            BasicSourceInfo sourceInfo;
            if (MetadataParser::parseUri(dashComponents.segmentTemplate->Getinitialization().c_str(), sourceInfo))
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

    const SegmentContent& DashRepresentation::getSegmentContent() const
    {
        return mSegmentContent;
    }

    void DashRepresentation::setAssociatedToRepresentation(DashRepresentation* representation)
    {
        mAssociatedToRepresentation = representation;
        mSegmentContent.associatedToInitializationSegmentId = mAssociatedToRepresentation->getSegmentContent().initializationSegmentId;
    }

    MP4VRParser* DashRepresentation::getParserInstance()
    {
        if (mParser != OMAF_NULL)
        {
            return mParser;
        }
        else
        {
            if (mAssociatedToRepresentation != OMAF_NULL)
            {
                return mAssociatedToRepresentation->getParserInstance();
            }
            else
            {
                mParser = OMAF_NEW_HEAP(MP4VRParser);
                return mParser;
            }
        }
    }

    dash::mpd::ISegmentTemplate* DashRepresentation::getSegmentTemplate(DashComponents dashComponents)
    {
        // Try first from the representation
        dash::mpd::ISegmentTemplate *segmentTemplate = dashComponents.representation->GetSegmentTemplate();
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
        OMAF_LOG_D("Start downloading first-time segment for repr %s; stream id %d", mSegmentContent.representationId.getData(), mVideoStreamId);
        if (mStreamMode == VideoStreamMode::ENHANCEMENT_IDLE)
        {
            mStreamMode = VideoStreamMode::ENHANCEMENT_FAST_FORWARD;
        }
        mDownloading = true;
        mSegmentStream->startDownload(startDownloadTime, mInitializeIndependently);

        return Error::OK;
    }

    Error::Enum DashRepresentation::startDownloadABR(uint32_t overrideSegmentId)
    {
        if (mDownloading)
        {
            return Error::OK;
        }

        if (mStreamMode == VideoStreamMode::ENHANCEMENT_IDLE)
        {
            mStreamMode = VideoStreamMode::ENHANCEMENT_FAST_FORWARD;
        }

        mDownloading = true;
        // clear too old segments that were loaded previously but never read (ABR switch was done before they were read)
        uint32_t id = 0;
        if (getParserInstance()->getNewestSegmentId(mSegmentContent.initializationSegmentId, id))
        {
            getParserInstance()->releaseSegmentsUntil(overrideSegmentId, mAudioStreams, mVideoStreams);
            if (id >= overrideSegmentId)
            {
                // we have useful segments
				// TODO the log can crash sometimes, the representationId gets large (the \0 gets lost?)
                /*OMAF_LOG_D("ABR: reuse existing segments, and start downloading next segment for repr %s; stream id %d", id + 1,
                          mSegmentContent.representationId.getData(), mVideoStreamId);*/
                mSegmentStream->startDownloadABR(id + 1, mInitializeIndependently);
                return Error::OK;
            }
            // else all segments were too old and were released
        }
        // else there were no segments
        OMAF_LOG_D("ABR: start downloading first-time segment %d for repr %s; stream id %d", overrideSegmentId, mSegmentContent.representationId.getData(), mVideoStreamId);
        mSegmentStream->startDownloadABR(overrideSegmentId, mInitializeIndependently);

        return Error::OK;
    }


    Error::Enum DashRepresentation::seekStreamsTo(uint64_t seekToPts)
    {
        getParserInstance()->seekToUs(seekToPts, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS, SeekAccuracy::FRAME_ACCURATE);
        return Error::OK;
    }

    // VAS version - starts new representation and assumes you are late => try to fast forward
    Error::Enum DashRepresentation::startDownloadWithOverride(uint64_t overridePTSUs, uint32_t overrideSegmentId)
    {
        mStreamMode = VideoStreamMode::ENHANCEMENT_FAST_FORWARD;
        if (mDownloading)
        {
            return Error::OK;
        }
        // calculate based on the time and segment duration
        //TODO: in dynamic template streams where calculation does not seem to work reliably we use id from other layers (input),
        // but in static streams calculation seem to work better? And in timeline streams we have to do the calculation as segment indexing is dynamic (overrideSegmentId = 0 here). Need more testing with dynamic streams
        if ((overrideSegmentId == 0 || mSegmentStream->isSeekable()) && overridePTSUs != OMAF_UINT64_MAX)
        {
            OMAF_LOG_V("Re-calculate segment index, old %d from %lld", overrideSegmentId, overridePTSUs);
            overrideSegmentId = mSegmentStream->calculateSegmentId(overridePTSUs);
        }

        mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
        uint32_t id = 0;
        if (getParserInstance()->getNewestSegmentId(mSegmentContent.initializationSegmentId, id))
        {
            OMAF_LOG_D("re-startDownloadWithOverride for stream %d", mVideoStreams[0]->getStreamId());
            // there are buffered segments
            OMAF_LOG_D("There are cached segments (up to %d), remove older than %d for repr %s", id, overrideSegmentId, mSegmentContent.representationId.getData());
            // we already have the segment, remove the older ones => will start reading from it
            getParserInstance()->releaseSegmentsUntil(overrideSegmentId, mAudioStreams, mVideoStreams);
            if (overridePTSUs != OMAF_UINT64_MAX)
            {
                bool seekResult = getParserInstance()->seekToUs(overridePTSUs, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS, SeekAccuracy::FRAME_ACCURATE);
                if (seekResult == false)
                {
                    mSeekToUsWhenDLComplete = overridePTSUs;
                }
            }
            mVideoStreams[0]->setMode(mStreamMode);

            mDownloading = true;
            if (id >= overrideSegmentId)
            {
                OMAF_LOG_D("Start downloading next segments for repr %s", mSegmentContent.representationId.getData());
                mSegmentStream->startDownloadWithOverride(id+1, mInitializeIndependently);
            }
            else
            {
                SubSegment subSegment;
                if (mDownloadSubSegment && getParserInstance()->getSegmentIndexFor(overrideSegmentId, overridePTSUs, subSegment) == Error::OK)
                {
                    OMAF_LOG_D("Start downloading sub segment %d for repr %s", overrideSegmentId, mSegmentContent.representationId.getData());
                    OMAF_LOG_D("Sub Segment id: %d, PTS(ms): %d ", subSegment.segmentId, subSegment.earliestPresentationTimeMs);
                    OMAF_LOG_D("Sub Segment range, startByte: %d, endByte: %d ", subSegment.startByte, subSegment.endByte);
                    mDownloadSubSegment = false;
                    mSegmentStream->startDownloadWithByteRange(overrideSegmentId, subSegment.startByte, subSegment.endByte, mInitializeIndependently);
                }
                else
                {
                    OMAF_LOG_D("Start downloading segment %d for repr %s", overrideSegmentId, mSegmentContent.representationId.getData());
                    // the segments were too old; start downloading from the override id
                    mSegmentStream->startDownloadWithOverride(overrideSegmentId, mInitializeIndependently);
                }
            }
        }
        else
        {
            // We have not downloaded any segments yet for this representation, or have used all downloaded segments without triggering to download new ones?
            mRestarted = true;
            mDownloading = true;

            if (!mVideoStreams.isEmpty())
            {
                mVideoStreams[0]->setMode(mStreamMode);
            }
            SubSegment subSegment;
            if (mDownloadSubSegment && getParserInstance()->getSegmentIndexFor(overrideSegmentId, overridePTSUs, subSegment) == Error::OK)
            {
                OMAF_LOG_D("Start first time downloading sub segment %d for repr %s; stream id %d", overrideSegmentId, mSegmentContent.representationId.getData(), mVideoStreamId);
                OMAF_LOG_D("Sub Segment id: %d, PTS(ms): %d ", subSegment.segmentId, subSegment.earliestPresentationTimeMs);
                OMAF_LOG_D("Sub Segment range, startByte: %d, endByte: %d ", subSegment.startByte, subSegment.endByte);
                mDownloadSubSegment = false;
                mSegmentStream->startDownloadWithByteRange(overrideSegmentId, subSegment.startByte, subSegment.endByte, mInitializeIndependently);
            }
            else
            {
                OMAF_LOG_D("Start downloading first-time segment %d for repr %s; stream id %d", overrideSegmentId, mSegmentContent.representationId.getData(), mVideoStreamId);
                mSegmentStream->startDownloadWithOverride(overrideSegmentId, mInitializeIndependently);
            }

            if (overridePTSUs != OMAF_UINT64_MAX)
            {
                mSeekToUsWhenDLComplete = overridePTSUs;
            }
        }
        return Error::OK;
    }

    Error::Enum DashRepresentation::stopDownload()
    {
        {
            if (mStreamMode != VideoStreamMode::BASE)
            {
                mStreamMode = VideoStreamMode::ENHANCEMENT_IDLE;
            }

            if ( !mDownloading )
            {
                return Error::OK;
            }

            mDownloading = false;
        }

        //TODO: we need to properly handle concurrency issues.  (if mParserMutex is LOCKED when we call destroyDownloadThread, when it has just called onSegmentDownloaded... we deadlock, badly.)
        //This should be "semi" safe.... (ie. not safe at all)
        //(semi safe, means the destroyDownloadThread returns when the thread is actually stopped.....)
        mSegmentStream->stopDownload();

        mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
        return Error::OK;
    }

    Error::Enum DashRepresentation::stopDownloadAsync(bool_t aReset)
    {
        {
            if (mStreamMode != VideoStreamMode::BASE)
            {
                mStreamMode = VideoStreamMode::ENHANCEMENT_IDLE;
            }
            if (mVideoStreams.getSize() > 0 && aReset)   // only 1 video stream per representation supported
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

        //TODO: we need to properly handle concurrency issues.  (if mParserMutex is LOCKED when we call destroyDownloadThread, when it has just called onSegmentDownloaded... we deadlock, badly.)
        //This should be "semi" safe.... (ie. not safe at all)
        //(semi safe, means the destroyDownloadThread returns when the thread is actually stopped.....)
        mSegmentStream->stopDownloadAsync();

        return Error::OK;
    }

    bool_t DashRepresentation::supportsSubSegments() const
    {
        return mSubSegmentSupported;;
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

    Error::Enum DashRepresentation::startSubSegmentDownload(uint64_t targetPtsUs, uint32_t overrideSegmentId)
    {
        mStreamMode = VideoStreamMode::ENHANCEMENT_FAST_FORWARD;
        if (mDownloading)
        {
            return Error::OK;
        }
        if (!mSubSegmentSupported)
        {
            return Error::NOT_SUPPORTED;
        }
        if (!mVideoStreams.isEmpty())
        {
            mVideoStreams[0]->setMode(mStreamMode);
        }

        mDownloadSubSegment = true;
        return startDownloadWithOverride(targetPtsUs, overrideSegmentId);
    }

    bool_t DashRepresentation::isBuffering()
    {
        if (!mSegmentStream->isActive())
        {
            //segment stream is not downloading...
            return false;
        }
        if (!mDownloading)
        {
            return false;//not downloading. so cant be buffering..
        }
        bool_t isBuffering = false;
        if (mAutoFillCache)
        {
            isBuffering = mSegmentStream->isBuffering();
        }
        else
        {
            if (mSegmentStream->cachedSegments() <= 1 && !mVideoStreams.isEmpty() && mVideoStreams[0]->getMode() == VideoStreamMode::ENHANCEMENT_NORMAL && mVideoStreams[0]->getSamplesLeft() == 0)
            {
                OMAF_LOG_D("There is no video data left in the current segment in repr: %s stream %d", mSegmentContent.representationId.getData(), mVideoStreams[0]->getStreamId());
                isBuffering = true;
            }
        }

        return isBuffering;
    }

    bool_t DashRepresentation::isError()
    {
        return mSegmentStream->isError();
    }

    void_t DashRepresentation::clearDownloadedContent()
    {
        if (mSegmentStream != OMAF_NULL)
        {
            mSegmentStream->clearDownloadedSegments();
            if (mAssociatedToRepresentation == OMAF_NULL)
            {   // dependent representations share parser. Let owner clean up parser.
                getParserInstance()->releaseAllSegments(mAudioStreams, mVideoStreams);
            }
            mInitialized = false;
        }
    }

    Error::Enum DashRepresentation::onMediaSegmentDownloaded(DashSegment* segment)
    {
        OMAF_LOG_D("onMediaSegmentDownloaded representationId: %s, initSegmentId: %d, mediaSegmentId: %d ", mSegmentContent.representationId.getData(), segment->getInitSegmentId(), segment->getSegmentId());
        segment->setSegmentContent(mSegmentContent);
        if (mInitializeIndependently && !mInitialized)
        {
            DashSegment* initSegment = mSegmentStream->getInitSegment();
            initSegment->setSegmentContent(mSegmentContent);
            Error::Enum result = getParserInstance()->openSegmentedInput(initSegment, mVideoStreamId != OMAF_UINT8_MAX);  // we set the video stream id, if we create video sources based on MPD; in all other cases we let the parser to assign the id so that also sources get the ids right
            if (result != Error::OK)
            {
                return result;
            }
            {
                Spinlock::ScopeLock lock(mLock);
                Error::Enum result = handleInputSegment(segment);

                if (result != Error::OK && result != Error::OK_SKIPPED)
                {
                    return result;
                }
                if (!mVideoStreams.isEmpty())   // only 1 video stream per representation supported - using the 1st one
                {
                    if (mVideoStreams[0]->getStreamId() == OMAF_UINT8_MAX)
                    {
                        // overwrite
                        mVideoStreams[0]->setStreamId(mVideoStreamId);
                    }
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
                    mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources());
                    mVideoStreams[0]->setMode(mStreamMode);
                }
            }

            mObserver->onNewStreamsCreated();
            mInitialized = true;
        }
        else
        {
            if (!mInitializeIndependently)
            {
                // we've now received the first segment, assume we've been initialized by now - is there a risk if we haven't?
                mInitialized = true;
            }
            Error::Enum result = handleInputSegment(segment);

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

        mObserver->onSegmentDownloaded(this);

        if (mSeekToUsWhenDLComplete != OMAF_UINT64_MAX)
        {
            seekWhenSegmentAvailable();
        }

        return Error::OK;
    }

    void_t DashRepresentation::onSegmentIndexDownloaded(DashSegment* segment)
    {
        Error::Enum error = getParserInstance()->addSegmentIndex(segment);
        if (error == Error::NOT_SUPPORTED || error == Error::INVALID_DATA)
        {
            mSubSegmentSupported = false;
            mSegmentStream->stopSegmentIndexDownload();
        }
    }

    void_t DashRepresentation::onCacheWarning()
    {
        if (mObserver != OMAF_NULL)
        {
            mObserver->onCacheWarning();
        }
    }

    Error::Enum DashRepresentation::handleInputSegment(DashSegment* aSegment)
    {
        if (mAssociatedToRepresentation == OMAF_NULL)
        {
            return getParserInstance()->addSegment(aSegment, mAudioStreams, mVideoStreams);
        }
        else
        {
            return getParserInstance()->addSegment(aSegment, mAssociatedToRepresentation->getCurrentAudioStreams(), mAssociatedToRepresentation->getCurrentVideoStreams());
        }
    }

    void DashRepresentation::processSegmentDownload()
    {
        bool_t forcedCacheUpdate = false;
        if (!mAutoFillCache && mSegmentStream->isActive() && !mSegmentStream->isDownloading() && mSegmentStream->cachedSegments() <= 1)
        {
            if (!mVideoStreams.isEmpty())
            {
                uint64_t left = (uint64_t)(mVideoStreams[0]->getSamplesLeft() * 1000 / mFps);
                uint64_t avg = mSegmentStream->getAvgDownloadTimeMs();
                if (left < avg)
                {
                    OMAF_LOG_D("Force cache update for %s (stream %d); left %lld, avg %lld", mSegmentContent.representationId.getData(), mVideoStreams[0]->getStreamId(), left, avg);
                    forcedCacheUpdate = true;
                }
            }
        }
        //run the segment stream here.
        mSegmentStream->processSegmentDownload(forcedCacheUpdate);
    }

    void_t DashRepresentation::processSegmentIndexDownload(uint32_t segmentId)
    {
        //run the segment stream here.
        mSegmentStream->processSegmentIndexDownload(segmentId);
    }

    MP4VideoStreams& DashRepresentation::getCurrentVideoStreams()
    {
        Spinlock::ScopeLock lock(mLock);
        return mVideoStreams;
    }

    MP4AudioStreams&  DashRepresentation::getCurrentAudioStreams()
    {
        Spinlock::ScopeLock lock(mLock);
        return mAudioStreams;
    }

    Error::Enum DashRepresentation::readNextVideoFrame(bool_t& segmentChanged, int64_t currentTimeUs)
    {
        //In the current implementation, 1 representation is restricted to support only single video streams/tracks
        if (mSegmentStream->cachedSegments() == 0)
        {
            return Error::INVALID_STATE;
        }
        else if (mVideoStreams.isEmpty())
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
                        // End of file is detected in the next phase, when peeking packets from stream. In VAS case this can mean the stream is not yet available
                        getParserInstance()->releaseUsedSegments(mAudioStreams, mVideoStreams);
                        result = Error::OK;
                    }
                    else
                    {
                        OMAF_LOG_D("Stream %d failed reading video for repr %s result %d",mVideoStreams[0]->getStreamId(), mSegmentContent.representationId.getData(), result);
                    }
                }
                else
                {
                    if (segmentChanged)
                    {
                        OMAF_LOG_D("stream %d read video %llu", mVideoStreams[0]->getStreamId(), (*it)->peekNextFilledPacket()->presentationTimeUs());
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

            if (!(*it)->hasFilledPackets())
            {
                Error::Enum latestResult = getParserInstance()->readFrame(*(*it), segmentChanged, -1);

                if (latestResult != Error::OK)
                {
                    result = latestResult;
                    continue;
                }
            }
        }

        return result;
    }

    bool_t DashRepresentation::isDone()//when all packets and segments are used up.. and not downloading more..
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

        if (sourceType == SourceType::EQUIRECTANGULAR_PANORAMA)
        {
            // creating a partially dummy config can be risky if metadataparser is changed to ask for more data. The API should only ask for the data it needs
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
                metadataParser.setVideoMetadata(mBasicSourceInfo, config);
            }
        }
        else if (sourceType == SourceType::CUBEMAP)
        {
            DecoderConfig config;
            config.streamId = mVideoStreamId;
            config.height = mHeight;
            config.width = mWidth;

            metadataParser.setVideoMetadata(mBasicSourceInfo, config);
        }

        mMetadataCreated = true;
    }

    void_t DashRepresentation::createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum role, const VASTileViewport& viewport)
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
            // creating a partially dummy config can be risky if metadataparser is changed to ask for more data. The API should only ask for the data it needs
            DecoderConfig config;
            config.streamId = mVideoStreamId;
            config.height = mHeight;
            config.width = mWidth;
            metadataParser.setVideoMetadataPackageEquirectTile(sourceId, mBasicSourceInfo.sourceDirection, role, viewport, config);
        }

        mMetadataCreated = true;
    }

    void_t DashRepresentation::updateVideoSourceTo(SourcePosition::Enum aPosition)
    {
        MetadataParser& metadataParser = getParserInstance()->getMetadataParser();
        if (metadataParser.forceVideoTo(aPosition) && mVideoStreams.getSize() > 0)
        {
            mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources());
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

    Error::Enum DashRepresentation::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
    {
        Error::Enum result = mSegmentStream->seekToMs(aSeekToTargetMs, aSeekToResultMs);
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
        getParserInstance()->releaseUsedSegments(mAudioStreams, mVideoStreams);
    }

    //this returns now not the current read position, but next useful segment index. It may be a bit misleading wrt function name
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

    uint32_t DashRepresentation::getLastSegmentId()
    {
        return mLastSegmentId;
    }

    uint32_t DashRepresentation::getSegmentId()
    {
        if (mVideoStreams.isEmpty() || mVideoStreams[0]->getMode() == VideoStreamMode::ENHANCEMENT_FAST_FORWARD)
        {
            // fast forward tile is not a reliable reference; it may even cause unintentional ++
            return INVALID_SEGMENT_INDEX;
        }
        if (mSegmentStream->isDownloading())
        {
            OMAF_LOG_D("%s is downloading, use segment %d +1", mSegmentContent.representationId.getData(), mLastSegmentId);
            return mLastSegmentId + 1;
        }
        else
        {
            OMAF_LOG_D("%s is in early part, use segment %d", mSegmentContent.representationId.getData(), mLastSegmentId);
            return mLastSegmentId;
        }
    }

    bool_t DashRepresentation::isSegmentDurationFixed(uint64_t& segmentDurationMs)
    {
        if (mSegmentStream->hasFixedSegmentSize())
        {
            segmentDurationMs = mSegmentStream->segmentDurationMs();
            return true;
        }
        //getting segment duration does not in general make sense with timeline
        return false;
    }

    float32_t DashRepresentation::getDownloadSpeedFactor()
    {
        return mSegmentStream->getDownloadSpeed();
    }

    bool_t DashRepresentation::isEndOfStream()
    {
        return mSegmentStream->isEndOfStream();
    }

    const char_t* DashRepresentation::getId() const
    {
        return mSegmentContent.representationId.getData();
    }

    const char_t* DashRepresentation::isAssociatedTo() const
    {
        return mSegmentContent.associatedToRepresentationId.getData();
    }

    void_t DashRepresentation::loadVideoMetadata()
    {
        if (mVideoStreams.isEmpty() || mMetadataCreated)
        {
            // No video streams so this is audio - OR metadata comes from MPD and is returned from higher layers
            mMetadataCreated = true;
            return;
        }
        MP4VideoStream* videoStream = mVideoStreams.at(0);
        MetadataParser& metadataParser = getParserInstance()->getMetadataParser();

        mMetadataCreated = true;

        if (videoStream->getMetadataStream() == OMAF_NULL)
        {
            mMetadataCreated = metadataParser.setVideoMetadata(mBasicSourceInfo, videoStream->getDecoderConfig());
        }
    }

    void_t DashRepresentation::setCacheFillMode(bool_t autoFill)
    {
        mAutoFillCache = autoFill;
        if (!mAutoFillCache)
        {
            mStreamMode = VideoStreamMode::ENHANCEMENT_NORMAL;
        }
        mSegmentStream->setCacheFillMode(autoFill);
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
    size_t DashRepresentation::getNrSegments() const
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
    bool_t DashRepresentation::hasSegment(uint32_t aSegmentId, size_t& aSegmentSize)
    {
        OMAF_ASSERT(false, "Not available in this class");
        return false;
    }

    bool_t DashRepresentation::readyForSegment(uint32_t aId)
    {
        OMAF_ASSERT(false, "Not available in this class");
        return false;
    }
    Error::Enum DashRepresentation::parseConcatenatedMediaSegment(DashSegment *aSegment)
    {
        OMAF_ASSERT(false, "Not available in this class");
        return Error::INVALID_STATE;
    }

    void_t DashRepresentation::seekWhenSegmentAvailable()
    {
        OMAF_LOG_D("Stream %d, seek in the downloaded segment to %lld", mVideoStreamId, mSeekToUsWhenDLComplete);
        getParserInstance()->seekToUs(mSeekToUsWhenDLComplete, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS, SeekAccuracy::FRAME_ACCURATE);
        mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
    }

OMAF_NS_END
