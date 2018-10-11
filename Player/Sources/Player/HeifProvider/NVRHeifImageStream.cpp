
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
#include "NVRHeifImageStream.h"

#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(HeifImageStream)

    HeifImageStream::HeifImageStream(MediaFormat* format)
    : MP4VideoStream(format)
    , mPrimaryRead(false)
    {
    }

    HeifImageStream::~HeifImageStream()
    {
    }

    void_t HeifImageStream::setTrack(MP4VR::TrackInformation* track)
    {
    }

    void_t HeifImageStream::setTimestamps(MP4VR::MP4VRFileReaderInterface* reader)
    {
    }

    // peek next sample without changing index
    Error::Enum HeifImageStream::peekNextSample(uint32_t& sample) const
    {
        return Error::OK;
    }

    Error::Enum HeifImageStream::peekNextSampleTs(MP4VR::TimestampIDPair& sample) const
    {
        return Error::OK;
    }

    // get sample and incements index by 1, i.e. after this call index points to next sample; may point out of array
    Error::Enum HeifImageStream::getNextSample(uint32_t& sample, uint64_t& sampleTimeMs, bool_t& configChanged, uint32_t& configId, bool_t& segmentChanged)
    {
        return Error::OK;
    }

    // timestamp-table is an alternative to sample time; used with timed metadata, not with normal streams
    Error::Enum HeifImageStream::getNextSampleTs(MP4VR::TimestampIDPair& sample)
    {
        return Error::INVALID_STATE;
    }

    // used when seeking
    bool_t HeifImageStream::findSampleIdByTime(uint64_t timeMs, uint64_t& finalTimeMs, uint32_t& sampleId)
    {
        return false;
    }

    bool_t HeifImageStream::findSampleIdByIndex(uint32_t sampleNr, uint32_t& sampleId)
    {
        return false;
    }

    // used after seek
    void_t HeifImageStream::setNextSampleId(uint32_t id, bool_t& segmentChanged)
    {
    }

    uint32_t HeifImageStream::getSamplesLeft() const
    {
        return 0;
    }

    uint64_t HeifImageStream::getFrameCount() const
    {
        return 1;
    }

    // EoF or end of data in available segments
    bool_t HeifImageStream::isEoF() const
    {
        return true;
    }

    bool_t HeifImageStream::isReferenceFrame()
    {
        return true;
    }

    uint32_t HeifImageStream::getMaxSampleSize() const
    {
        return 1;//TODO?? Value = 0 breaks exec. Can't get the actual value beforehand like in mp4 case, can check it when reading.
    }

    bool_t HeifImageStream::isDepth()
    {
        return false;
    }

    void_t HeifImageStream::setPrimaryAsRead()
    {
        mPrimaryRead = true;
    }

    bool_t HeifImageStream::isPrimaryRead()
    {
        return mPrimaryRead;
    }
OMAF_NS_END
