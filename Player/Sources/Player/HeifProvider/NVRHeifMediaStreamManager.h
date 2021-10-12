
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
#pragma once

#include "NVREssentials.h"

#include "Media/NVRMP4StreamManager.h"

#include "Provider/NVRCoreProvider.h"

#include "Foundation/NVRPathName.h"
#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMediaFormat.h"

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

    virtual const CoreProviderSources& getVideoSources();

    MediaInformation getMediaInformation();

public:  // from MP4StreamManager
    virtual void_t getAudioStreams(MP4AudioStreams& aMainAudioStreams, MP4AudioStreams& aAdditionalAudioStreams);
    virtual const MP4AudioStreams& getAudioStreams();
    virtual const MP4VideoStreams& getVideoStreams();
    virtual const MP4MetadataStreams& getMetadataStreams();

    virtual Error::Enum readVideoFrames(int64_t currentTimeUs);
    virtual Error::Enum readAudioFrames();
    virtual Error::Enum readMetadata(int64_t aCurrentTimeUs);

    virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream);
    virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);
    virtual MP4VRMediaPacket* getMetadataFrame(MP4MediaStream& stream, int64_t currentTimeUs);

    virtual const CoreProviderSourceTypes& getVideoSourceTypes();

    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream, const MP4VRMediaPacket& metadataFrame)
    {
        return Error::OK;
    };

    virtual bool_t getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;

    virtual bool_t isBuffering();
    virtual bool_t isEOS() const;
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;

private:
    MemoryAllocator& mAllocator;
    // owned
    HeifParser* mMediaFormatParser;

    // Adding/removing streams are protected by the control and render mutexes; reading them can be done via either of
    // them
    MP4AudioStreams mAudioStreams;
    MP4VideoStreams mVideoStreams;
    MP4MetadataStreams mMetadataStreams;
};
OMAF_NS_END
