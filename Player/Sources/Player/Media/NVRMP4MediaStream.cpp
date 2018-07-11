
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
#include "Media/NVRMP4MediaStream.h"

#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(MP4MediaStream)

    MP4MediaStream::MP4MediaStream(MediaFormat* format)
    : mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mFormat(format)
    , mIsAudio(false)
    , mIsVideo(false)
    , mStreamId(OMAF_UINT8_MAX)
    , mSampleIndex()
    , mTrack(OMAF_NULL)
    , mTrackId(OMAF_UINT32_MAX)
    , mSampleTsPairs()
    , mEmptyPackets()
    , mFilledPackets()
    , mMetadataStream(OMAF_NULL)
    , mSamplesRead(0)
    , mNextRefFrame(0)
    {
    }

    MP4MediaStream::~MP4MediaStream()
    {
        //free buffers
        while (!mFilledPackets.isEmpty())
        {
            MP4VRMediaPacket* p = getNextFilledPacket();
            if (p != OMAF_NULL)
            {
                OMAF_DELETE(mAllocator, p);
            }
        }
        while (!mEmptyPackets.isEmpty())
        {
            MP4VRMediaPacket* p = getNextEmptyPacket();
            if (p != OMAF_NULL)
            {
                OMAF_DELETE(mAllocator, p);
            }
        }
        OMAF_DELETE(mAllocator, mFormat);
        if (mMetadataStream != OMAF_NULL)
        {
            OMAF_DELETE(mAllocator, mMetadataStream);
        }
    }

    uint32_t MP4MediaStream::getTrackId() const
    {
        return mTrackId;
    }

    void_t MP4MediaStream::setStreamId(streamid_t id)
    {
        mStreamId = id;
    }
    streamid_t MP4MediaStream::getStreamId() const
    {
        return mStreamId;
    }

    void_t MP4MediaStream::setMetadataStream(MP4MediaStream* stream)
    {
        mMetadataStream = stream;
    }

    MP4MediaStream* MP4MediaStream::getMetadataStream()
    {
        return mMetadataStream;
    }

    MP4MediaStream* MP4MediaStream::getMetadataStream(const char_t* aFourCC)
    {
        if (StringCompare(mMetadataStream->getFormat()->getFourCC(), aFourCC) == ComparisonResult::EQUAL)
        {
            return mMetadataStream;
        }
        else
        {
            return OMAF_NULL;
        }
    }

    MediaFormat* MP4MediaStream::getFormat() const
    {
        return mFormat;
    }

    uint32_t MP4MediaStream::getConfigId() const
    {
        if (mTrack == OMAF_NULL || mSampleIndex.mGlobalSampleIndex > mTrack->sampleProperties.size)
        {
            return 0;
        }
        return mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleDescriptionIndex;
    }

    int32_t MP4MediaStream::getCount() const
    {
        return mSamplesRead;
    }

    void_t MP4MediaStream::setTrack(MP4VR::TrackInformation* track)
    {
        if (track == OMAF_NULL)
        {
            // keep track ID since it is needed if new track data is reassigned after this
            mTrack = OMAF_NULL;
            return;
        }

        mTrackId = track->trackId;

        // sync the global index
        if (track->sampleProperties.size > 0)
        {
            if (track->sampleProperties[0].segmentId > mSampleIndex.mGlobalStartSegment)
            {
                // The very first segment is now added, or the old first segment in index struct (mGlobalStartSegment) is no longer the first one
                if (mSampleIndex.mCurrentSegment < track->sampleProperties[0].segmentId)
                {
                    // The very first segment is now added, or we were reading from a segment that became invalid
                    mSampleIndex.mGlobalSampleIndex = 0;
                    mSampleIndex.mSegmentSampleIndex = 0;
                    mSampleIndex.mCurrentSegment = track->sampleProperties[0].segmentId;
                    mNextRefFrame = 0;
                    OMAF_LOG_V("setTrack reset indices for %d", getStreamId());//track->trackId);
                }
                else
                {
                    // the mCurrentSegment is still valid, but start segment is different than previously (if-condition above)
                    // segment current-N was removed, count how many samples there still are before the current segment
                    // if N == 1 (1 or more segments removed, mCurrentSegment becomes the first one): 
                    //         the while loop is skipped since track->sampleProperties[index].segmentId == mSampleIndex.mCurrentSegment; 
                    //         just use the index inside the current segment as the global index
                    // if N > 1 (1 or more segments removed, but some segments are still valid before the mCurrentSegment): the while loop becomes active
                    int32_t samplesInPrevSegments = 0;
                    while (track->sampleProperties[samplesInPrevSegments].segmentId < mSampleIndex.mCurrentSegment)
                    {
                        samplesInPrevSegments++;
                    }
                    mSampleIndex.mGlobalSampleIndex = samplesInPrevSegments + mSampleIndex.mSegmentSampleIndex;
                    mNextRefFrame = 0;
                    //OMAF_LOG_D("setTrack set mGlobalSampleIndex to %d for %d", mSampleIndex.mGlobalSampleIndex, getStreamId());//track->trackId);
                }

                mSampleIndex.mGlobalStartSegment = track->sampleProperties[0].segmentId;
            }
            else
            {
                //OMAF_LOG_V("setTrack %d segmentIds %d %d", getStreamId(),track->sampleProperties[0].segmentId, mSampleIndex.mGlobalStartSegment);
            }
        }
        else
        {
            // empty track
            mSampleIndex.mCurrentSegment = 0;
            mSampleIndex.mSegmentSampleIndex = 0;
            mSampleIndex.mGlobalSampleIndex = 0;
            mSampleIndex.mGlobalStartSegment = 0;
            mNextRefFrame = 0;
            OMAF_LOG_D("setTrack set track without samples! %d", getStreamId());//track->trackId);
        }
        mTrack = track;
    }

    void_t MP4MediaStream::setTimestamps(MP4VR::MP4VRFileReaderInterface* reader)
    {
        uint32_t result = reader->getTrackTimestamps(mTrackId, mSampleTsPairs);
    }

    // peek next sample without changing index
    Error::Enum MP4MediaStream::peekNextSample(uint32_t& sample) const
    {
        if (mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size)
        {
            return Error::END_OF_FILE;
        }
        sample = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleId;
        return Error::OK;
    }

    Error::Enum MP4MediaStream::peekNextSampleTs(MP4VR::TimestampIDPair& sample) const
    {
        if (mSampleIndex.mGlobalSampleIndex >= mSampleTsPairs.size)
        {
            return Error::END_OF_FILE;
        }
        sample = mSampleTsPairs[mSampleIndex.mGlobalSampleIndex];
        return Error::OK;
    }

    // get sample and incements index by 1, i.e. after this call index points to next sample; may point out of array
    Error::Enum MP4MediaStream::getNextSample(uint32_t& sample, uint64_t& sampleTimeMs, bool_t& configChanged, uint32_t& configId, bool_t& segmentChanged)
    {
        if (mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size)
        {
            return Error::END_OF_FILE;
        }

        if (mFormat->getDecoderConfigInfoId() != 0 && mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleDescriptionIndex != mFormat->getDecoderConfigInfoId())
        {
            OMAF_LOG_D("Decoder configuration changed!");
            configChanged = true;
            configId = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleDescriptionIndex;
        }
        else
        {
            configChanged = false;
        }

        if (mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId != mSampleIndex.mCurrentSegment)
        {
            OMAF_LOG_V("getNextSample: Move to next segment %d", mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId);
            mSampleIndex.mCurrentSegment = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId;
            mSampleIndex.mSegmentSampleIndex = 1;
            segmentChanged = true;
        }
        else
        {
            mSampleIndex.mSegmentSampleIndex++;
            segmentChanged = false;
        }

        sample = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleId;
        sampleTimeMs = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].earliestTimestamp;

        mSampleIndex.mGlobalSampleIndex++;

        mSamplesRead++;
        return Error::OK;
    }

    // timestamp-table is an alternative to sample time; used with timed metadata, not with normal streams
    Error::Enum MP4MediaStream::getNextSampleTs(MP4VR::TimestampIDPair& sample)
    {
        if (mSampleIndex.mGlobalSampleIndex >= mSampleTsPairs.size)
        {
            return Error::END_OF_FILE;
        }
        if (mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId != mSampleIndex.mCurrentSegment)
        {
            mSampleIndex.mCurrentSegment = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId;
            mSampleIndex.mSegmentSampleIndex = 1;
        }
        else
        {
            mSampleIndex.mSegmentSampleIndex++;
        }

        sample = mSampleTsPairs[mSampleIndex.mGlobalSampleIndex];
        mSampleIndex.mGlobalSampleIndex++;

        mSamplesRead++;
        return Error::OK;
    }

    // used when seeking
    bool_t MP4MediaStream::findSampleIdByTime(uint64_t timeMs, uint64_t& finalTimeMs, uint32_t& sampleId)
    {
        for (MP4VR::TimestampIDPair *pair = mSampleTsPairs.begin();
            pair != mSampleTsPairs.end(); pair++)
        {
            if (pair->timeStamp >= timeMs)
            {
                sampleId = pair->itemId;
                finalTimeMs = pair->timeStamp;
                return true;
            }
        }

        return false;
    }

    bool_t MP4MediaStream::findSampleIdByIndex(uint32_t sampleNr, uint32_t& sampleId)
    {
        if (mSampleTsPairs.size > sampleNr)
        {
            sampleId = mSampleTsPairs[sampleNr].itemId;
            return true;
        }

        return false;
    }

    // used after seek
    void_t MP4MediaStream::setNextSampleId(uint32_t id, bool_t& segmentChanged)
    {
        OMAF_ASSERT(mTrack != OMAF_NULL, "No track assigned");
        size_t indexInSegment = 0;
        for (size_t index = 0; index < mTrack->sampleProperties.size; index++)
        {
            if (mTrack->sampleProperties[index].sampleId == id)
            {
                mSampleIndex.mGlobalSampleIndex = static_cast<uint32_t>(index);
                mSampleIndex.mSegmentSampleIndex = static_cast<uint32_t>(indexInSegment);
                mNextRefFrame = 0;
                break;
            }
            if (mTrack->sampleProperties[index].segmentId == mSampleIndex.mCurrentSegment)
            {
                indexInSegment++;
            }
            else
            {
                // the correct segment should have been selected in the initial seeking phase, and here we are just seeking inside a segment
                OMAF_ASSERT(true, "Seeking into a different segment!!");
            }
        }
    }

    uint32_t MP4MediaStream::getCurrentSegmentId() const
    {
        return mSampleIndex.mCurrentSegment;
    }

    uint32_t MP4MediaStream::getSamplesLeft() const
    {
        return (mTrack->sampleProperties.size - (mSampleIndex.mGlobalSampleIndex-1));
    }

    bool_t MP4MediaStream::isCurrentSegmentInProgress() const
    {
        if (mSampleIndex.mSegmentSampleIndex > 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    uint64_t MP4MediaStream::getCurrentReadPositionMs()
    {
        uint64_t sampleTimeMs = 0;
        if (mTrack != OMAF_NULL && mTrack->sampleProperties.size > 0)
        {
            uint32_t index = 0;
            if (mSampleIndex.mGlobalSampleIndex > 0)
            {
                index = mSampleIndex.mGlobalSampleIndex-1;
            }
            sampleTimeMs = mTrack->sampleProperties[index].earliestTimestamp;
        }
        return sampleTimeMs;
    }

    uint64_t MP4MediaStream::getTimeLeftInCurrentSegmentMs() const
    {
        if (mTrack->sampleProperties.size == 0 || mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size)
        {
            return 0;
        }
        uint64_t now = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].earliestTimestamp;
        uint64_t end = 0;

        // must not use mSampleIndex.mCurrentSegment since it may be 1 segment behind because mGlobalSampleIndex points to the next sample to be read, and mCurrentSegment points to the last segment read
        uint32_t currentSegment = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId;
        for (uint32_t i = mSampleIndex.mGlobalSampleIndex; i < mTrack->sampleProperties.size; i++)
        {
            if (mTrack->sampleProperties[i].segmentId != currentSegment)
            {
                end = mTrack->sampleProperties[i-1].earliestTimestamp;
                break;
            }
        }
        if (end == 0)
        {
            // there is no newer segment available, take the timestamp of the last available sample
            end = mTrack->sampleProperties[mTrack->sampleProperties.size-1].earliestTimestamp;
        }
        return (end - now);
    }

    uint64_t MP4MediaStream::getFrameCount() const
    {
        OMAF_ASSERT(mTrack != OMAF_NULL, "No track assigned");
        return mTrack->sampleProperties.size;
    }

    // EoF or end of data in available segments
    bool_t MP4MediaStream::isEoF() const
    {
        OMAF_ASSERT(mTrack != OMAF_NULL, "No track assigned");
        return (mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size);
    }

    uint32_t MP4MediaStream::framesUntilNextSyncFrame(uint64_t& syncFrameTimeMs)
    {
        if (mNextRefFrame < mSampleIndex.mGlobalSampleIndex)
        {
            // previously found frame index is out of date
            // find it
            uint32_t count = mTrack->sampleProperties.size;
            bool_t found = false;
            for (uint32_t i = mSampleIndex.mGlobalSampleIndex; i < count; i++)
            {
                if (mTrack->sampleProperties[i].sampleType == MP4VR::OUTPUT_REFERENCE_FRAME)
                {
                    found = true;
                    mNextRefFrame = i;
                    OMAF_LOG_D("next ref frame %d", mNextRefFrame);
                    break;
                }
            }
            if (!found)
            {
                // couldn't find it, it must be in the next segment that is not yet available
                syncFrameTimeMs = mTrack->sampleProperties[mTrack->sampleProperties.size-1].earliestTimestamp + 33; // guessing frame interval
                return mTrack->sampleProperties.size - mSampleIndex.mGlobalSampleIndex;
            }
        }
        syncFrameTimeMs = mTrack->sampleProperties[mNextRefFrame].earliestTimestamp;
        return mNextRefFrame - mSampleIndex.mGlobalSampleIndex + 1;
    }

    bool_t MP4MediaStream::isReferenceFrame()
    {
        if (mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex-1].sampleType == MP4VR::OUTPUT_REFERENCE_FRAME)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    uint32_t MP4MediaStream::getMaxSampleSize() const
    {
        OMAF_ASSERT(mTrack != OMAF_NULL, "No track assigned");
        return mTrack->maxSampleSize;
    }

    bool_t MP4MediaStream::hasFilledPackets() const
    {
        return !mFilledPackets.isEmpty();
    }

    MP4VRMediaPacket* MP4MediaStream::getNextFilledPacket()
    {
        if (mFilledPackets.isEmpty())
        {
            return OMAF_NULL;
        }
        MP4VRMediaPacket* packet = mFilledPackets.front();
        if (packet != OMAF_NULL)
        {
            mFilledPackets.pop();
        }
        return packet;
    }

    MP4VRMediaPacket* MP4MediaStream::peekNextFilledPacket()
    {
        if (mFilledPackets.isEmpty())
        {
            return OMAF_NULL;
        }
        MP4VRMediaPacket* packet = mFilledPackets.front();
        return packet;
    }

    bool_t MP4MediaStream::popFirstFilledPacket()
    {
        MP4VRMediaPacket* packet = mFilledPackets.front();
        if (packet != OMAF_NULL)
        {
            mFilledPackets.pop();
            return true;
        }
        return false;
    }

    void_t MP4MediaStream::storeFilledPacket(MP4VRMediaPacket* packet)
    {
        OMAF_ASSERT(packet != OMAF_NULL, "");
        mFilledPackets.push(packet);
    }

    MP4VRMediaPacket* MP4MediaStream::getNextEmptyPacket()
    {
        MP4VRMediaPacket* packet = OMAF_NULL;
        if (mEmptyPackets.isEmpty())
        {
            if (getMaxSampleSize() == 0)
            {
                OMAF_LOG_D("No track assigned, cannot determine track max packet size");
                return OMAF_NULL;
            }
            else if (mFilledPackets.getSize() < mFilledPackets.getCapacity())
            {
                packet = OMAF_NEW(mAllocator, MP4VRMediaPacket)(getMaxSampleSize(), mMetadataStream);
            }
            else
            {
                return OMAF_NULL;
            }
        }
        else
        { 
            packet = mEmptyPackets.front();
            mEmptyPackets.pop();
        }
        return packet;
    }

    void_t MP4MediaStream::returnEmptyPacket(MP4VRMediaPacket* packet)
    {
        OMAF_ASSERT(packet != OMAF_NULL, "");
        packet->setDataSize(0);
        mEmptyPackets.push(packet);
    }

    void_t MP4MediaStream::resetPackets()
    {
        while (!mFilledPackets.isEmpty())
        {
            returnEmptyPacket(getNextFilledPacket());
        }
    }

OMAF_NS_END
