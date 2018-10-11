
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
#pragma once

#include "NVREssentials.h"

#include "Media/NVRMP4StreamManager.h"

#include "Provider/NVRCoreProvider.h"

#include "Media/NVRMediaFormat.h"
#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "Foundation/NVRPathName.h"

OMAF_NS_BEGIN

class HeifParser;

class HeifMediaStreamManager : public MP4StreamManager
    {
    public:
        HeifMediaStreamManager();
        virtual ~HeifMediaStreamManager();

        virtual Error::Enum openInput(const PathName& mediaUri);

        Error::Enum selectNextImage();

        virtual void_t resetStreams();

        virtual const MP4AudioStreams& getAudioStreams();
        virtual const MP4VideoStreams& getVideoStreams();
        virtual int32_t getNrStreams();

        virtual Error::Enum readVideoFrames(int64_t currentTimeUs);
        virtual Error::Enum readAudioFrames();

        virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream);
        virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);

        virtual const CoreProviderSourceTypes& getVideoSourceTypes();
        virtual const CoreProviderSources& getVideoSources();

        virtual bool_t isBuffering();
        virtual bool_t isEOS() const;
        virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;

        virtual bool_t seekToUs(uint64_t& seekPosUs, SeekDirection::Enum direction, SeekAccuracy::Enum accuracy);
        virtual bool_t seekToFrame(int32_t seekFrameNr, uint64_t& seekPosUs);// , SeekSyncFrame::Enum mode);

        virtual uint64_t getNextVideoTimestampUs();

        virtual int64_t findAndSeekSyncFrameMs(uint64_t currentTimeMs, MP4MediaStream* videoStream, SeekDirection::Enum direction);

        MediaInformation getMediaInformation();

    private:
        MemoryAllocator& mAllocator;
        // owned
        HeifParser* mMediaFormatParser;

        // Adding/removing streams are protected by the control and render mutexes; reading them can be done via either of them
        MP4AudioStreams mAudioStreams;
        MP4VideoStreams mVideoStreams;

    };
OMAF_NS_END
