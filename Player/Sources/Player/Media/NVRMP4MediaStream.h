
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
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedQueue.h"
#include "Provider/NVRCoreProviderSources.h"
#include <mp4vrfiledatatypes.h>
#include <mp4vrfilereaderinterface.h>

#include "Media/NVRMP4MediaPacketQueue.h"

#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

    class MediaFormat;
    class MP4VRMediaPacket;
    class MemoryAllocator;

    class MP4MediaStream : public MP4VRMediaPacketQueue
    {
    public:
        MP4MediaStream(MediaFormat* format);
        virtual ~MP4MediaStream();

        virtual uint32_t getTrackId() const;
        virtual MediaFormat* getFormat() const;
        virtual uint32_t getConfigId() const;

        virtual void_t setStreamId(streamid_t id);
        virtual streamid_t getStreamId() const;

        virtual void_t setMetadataStream(MP4MediaStream* stream);
        virtual MP4MediaStream* getMetadataStream();
        virtual MP4MediaStream* getMetadataStream(const char_t* aFourCC);

        virtual void_t setTimestamps(MP4VR::MP4VRFileReaderInterface* reader);

        virtual uint32_t getMaxSampleSize() const;

        virtual void_t setTrack(MP4VR::TrackInformation* track);
        // peek, no index changed
        virtual Error::Enum peekNextSample(uint32_t& sample) const;
        virtual Error::Enum peekNextSampleTs(MP4VR::TimestampIDPair& sample) const;
        // also incements index by 1
        virtual Error::Enum getNextSample(uint32_t& sample, uint64_t& timeMs, bool_t& configChanged, uint32_t& configId, bool_t& segmentChanged);
        virtual Error::Enum getNextSampleTs(MP4VR::TimestampIDPair& sample);
        // used when seeking
        virtual bool_t findSampleIdByTime(uint64_t timeMs, uint64_t& finalTimeMs, uint32_t& sampleId);
        virtual bool_t findSampleIdByIndex(uint32_t sampleNr, uint32_t& sampleId);
        virtual void_t setNextSampleId(uint32_t id, bool_t& segmentChanged);
        virtual uint32_t getCurrentSegmentId() const;
        virtual uint32_t getSamplesLeft() const;
        virtual bool_t isCurrentSegmentInProgress() const;

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

        struct SampleIndex
        {
            SampleIndex()
                : mCurrentSegment(0)
                , mSegmentSampleIndex(0)
                , mGlobalStartSegment(0)
                , mGlobalSampleIndex(0)
            {}
                
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

        //buffers allocated only when needed
        FixedQueue<MP4VRMediaPacket*, 100> mEmptyPackets;
        FixedQueue<MP4VRMediaPacket*, 100> mFilledPackets;

        // reference, now owned, but can be NULL so pointer used
        MP4MediaStream* mMetadataStream;

        uint32_t mNextRefFrame;

    };
OMAF_NS_END
