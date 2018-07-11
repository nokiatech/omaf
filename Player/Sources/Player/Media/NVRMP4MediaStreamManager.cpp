
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
#include "Media/NVRMP4MediaStreamManager.h"
#include "Media/NVRMediaPacket.h"
#include "Media/NVRMP4Parser.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MP4MediaStreamManager);

    MP4MediaStreamManager::MP4MediaStreamManager()
        : mAllocator(*MemorySystem::DefaultHeapAllocator())
        , mMediaFormatParser(OMAF_NULL)
        , mAudioStreams()
        , mVideoStreams()

    {

    }
    MP4MediaStreamManager::~MP4MediaStreamManager()
    {
        OMAF_DELETE(mAllocator, mMediaFormatParser);
        resetStreams();
    }

    Error::Enum MP4MediaStreamManager::openInput(const PathName& mediaUri)
    {
        // free old mp4reader
        if (mMediaFormatParser != OMAF_NULL)
        {
            OMAF_DELETE(mAllocator, mMediaFormatParser);
            mMediaFormatParser = OMAF_NULL;
        }

        // create new mp4reader
        mMediaFormatParser = OMAF_NEW(mAllocator, MP4VRParser)();

        Error::Enum result = mMediaFormatParser->openInput(mediaUri, mAudioStreams, mVideoStreams);
        for (MP4VideoStreams::ConstIterator it = mVideoStreams.begin(); it != mVideoStreams.end(); ++it)
        {
            (*it)->setMode(VideoStreamMode::BASE);
        }

        return result;
    }

    void_t MP4MediaStreamManager::resetStreams()
    {
        while (!mVideoStreams.isEmpty())
        {
            MP4VideoStream* videoStream = mVideoStreams[0];
            videoStream->resetPackets();
            mVideoStreams.removeAt(0);
            OMAF_DELETE(mAllocator, videoStream);
        }
        mVideoStreams.clear();

        while (!mAudioStreams.isEmpty())
        {
            MP4AudioStream* audioStream = mAudioStreams[0];
            audioStream->resetPackets();
            mAudioStreams.removeAt(0);
            OMAF_DELETE(mAllocator, audioStream);
        }
        mAudioStreams.clear();
    }

    const MP4AudioStreams& MP4MediaStreamManager::getAudioStreams()
    {
        return mAudioStreams;
    }

    const MP4VideoStreams& MP4MediaStreamManager::getVideoStreams()
    {
        return mVideoStreams;
    }
    int32_t MP4MediaStreamManager::getNrStreams()
    {
        return static_cast<int32_t>(mAudioStreams.getSize() + mVideoStreams.getSize());
    }

    MP4VRMediaPacket* MP4MediaStreamManager::getNextAudioFrame(MP4MediaStream& stream)
    {
        if (!stream.hasFilledPackets())
        {
            bool_t segmentChanged = false;  // the value is not used in local file playback
            mMediaFormatParser->readFrame(stream, segmentChanged, -1);
        }
        return stream.peekNextFilledPacket();
    }

    MP4VRMediaPacket* MP4MediaStreamManager::getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs)
    {
        if (!stream.hasFilledPackets())
        {
            bool_t segmentChanged = false;  // the value is not used in local file playback
            mMediaFormatParser->readFrame(stream, segmentChanged, currentTimeUs);
        }
        return stream.peekNextFilledPacket();
    }

    Error::Enum MP4MediaStreamManager::readVideoFrames(int64_t currentTimeUs)
    {
        // Do nothing here on file playback
        return Error::OK;
    }
    Error::Enum MP4MediaStreamManager::readAudioFrames()
    {
        // Do nothing here on file playback
        return Error::OK;
    }

    const CoreProviderSourceTypes& MP4MediaStreamManager::getVideoSourceTypes()
    {
        return mMediaFormatParser->getVideoSourceTypes();
    }

    const CoreProviderSources& MP4MediaStreamManager::getVideoSources()
    {
        return mMediaFormatParser->getVideoSources();
    }

    bool_t MP4MediaStreamManager::isBuffering()
    {
        return false;
    }

    bool_t MP4MediaStreamManager::isEOS() const
    {
        return mMediaFormatParser->isEOS(mAudioStreams, mVideoStreams);
    }
    bool_t MP4MediaStreamManager::isReadyToSwitch(MP4MediaStream& aStream) const
    {
        return false;
    }

    bool_t MP4MediaStreamManager::seekToUs(uint64_t& seekPosUs, SeekDirection::Enum direction, SeekAccuracy::Enum accuracy)
    {
        return mMediaFormatParser->seekToUs(seekPosUs, mAudioStreams, mVideoStreams, direction, accuracy);
    }

    bool_t MP4MediaStreamManager::seekToFrame(int32_t seekFrameNr, uint64_t& seekPosUs)
    {
        return mMediaFormatParser->seekToFrame(seekFrameNr, seekPosUs, mAudioStreams, mVideoStreams);
    }

    uint64_t MP4MediaStreamManager::getNextVideoTimestampUs()
    {
        return mMediaFormatParser->getNextVideoTimestampUs(*mVideoStreams[0]);
    }

    // this is not a full seek as it is done for video only. It is used in alternate tracks case to find the time when the next track is ready to switch and pre-seek the next track for the switch
    int64_t MP4MediaStreamManager::findAndSeekSyncFrameMs(uint64_t currentTimeMs, MP4MediaStream* videoStream, SeekDirection::Enum direction)
    {
        // first find the sample that corresponds to the current time instant
        uint32_t sampleId = 0;
        uint64_t finalTimeMs = 0;
        videoStream->findSampleIdByTime(currentTimeMs, finalTimeMs, sampleId);
        bool_t segmentChanged; // not used in this context
        // then seek to the next/prev sync frame in the stream
        return mMediaFormatParser->seekToSyncFrame(sampleId, videoStream, direction, segmentChanged);
    }

    MediaInformation MP4MediaStreamManager::getMediaInformation()
    {
        if (mMediaFormatParser != OMAF_NULL)
        {
            return mMediaFormatParser->getMediaInformation();
        }
        else
        {
            return MediaInformation();
        }
    }

OMAF_NS_END
