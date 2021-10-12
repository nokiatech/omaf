
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
#include "Media/NVRMP4Parser.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMediaFormat.h"

OMAF_NS_BEGIN

class MP4MediaStreamManager : public MP4StreamManager, public MP4StreamCreator
{
public:
    MP4MediaStreamManager();
    virtual ~MP4MediaStreamManager();

    virtual Error::Enum openInput(const PathName& mediaUri);

    virtual void_t resetStreams();

    virtual int32_t getNrStreams();

    virtual bool_t seekToUs(uint64_t& seekPosUs, SeekDirection::Enum direction, SeekAccuracy::Enum accuracy);
    virtual bool_t seekToFrame(int32_t seekFrameNr, uint64_t& seekPosUs);  // , SeekSyncFrame::Enum mode);

    virtual uint64_t getNextVideoTimestampUs();

    virtual int64_t findAndSeekSyncFrameMs(uint64_t currentTimeMs,
                                           MP4MediaStream* videoStream,
                                           SeekDirection::Enum direction);

    virtual const CoreProviderSources& getVideoSources();
    virtual const CoreProviderSources& getAllVideoSources();

    MediaInformation getMediaInformation();

    virtual Error::Enum setViewpoint(uint32_t aDestinationId);
    virtual bool_t isViewpointSwitchReadyToComplete(uint32_t aCurrentPlayTimeMs);
    virtual void_t completeViewpointSwitch(bool_t& aBgAudioContinues);


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

    virtual bool_t stopOverlayAudioStream(streamid_t aVideoStreamId, streamid_t& aAudioStreamId);
    virtual MP4AudioStream* switchAudioStream(streamid_t aVideoStreamId,
                                              streamid_t& aStoppedAudioStreamId,
                                              bool_t aPreviousOverlayStopped);
    virtual bool_t switchOverlayVideoStream(streamid_t aVideoStreamId,
                                            int64_t currentTimeUs,
                                            bool_t& aPreviousOverlayStopped);
    virtual bool_t stopOverlayVideoStream(streamid_t aVideoStreamId);

    /**
     * Inform parser that new metadata packet was read and now would be good time to apply it to the internal state.
     */
    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream, const MP4VRMediaPacket& metadataFrame);

    virtual bool_t getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;
    virtual size_t getNrOfViewpoints() const;
    virtual Error::Enum getViewpointId(size_t aIndex, uint32_t& aId) const;
    virtual size_t getCurrentViewpointIndex() const;

    virtual bool_t isBuffering();
    virtual bool_t isEOS() const;
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;


public:  // from MP4StreamCreator
    virtual MP4MediaStream* createStream(MediaFormat* aFormat) const;
    virtual MP4VideoStream* createVideoStream(MediaFormat* aFormat) const;
    virtual MP4AudioStream* createAudioStream(MediaFormat* aFormat) const;
    virtual void_t destroyStream(MP4MediaStream* aStream) const;

private:
    struct Mp4Viewpoint
    {
        uint32_t viewpointId;
        const char_t* viewpointLabel;
        ViewpointSwitchingList viewpointSwitchingList;
        MP4VideoStreams videoStreams;
        MP4AudioStreams audioStreams;
        MP4MetadataStreams metadataStreams;
    };

    Error::Enum selectInitialViewpoint();
    Mp4Viewpoint* findViewpoint(uint32_t aViewpointId);
    void_t selectStreams();

private:
    MemoryAllocator& mAllocator;
    // owned
    MP4VRParser* mMediaFormatParser;

    // Adding/removing streams are protected by the control and render mutexes; reading them can be done via either of
    // them
    MP4AudioStreams mAllAudioStreams;
    MP4AudioStreams mAudioStreams;
    MP4AudioStreams mActiveAudioStreams;

    MP4VideoStreams mAllVideoStreams;
    MP4VideoStreams mVideoStreams;
    MP4VideoStreams mActiveVideoStreams;

    MP4MetadataStreams mAllMetadataStreams;
    MP4MetadataStreams mMetadataStreams;

    CoreProviderSources mActiveVideoSources;
    MP4AudioStream* mBackgroundAudioStream;

    FixedArray<Mp4Viewpoint*, 64> mViewpoints;
    size_t mCurrentViewpointIndex;
    size_t mNewViewpointIndex;
    uint64_t mSwitchTimeMs;
};
OMAF_NS_END
