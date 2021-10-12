
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
#include "Media/NVRMP4MediaStream.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Media/NVRMP4Parser.h"
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"
#include "Metadata/NVRViewpointCoordSysRotation.h"
#include "Metadata/NVRViewpointUserAction.h"

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
    , mMetadataStreams()
    , mSamplesRead(0)
    , mNextRefFrame(0)
    , mStartOffset(0)
    , mUserActivateable(false)
    , mViewpoint(false)
    , mViewpointId(0)
    , mViewpointLabel(OMAF_NULL)
    , mViewpointGlobalSysRotation()
    , mViewpointUserControls()
{
}

MP4MediaStream::~MP4MediaStream()
{
    // free buffers
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
    OMAF_DELETE_ARRAY_HEAP(mViewpointLabel);
    OMAF_DELETE(mAllocator, mFormat);
}

void_t MP4MediaStream::setStartOffset(int64_t startOffset)
{
    mStartOffset = startOffset;
}

uint32_t MP4MediaStream::getTrackId() const
{
    return mTrackId;
}

const MP4VR::TrackInformation* MP4MediaStream::getTrack() const
{
    return mTrack;
}

void_t MP4MediaStream::setStreamId(streamid_t id)
{
    mStreamId = id;
}
streamid_t MP4MediaStream::getStreamId() const
{
    return mStreamId;
}

void_t MP4MediaStream::setViewpointInfo(MP4VR::VipoEntityGroupProperty& aViewpointInfo)
{
    mViewpoint = true;
    mViewpointId = aViewpointInfo.viewpointId;

	OMAF_ASSERT(mViewpointLabel == OMAF_NULL, "Trying to set viewpoint again to the same stream.");
    mViewpointLabel = OMAF_NEW_ARRAY_HEAP(char_t, aViewpointInfo.viewpointLabel.size + 1); 
    std::copy(aViewpointInfo.viewpointLabel.begin(), aViewpointInfo.viewpointLabel.end(), mViewpointLabel);
    mViewpointLabel[aViewpointInfo.viewpointLabel.size] = '\0';

    // copy gcs to a struct to pass on to provider-level
    mViewpointGlobalSysRotation.yaw = aViewpointInfo.viewpointGlobalCoordinateSysRotation.viewpointGcsYaw;
    mViewpointGlobalSysRotation.pitch = aViewpointInfo.viewpointGlobalCoordinateSysRotation.viewpointGcsPitch;
    mViewpointGlobalSysRotation.roll = aViewpointInfo.viewpointGlobalCoordinateSysRotation.viewpointGcsRoll;


    // collect info for user controls
    mViewpointUserControls.clear();

	if (aViewpointInfo.viewpointSwitchingList)
    {
        auto& switchingList = aViewpointInfo.viewpointSwitchingList.get();

		for (auto& switchingItem : switchingList.viewpointSwitching)
        {
            ViewpointSwitchControl vpSwitchControl;
            ViewpointSwitching vpSwitchConfig;

            vpSwitchConfig.destinationViewpoint = 
				vpSwitchControl.destinationViewpointId = switchingItem.destinationViewpointId;

            vpSwitchControl.switchActivationStart = false;
            vpSwitchControl.switchActivationEnd = false;
            
            if (switchingItem.viewpointTimelineSwitch)
            {
                auto& timelineSwitch = switchingItem.viewpointTimelineSwitch.get();

                if (timelineSwitch.maxTime)
                {
                    vpSwitchControl.switchActivationEnd = true;
                    vpSwitchControl.switchActivationEndTime = *timelineSwitch.maxTime;
                }
                if (timelineSwitch.minTime)
                {
                    vpSwitchControl.switchActivationStart = true;
                    vpSwitchControl.switchActivationStartTime = *timelineSwitch.minTime;
                }

                // timeline modifications
                // Note that we expect the offsets to be in ms at this point
                switch (timelineSwitch.tOffset.getKey())
                {
                case ISOBMFF::OffsetKind::ABSOLUTE_TIME:
                    vpSwitchConfig.absoluteOffsetUsed = true;
                    vpSwitchConfig.absoluteOffset = timelineSwitch.tOffset.at<ISOBMFF::OffsetKind::ABSOLUTE_TIME>();
                    break;
                case ISOBMFF::OffsetKind::RELATIVE_TIME:
                    vpSwitchConfig.absoluteOffsetUsed = false;
                    vpSwitchConfig.relativeOffset = timelineSwitch.tOffset.at<ISOBMFF::OffsetKind::RELATIVE_TIME>();
                    break;
                }
            }

            for (ISOBMFF::ViewpointSwitchRegionStruct& region : switchingItem.viewpointSwitchRegions)
            {
                OMAF_LOG_E("To be fixed: Only the last overlay switch region is recorded");
                switch (region.region.getKey())
                {
                case ISOBMFF::ViewpointRegionType::OVERLAY:
                {
                    vpSwitchControl.viewpointSwitchRegion.type = ViewpointSwitchRegionType::OVERLAY;
                    vpSwitchControl.viewpointSwitchRegion.refOverlayId =
                        region.region.at<ISOBMFF::ViewpointRegionType::OVERLAY>();
                }
                break;
                case ISOBMFF::ViewpointRegionType::SPHERE_RELATIVE:
                {
                }
                break;
                case ISOBMFF::ViewpointRegionType::VIEWPORT_RELATIVE:
                {
                }
                break;
                }
            }
        
			mViewpointUserControls.add(vpSwitchControl);
            mViewpointSwitching.add(vpSwitchConfig);
        }
    }
}

bool_t MP4MediaStream::isViewpoint() const
{
    return mViewpoint;
}

uint32_t MP4MediaStream::getViewpointId() const
{
    return mViewpointId;
}

const char_t* MP4MediaStream::getViewpointId(uint32_t& aId) const
{
    aId = mViewpointId;
    return mViewpointLabel;
}

const ViewpointGlobalCoordinateSysRotation& MP4MediaStream::getViewpointGCSRotation() const
{
    return mViewpointGlobalSysRotation;
}

const ViewpointSwitchControls& MP4MediaStream::getViewpointSwitchControls() const
{
    return mViewpointUserControls;
}

const ViewpointSwitchingList& MP4MediaStream::getViewpointSwitchConfig() const
{
    return mViewpointSwitching;
}

void_t MP4MediaStream::addMetadataStream(MP4MediaStream* stream)
{
    mMetadataStreams.add(stream);
}

const MP4MetadataStreams& MP4MediaStream::getMetadataStreams() const
{
    return mMetadataStreams;
}

void_t MP4MediaStream::addRefStream(MP4VR::FourCC type, MP4MediaStream* stream)
{
    // currently support only cdsc references
    OMAF_ASSERT(type == "cdsc" || type == "soun", "Ref stream must be cdsc type");
    mRefStreams.add(stream);
}

const MP4RefStreams& MP4MediaStream::getRefStreams(MP4VR::FourCC type) const
{
    // currently support only cdsc references
    OMAF_ASSERT(type == "cdsc" || type == "soun", "Ref stream must be cdsc type");
    return mRefStreams;
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
            // The very first segment is now added, or the old first segment in index struct (mGlobalStartSegment) is no
            // longer the first one
            if (mSampleIndex.mCurrentSegment < track->sampleProperties[0].segmentId)
            {
                // The very first segment is now added, or we were reading from a segment that became invalid
                mSampleIndex.mGlobalSampleIndex = 0;
                mSampleIndex.mSegmentSampleIndex = 0;
                mSampleIndex.mCurrentSegment = track->sampleProperties[0].segmentId;
                mNextRefFrame = 0;
                OMAF_LOG_V("setTrack reset indices for %d", getStreamId());  // track->trackId);
            }
            else
            {
                // the mCurrentSegment is still valid, but start segment is different than previously (if-condition
                // above)
                // segment current-N was removed, count how many samples there still are before the current segment
                // if N == 1 (1 or more segments removed, mCurrentSegment becomes the first one):
                //         the while loop is skipped since track->sampleProperties[index].segmentId ==
                //         mSampleIndex.mCurrentSegment;
                //         just use the index inside the current segment as the global index
                // if N > 1 (1 or more segments removed, but some segments are still valid before the mCurrentSegment):
                // the while loop becomes active
                int32_t samplesInPrevSegments = 0;
                while (track->sampleProperties[samplesInPrevSegments].segmentId < mSampleIndex.mCurrentSegment)
                {
                    samplesInPrevSegments++;
                }
                mSampleIndex.mGlobalSampleIndex = samplesInPrevSegments + mSampleIndex.mSegmentSampleIndex;
                mNextRefFrame = 0;
                // OMAF_LOG_D("setTrack set mGlobalSampleIndex to %d for %d", mSampleIndex.mGlobalSampleIndex,
                // getStreamId());//track->trackId);
            }

            mSampleIndex.mGlobalStartSegment = track->sampleProperties[0].segmentId;
        }
        else
        {
            // OMAF_LOG_V("setTrack %d segmentIds %d %d", getStreamId(),track->sampleProperties[0].segmentId,
            // mSampleIndex.mGlobalStartSegment);
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
        OMAF_LOG_D("setTrack set track without samples! %d", getStreamId());  // track->trackId);
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

// get sample properties
Error::Enum MP4MediaStream::peekNextSampleInfo(MP4VR::SampleInformation& sampleInfo) const
{
    if (mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size)
    {
        return Error::END_OF_FILE;
    }
    sampleInfo = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex];
    return Error::OK;
}

// get sample and incements index by 1, i.e. after this call index points to next sample; may point out of array
Error::Enum MP4MediaStream::getNextSample(uint32_t& sample,
                                          uint64_t& sampleTimeMs,
                                          bool_t& configChanged,
                                          uint32_t& configId,
                                          bool_t& segmentChanged)
{
    if (mSampleIndex.mGlobalSampleIndex >= mTrack->sampleProperties.size)
    {
        return Error::END_OF_FILE;
    }

    if (mFormat->getDecoderConfigInfoId() != 0 &&
        mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].sampleDescriptionIndex !=
            mFormat->getDecoderConfigInfoId())
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
        OMAF_LOG_V("getNextSample: Move to next segment %d",
                   mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId);
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

	// DEBUG: Check that earliest timestamp matches
	// for (auto& pair : mSampleTsPairs)
    // {
    //    if (pair.itemId == sample)
    //    {
    //        sampleTimeMs = pair.timeStamp;
    //        break;
    //    }
    // }

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
    auto timeScaleMsPerTS = 1000.0 / mTrack->timeScale;
    if (mTrack->sampleProperties[0].earliestTimestampTS * timeScaleMsPerTS > timeMs)
    {
        // even the first sample has a higher timestamps than the required time, use it anyway
        sampleId = mTrack->sampleProperties[0].sampleId;
        finalTimeMs = mTrack->sampleProperties[0].earliestTimestampTS * timeScaleMsPerTS;
        return true;
    }

    for (auto& sampleProp : mTrack->sampleProperties)
    {
        uint64_t sampleStartMs = sampleProp.earliestTimestampTS * timeScaleMsPerTS;
        uint64_t sampleEndMs = sampleStartMs + (uint64_t) sampleProp.sampleDurationTS * timeScaleMsPerTS;

        if (sampleStartMs <= timeMs && sampleEndMs > timeMs)
        {
            sampleId = sampleProp.sampleId;
            finalTimeMs = sampleProp.earliestTimestamp;
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
            // the correct segment should have been selected in the initial seeking phase, and here we are just seeking
            // inside a segment
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
    // mSampleIndex.mGlobalSampleIndex points to the next sample to read, and is not the 0-based playhead position /
    // index. hence if we have 0 samples left, mSampleIndex.mGlobalSampleIndex == mTrack->sampleProperties.size
    return (mTrack->sampleProperties.size - mSampleIndex.mGlobalSampleIndex);
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

bool_t MP4MediaStream::isAtLastSegmentBoundary(uint32_t& aSegmentIndex) const
{
    if (mSampleIndex.mCurrentSegment > 0)
    {
        aSegmentIndex = mSampleIndex.mCurrentSegment;
    }
    if (mSampleIndex.mSegmentSampleIndex == mTrack->sampleProperties.size)
    {
        // in the end of the last segment
        OMAF_LOG_V("Track %d isAtSegmentBoundary the last sample of available segments was read %d", mTrackId,
                   mSampleIndex.mCurrentSegment);
        return true;
    }
    else
    {
        return false;
    }
}

bool_t MP4MediaStream::isAtAnySegmentBoundary(uint32_t& aSegmentIndex) const
{
    if (isAtLastSegmentBoundary(aSegmentIndex))
    {
        return true;
    }
    else if (mSampleIndex.mSegmentSampleIndex == 0)
    {
        // This case is relevant with multi-resolution case, where switch can occur in any segment boundary.
        // In other cases we need to use the concatenated segments that are already in the parser.
        // We are at beginning of a new one, so mSampleIndex.mCurrentSegment is one too big
        if (mSampleIndex.mCurrentSegment > 0 && aSegmentIndex > 0)
        {
            aSegmentIndex--;
        }
        OMAF_LOG_V("Track %d isAtSegmentBoundary, done %d", mTrackId, aSegmentIndex);
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
            index = mSampleIndex.mGlobalSampleIndex - 1;
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

    // must not use mSampleIndex.mCurrentSegment since it may be 1 segment behind because mGlobalSampleIndex points to
    // the next sample to be read, and mCurrentSegment points to the last segment read
    uint32_t currentSegment = mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex].segmentId;
    for (uint32_t i = mSampleIndex.mGlobalSampleIndex; i < mTrack->sampleProperties.size; i++)
    {
        if (mTrack->sampleProperties[i].segmentId != currentSegment)
        {
            end = mTrack->sampleProperties[i - 1].earliestTimestamp;
            break;
        }
    }
    if (end == 0)
    {
        // there is no newer segment available, take the timestamp of the last available sample
        end = mTrack->sampleProperties[mTrack->sampleProperties.size - 1].earliestTimestamp;
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
            syncFrameTimeMs = mTrack->sampleProperties[mTrack->sampleProperties.size - 1].earliestTimestamp +
                33;  // guessing frame interval
            return mTrack->sampleProperties.size - mSampleIndex.mGlobalSampleIndex;
        }
    }
    syncFrameTimeMs = mTrack->sampleProperties[mNextRefFrame].earliestTimestamp;
    return mNextRefFrame - mSampleIndex.mGlobalSampleIndex + 1;
}

bool_t MP4MediaStream::isReferenceFrame()
{
    if (mTrack->sampleProperties[mSampleIndex.mGlobalSampleIndex - 1].sampleType == MP4VR::OUTPUT_REFERENCE_FRAME)
    {
        return true;
    }
    else
    {
        return false;
    }
}

bool_t MP4MediaStream::isUserActivateable()
{
    return mUserActivateable;
}

void_t MP4MediaStream::setUserActivateable()
{
    mUserActivateable = true;
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
    packet->setPresentationTimeUs(packet->presentationTimeUs() + mStartOffset);
    mFilledPackets.push(packet);
    OMAF_LOG_D(">> storeFilledPacket for stream %d packet: sampleId: %d, decodingTime: %d, presentationTime: %d, duration: %d, isReconfigured: %d, isReferenceFrame: %d",
               mStreamId, packet->sampleId(), packet->decodingTimeUs(), packet->presentationTimeUs(), packet->durationUs(), packet->isReConfigRequired(), packet->isReferenceFrame());
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
            packet = OMAF_NEW(mAllocator, MP4VRMediaPacket)(getMaxSampleSize());
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
