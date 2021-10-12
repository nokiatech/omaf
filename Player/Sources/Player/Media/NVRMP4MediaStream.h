
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
#include <reader/mp4vrfiledatatypes.h>
#include <reader/mp4vrfilereaderinterface.h>
#include "Foundation/NVRFixedQueue.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Provider/NVRCoreProviderSources.h"

#include "Media/NVRMP4MediaPacketQueue.h"
#include "Metadata/NVRViewpointCoordSysRotation.h"
#include "Metadata/NVRViewpointSwitchConfig.h"
#include "Metadata/NVRViewpointUserAction.h"

#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

class MediaFormat;
class MP4VRMediaPacket;
class MemoryAllocator;
class MP4VRParser;
class MP4MediaStream;
typedef FixedArray<MP4MediaStream*, 64> MP4MetadataStreams;
typedef FixedArray<MP4MediaStream*, 64> MP4RefStreams;

class MP4MediaStream : public MP4VRMediaPacketQueue
{
public:
    MP4MediaStream(MediaFormat* format);
    virtual ~MP4MediaStream();

    virtual void_t setStartOffset(int64_t startOffset);

    virtual uint32_t getTrackId() const;
    virtual const MP4VR::TrackInformation* getTrack() const;
    virtual MediaFormat* getFormat() const;
    virtual uint32_t getConfigId() const;

    virtual void_t setStreamId(streamid_t id);
    virtual streamid_t getStreamId() const;

    virtual void_t setViewpointInfo(MP4VR::VipoEntityGroupProperty& aViewpointInfo);
    virtual bool_t isViewpoint() const;
    virtual uint32_t getViewpointId() const;
    virtual const char_t* getViewpointId(uint32_t& aId) const;
    virtual const ViewpointGlobalCoordinateSysRotation& getViewpointGCSRotation() const;
    virtual const ViewpointSwitchControls& getViewpointSwitchControls() const;
    virtual const ViewpointSwitchingList& getViewpointSwitchConfig() const;

    virtual void_t addMetadataStream(MP4MediaStream* stream);
    virtual const MP4MetadataStreams& getMetadataStreams() const;

    virtual void_t addRefStream(MP4VR::FourCC type, MP4MediaStream* stream);
    virtual const MP4RefStreams& getRefStreams(MP4VR::FourCC type) const;

    virtual void_t setTimestamps(MP4VR::MP4VRFileReaderInterface* reader);

    virtual uint32_t getMaxSampleSize() const;

    virtual void_t setTrack(MP4VR::TrackInformation* track);
    // peek, no index changed
    virtual Error::Enum peekNextSample(uint32_t& sample) const;
    virtual Error::Enum peekNextSampleTs(MP4VR::TimestampIDPair& sample) const;
    virtual Error::Enum peekNextSampleInfo(MP4VR::SampleInformation& sampleInfo) const;

    // also incements index by 1
    virtual Error::Enum getNextSample(uint32_t& sample,
                                      uint64_t& timeMs,
                                      bool_t& configChanged,
                                      uint32_t& configId,
                                      bool_t& segmentChanged);
    virtual Error::Enum getNextSampleTs(MP4VR::TimestampIDPair& sample);
    // used when seeking
    virtual bool_t findSampleIdByTime(uint64_t timeMs, uint64_t& finalTimeMs, uint32_t& sampleId);
    virtual bool_t findSampleIdByIndex(uint32_t sampleNr, uint32_t& sampleId);
    virtual void_t setNextSampleId(uint32_t id, bool_t& segmentChanged);
    virtual uint32_t getCurrentSegmentId() const;
    virtual uint32_t getSamplesLeft() const;
    virtual bool_t isCurrentSegmentInProgress() const;
    virtual bool_t isAtLastSegmentBoundary(uint32_t& aSegmentIndex) const;
    virtual bool_t isAtAnySegmentBoundary(uint32_t& aSegmentIndex) const;

    virtual uint64_t getCurrentReadPositionMs();
    virtual uint64_t getTimeLeftInCurrentSegmentMs() const;

    virtual uint64_t getFrameCount() const;
    virtual int32_t getCount() const;

    virtual bool_t isEoF() const;


    virtual bool_t IsVideo()
    {
        return mIsVideo;
    }
    virtual bool_t IsAudio()
    {
        return mIsAudio;
    }

    virtual uint32_t framesUntilNextSyncFrame(uint64_t& syncFrameTimeMs);
    virtual bool_t isReferenceFrame();

    virtual bool_t isUserActivateable();
    virtual void_t setUserActivateable();

public:
    // from MP4VRMediaPacketQueue
    virtual bool_t hasFilledPackets() const;
    virtual MP4VRMediaPacket* getNextFilledPacket();
    virtual MP4VRMediaPacket* peekNextFilledPacket();
    virtual bool_t popFirstFilledPacket();
    virtual void_t storeFilledPacket(MP4VRMediaPacket* packet);
    virtual MP4VRMediaPacket* getNextEmptyPacket();
    virtual void_t returnEmptyPacket(MP4VRMediaPacket* packet);
    virtual void_t resetPackets();


protected:
    MemoryAllocator& mAllocator;
    MediaFormat* mFormat;
    bool_t mIsAudio;
    bool_t mIsVideo;
    streamid_t mStreamId;

    int64_t mStartOffset;

    struct SampleIndex
    {
        SampleIndex()
            : mCurrentSegment(0)
            , mSegmentSampleIndex(0)
            , mGlobalStartSegment(0)
            , mGlobalSampleIndex(0)
        {
        }

        uint32_t mCurrentSegment;
        uint32_t mSegmentSampleIndex;
        uint32_t mGlobalStartSegment;
        uint32_t mGlobalSampleIndex;
    };


    SampleIndex mSampleIndex;

    // reference, not owned
    MP4VR::TrackInformation* mTrack;

    uint32_t mSamplesRead;

private:
    uint32_t mTrackId;

    MP4VR::DynArray<MP4VR::TimestampIDPair> mSampleTsPairs;

    // buffers allocated only when needed
    FixedQueue<MP4VRMediaPacket*, 100> mEmptyPackets;
    FixedQueue<MP4VRMediaPacket*, 100> mFilledPackets;

    MP4MetadataStreams mMetadataStreams;

    MP4RefStreams mRefStreams;

    uint32_t mNextRefFrame;

    bool_t mUserActivateable;

    bool_t mViewpoint;
    uint64_t mViewpointId;
    char_t* mViewpointLabel;
    ViewpointGlobalCoordinateSysRotation mViewpointGlobalSysRotation;
    ViewpointSwitchingList mViewpointSwitching;
    ViewpointSwitchControls mViewpointUserControls;
};

OMAF_NS_END
