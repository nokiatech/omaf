
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
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4VideoStream.h"
OMAF_NS_BEGIN

class MP4StreamManager
{
public:
    virtual const MP4AudioStreams& getAudioStreams() = 0;
    virtual const MP4VideoStreams& getVideoStreams() = 0;
    virtual const MP4MetadataStreams& getMetadataStreams() = 0;

    virtual Error::Enum readVideoFrames(int64_t currentTimeUs) = 0;
    virtual Error::Enum readAudioFrames() = 0;
    virtual Error::Enum readMetadata() = 0;

    virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream) = 0;
    virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs) = 0;
    virtual MP4VRMediaPacket* getMetadataFrame(MP4MediaStream& stream, int64_t currentTimeUs) = 0;

    virtual bool_t isBuffering() = 0;
    virtual bool_t isEOS() const = 0;
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const = 0;
};


OMAF_NS_END
