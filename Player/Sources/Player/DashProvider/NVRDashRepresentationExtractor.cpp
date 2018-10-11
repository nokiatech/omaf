
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
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashRepresentationExtractor)

    DashRepresentationExtractor::DashRepresentationExtractor()
        : DashRepresentationTile()
        , mCoveredViewport(OMAF_NULL)
    {
        mInitializeIndependently = true;
    }

    DashRepresentationExtractor::~DashRepresentationExtractor()
    {
        OMAF_DELETE_HEAP(mCoveredViewport);
    }

    void_t DashRepresentationExtractor::createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel)
    {
        if (mVideoStreamId == OMAF_UINT8_MAX)
        {
            // when creating sources from MPD data, we need the stream id before streams have been generated
            mVideoStreamId = VideoDecoderManager::getInstance()->getSharedStreamID();
        }

        // skip it for now, read it from first segment instead
        mSourceType = sourceType;
        mSourceId = sourceId;
        mRole = channel;
        return;
    }

    bool_t DashRepresentationExtractor::readyForSegment(uint32_t aId)
    {
        return getParserInstance()->readyForSegment(mVideoStreams, aId);
    }

    bool_t DashRepresentationExtractor::isDone()//when all packets and segments are used up.. and not downloading more..
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
            if (mVideoStreams[i]->getSamplesLeft() <= 1)    //1??
            {
                return true;
            }
        }

        return false;
    }

    Error::Enum DashRepresentationExtractor::parseConcatenatedMediaSegment(DashSegment *aSegment)
    {
        Error::Enum result = Error::OK;
        if (mVideoStreams.isEmpty())
        {
            Spinlock::ScopeLock lock(mLock);
            OMAF_LOG_V("parseConcatenatedMediaSegment %d first time", aSegment->getSegmentId());
            result = getParserInstance()->addSegment(aSegment, mAudioStreams, mVideoStreams);
            if (result != Error::OK && result != Error::OK_SKIPPED)
            {
                OMAF_LOG_W("Parser refused to accept the segment!");
                return result;
            }
            if (mVideoStreams.isEmpty())
            {
                return result;
            }
            if (mVideoStreams[0]->getStreamId() == OMAF_UINT8_MAX)
            {
                // overwrite the uninitialized id
                mVideoStreams[0]->setStreamId(mVideoStreamId);
            }

            // create video source only now
            Error::Enum propertiesOk = getParserInstance()->parseVideoSources(*mVideoStreams[0], mSourceType, mBasicSourceInfo);
            if (propertiesOk != Error::OK)
            {
                return propertiesOk;
            }
            // even if there is no source info in mp4 (e.g RWPK), we can use the previously stored MPD parameters/default values for the source
            DashRepresentation::createVideoSource(mSourceId, mSourceType, mRole);
            OMAF_LOG_V("parseConcatenatedMediaSegment source created");
            uint32_t segmentIndex = 0;
            mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources(), getParserInstance()->getReadPositionUs(mVideoStreams, segmentIndex));
            mVideoStreams[0]->setMode(mStreamMode);
            mObserver->onNewStreamsCreated();

        }
        else
        {
            OMAF_LOG_V("%lld parseConcatenatedMediaSegment %d", Time::getClockTimeMs(), aSegment->getSegmentId());
            result = getParserInstance()->addSegment(aSegment, mAudioStreams, mVideoStreams);

            if (!mVideoStreams.front()->hasVideoSources())
            {
                uint32_t segmentIndex = 0;
                mVideoStreams[0]->setVideoSources(getParserInstance()->getVideoSources(), getParserInstance()->getReadPositionUs(mVideoStreams, segmentIndex));
            }
        }
        if (mSeekToUsWhenDLComplete != OMAF_UINT64_MAX)
        {
            OMAF_LOG_D("Stream %d, seek in the downloaded segment to %lld", mVideoStreamId, mSeekToUsWhenDLComplete);
            getParserInstance()->seekToUs(mSeekToUsWhenDLComplete, mAudioStreams, mVideoStreams, SeekDirection::PREVIOUS, SeekAccuracy::FRAME_ACCURATE);
            mSeekToUsWhenDLComplete = OMAF_UINT64_MAX;
        }
        return result;
    }

    void_t DashRepresentationExtractor::setCoveredViewport(VASTileViewport* aCoveredViewport)
    {
        mCoveredViewport = aCoveredViewport;
    }

    VASTileViewport* DashRepresentationExtractor::getCoveredViewport()
    {
        return mCoveredViewport;
    }

OMAF_NS_END
