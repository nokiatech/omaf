
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
#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaType.h"
#include "Metadata/NVRViewpointCoordSysRotation.h"
#include "Metadata/NVRViewpointUserAction.h"

OMAF_NS_BEGIN

const CoreProviderSources kEmptySources;

class MP4StreamManager
{
public:
    virtual void_t getAudioStreams(MP4AudioStreams& aMainAudioStreams, MP4AudioStreams& aAdditionalAudioStreams) = 0;
    virtual const MP4AudioStreams& getAudioStreams() = 0;
    virtual const MP4VideoStreams& getVideoStreams() = 0;
    virtual const MP4MetadataStreams& getMetadataStreams() = 0;

    virtual Error::Enum readVideoFrames(int64_t currentTimeUs) = 0;
    virtual Error::Enum readAudioFrames() = 0;
    virtual Error::Enum readMetadata(int64_t aCurrentTimeUs) = 0;

    virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream) = 0;
    virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs) = 0;
    virtual MP4VRMediaPacket* getMetadataFrame(MP4MediaStream& stream, int64_t currentTimeUs) = 0;

    virtual const CoreProviderSourceTypes& getVideoSourceTypes() = 0;

    virtual bool_t stopOverlayAudioStream(streamid_t aVideoStreamId, streamid_t& aStoppedAudioStreamId)
    {
        return false;
    }
    virtual MP4AudioStream* switchAudioStream(streamid_t aVideoStreamId,
                                              streamid_t& aStoppedAudioStreamId,
                                              bool_t aPreviousOverlayStopped)
    {
        return OMAF_NULL;
    }
    virtual bool_t switchOverlayVideoStream(streamid_t aVideoStreamId,
                                            int64_t currentTimeUs,
                                            bool_t& aPreviousOverlayStopped)
    {
        return false;
    }
    virtual bool_t stopOverlayVideoStream(streamid_t aVideoStreamId)
    {
        return false;
    }

    /**
     * Update new overlay data to internal state.
     *
     * @param metadataFrame Metadata packet whose info to update
     */
    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream, const MP4VRMediaPacket& metadataFrame) = 0;

    virtual bool_t getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const = 0;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const = 0;
    virtual size_t getNrOfViewpoints() const
    {
        return 1;
    }
    virtual Error::Enum getViewpointId(size_t aIndex, uint32_t& aId) const
    {
        return Error::NOT_SUPPORTED;
    }
    virtual size_t getCurrentViewpointIndex() const
    {
        return 0;
    }

    virtual bool_t isBuffering() = 0;
    virtual bool_t isEOS() const = 0;
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const = 0;

    virtual const CoreProviderSources& getPastBackgroundVideoSourcesAt(uint64_t aPts)
    {
        return kEmptySources;
    }
};


OMAF_NS_END
