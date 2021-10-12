
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
#include "Media/NVRMP4MediaStreamManager.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMP4Parser.h"
#include "Media/NVRMediaPacket.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MP4MediaStreamManager);

MP4MediaStreamManager::MP4MediaStreamManager()
    : mAllocator(*MemorySystem::DefaultHeapAllocator())
    , mMediaFormatParser(OMAF_NULL)
    , mAudioStreams()
    , mActiveAudioStreams()
    , mVideoStreams()
    , mActiveVideoStreams()
    , mMetadataStreams()
    , mBackgroundAudioStream(OMAF_NULL)
{
}
MP4MediaStreamManager::~MP4MediaStreamManager()
{
    OMAF_DELETE(mAllocator, mMediaFormatParser);
    resetStreams();
}

Error::Enum MP4MediaStreamManager::openInput(const PathName& mediaUri)
{
    // free old mp4reader
    if (mMediaFormatParser != OMAF_NULL)
    {
        OMAF_DELETE(mAllocator, mMediaFormatParser);
        mMediaFormatParser = OMAF_NULL;
    }

    // create new mp4reader
    mMediaFormatParser = OMAF_NEW(mAllocator, MP4VRParser)(*this, OMAF_NULL);

    Error::Enum result =
        mMediaFormatParser->openInput(mediaUri, mAllAudioStreams, mAllVideoStreams, mAllMetadataStreams);

    if (result == Error::OK)
    {
        selectInitialViewpoint();

        selectStreams();
    }

    return result;
}

void_t MP4MediaStreamManager::resetStreams()
{
    while (!mAllVideoStreams.isEmpty())
    {
        MP4VideoStream* videoStream = mAllVideoStreams[0];
        videoStream->resetPackets();
        mAllVideoStreams.removeAt(0);
        destroyStream(videoStream);
    }
    mAllVideoStreams.clear();
    mVideoStreams.clear();
    mActiveVideoStreams.clear();
    mActiveVideoSources.clear();
    while (!mViewpoints.isEmpty())
    {
        Mp4Viewpoint* vp = mViewpoints[0];
        mViewpoints.removeAt(0);
        OMAF_DELETE_HEAP(vp);
    }
    while (!mAllAudioStreams.isEmpty())
    {
        MP4AudioStream* audioStream = mAllAudioStreams[0];
        audioStream->resetPackets();
        mAllAudioStreams.removeAt(0);
        destroyStream(audioStream);
    }
    mAudioStreams.clear();
    mActiveAudioStreams.clear();
    while (!mAllMetadataStreams.isEmpty())
    {
        MP4MediaStream* metaStream = mAllMetadataStreams[0];
        metaStream->resetPackets();
        mAllMetadataStreams.removeAt(0);
        destroyStream(metaStream);
    }
    mMetadataStreams.clear();
}

void_t MP4MediaStreamManager::getAudioStreams(MP4AudioStreams& aMainAudioStreams,
                                              MP4AudioStreams& aAdditionalAudioStreams)
{
    if (mBackgroundAudioStream != OMAF_NULL)
    {
        aMainAudioStreams.add(mBackgroundAudioStream);
    }
    if (mActiveAudioStreams.getSize() > 0)
    {
        for (MP4AudioStream* audio : mActiveAudioStreams)
        {
            if (audio != mBackgroundAudioStream)
            {
                aAdditionalAudioStreams.add(audio);
            }
        }
    }
}

const MP4AudioStreams& MP4MediaStreamManager::getAudioStreams()
{
    return mActiveAudioStreams;
}

const MP4VideoStreams& MP4MediaStreamManager::getVideoStreams()
{
    return mActiveVideoStreams;
}

const MP4MetadataStreams& MP4MediaStreamManager::getMetadataStreams()
{
    return mMetadataStreams;
}

bool_t MP4MediaStreamManager::switchOverlayVideoStream(streamid_t aVideoStreamId,
                                                       int64_t currentTimeUs,
                                                       bool_t& aPreviousOverlayStopped)
{
    aPreviousOverlayStopped = false;
    for (MP4VideoStream* videoStream : mActiveVideoStreams)
    {
        if (videoStream->isUserActivateable())
        {
            stopOverlayVideoStream(videoStream->getStreamId());
            const CoreProviderSources sources = videoStream->getVideoSources();
            for (CoreProviderSource* source : sources)
            {
                if (source->type == SourceType::OVERLAY)
                {
                    OverlaySource* ovly = (OverlaySource*) source;
                    ovly->active = false;
                    videoStream->setVideoSources(sources);
                    aPreviousOverlayStopped = true;
                    break;
                }
            }

            break;
        }
    }
    for (MP4VideoStream* videoStream : mVideoStreams)
    {
        if (videoStream->getStreamId() == aVideoStreamId && !mActiveVideoStreams.contains(videoStream))
        {
            OMAF_LOG_V("Start overlay video stream %d", aVideoStreamId);
            videoStream->setStartOffset(currentTimeUs);
            mActiveVideoStreams.add(videoStream);
            mActiveVideoSources.add(videoStream->getVideoSources());
            return true;
        }
    }
    return false;
}

bool_t MP4MediaStreamManager::stopOverlayVideoStream(streamid_t aVideoStreamId)
{
    for (MP4VideoStream* videoStream : mVideoStreams)
    {
        if (videoStream->getStreamId() == aVideoStreamId)
        {
            OMAF_LOG_V("Stop overlay video stream %d", aVideoStreamId);
            mActiveVideoStreams.remove(videoStream);
            for (CoreProviderSource* source : videoStream->getVideoSources())
            {
                mActiveVideoSources.remove(source);
            }
            // reset video playhead to the beginning of the stream
            uint32_t sampleId = 0;
            videoStream->findSampleIdByIndex(0, sampleId);
            bool_t segmentChanged = false;
            videoStream->setNextSampleId(sampleId, segmentChanged);

            return true;
        }
    }
    return false;
}

bool_t MP4MediaStreamManager::stopOverlayAudioStream(streamid_t aVideoStreamId, streamid_t& aAudioStreamId)
{
    if (mActiveAudioStreams.isEmpty() ||
        (mActiveAudioStreams.getSize() == 1 && mActiveAudioStreams.at(0) == mBackgroundAudioStream))
    {
        return false;
    }
    for (MP4VideoStream* videoStream : mVideoStreams)
    {
        if (videoStream->getStreamId() == aVideoStreamId)
        {
            MP4RefStreams audio = videoStream->getRefStreams(MP4VR::FourCC("soun"));
            if (!audio.isEmpty())
            {
                MP4AudioStream* currentStream = (MP4AudioStream*) audio.at(0);
                if (mActiveAudioStreams.contains(currentStream))
                {
                    // disabling an overlay that had audio playing
                    // reset audio playhead to the beginning of the stream
                    uint32_t sampleId = 0;
                    currentStream->findSampleIdByIndex(0, sampleId);
                    bool_t segmentChanged = false;
                    OMAF_LOG_V("Reset audio stream %d", currentStream->getStreamId());
                    currentStream->setNextSampleId(sampleId, segmentChanged);
                    currentStream->resetPackets();
                    aAudioStreamId = currentStream->getStreamId();
                    mActiveAudioStreams.remove(currentStream);
                    return true;
                }
                else
                {
                    OMAF_LOG_D("The closed overlay didn't have audio playing");
                }
            }
            break;
        }
    }
    return false;
}


MP4AudioStream* MP4MediaStreamManager::switchAudioStream(streamid_t aVideoStreamId,
                                                         streamid_t& aStoppedAudioStreamId,
                                                         bool_t aPreviousOverlayStopped)
{
    for (MP4VideoStream* videoStream : mVideoStreams)
    {
        if (videoStream->getStreamId() == aVideoStreamId)
        {
            MP4RefStreams audio = videoStream->getRefStreams(MP4VR::FourCC("soun"));
            // enabling a new overlay, check if it has audio
            if (!audio.isEmpty() || aPreviousOverlayStopped)
            {
                // the new one has audio or the previous one was stopped => stop the current overlay audio

                MP4AudioStream* currentStream = OMAF_NULL;
                if (!mActiveAudioStreams.isEmpty())
                {
                    if (mActiveAudioStreams.at(0) == mBackgroundAudioStream)
                    {
                        if (mActiveAudioStreams.getSize() > 1)
                        {
                            currentStream = mActiveAudioStreams.at(1);
                        }
                    }
                    else
                    {
                        currentStream = mActiveAudioStreams
                                            .back();  // This is assuming the latest one is played in AACAudioRenderer
                    }
                    if (currentStream && !currentStream->isUserActivateable())
                    {
                        currentStream = OMAF_NULL;
                    }
                }

                if (currentStream != OMAF_NULL)
                {
                    // reset audio playhead of the current overlay to the beginning of the stream
                    uint32_t sampleId = 0;
                    currentStream->findSampleIdByIndex(0, sampleId);
                    bool_t segmentChanged = false;
                    OMAF_LOG_V("Reset audio stream %d", currentStream->getStreamId());
                    currentStream->setNextSampleId(sampleId, segmentChanged);
                    currentStream->resetPackets();
                    aStoppedAudioStreamId = currentStream->getStreamId();
                    mActiveAudioStreams.remove(currentStream);
                }
                if (!audio.isEmpty())
                {
                    MP4AudioStream* newStream = (MP4AudioStream*) audio.at(0);
                    mActiveAudioStreams.add(newStream);
                    OMAF_LOG_D("Activated audio stream %d", newStream->getStreamId());
                    return newStream;
                }
            }
            break;
        }
    }
    return OMAF_NULL;
}

int32_t MP4MediaStreamManager::getNrStreams()
{
    return static_cast<int32_t>(mAudioStreams.getSize() + mVideoStreams.getSize() + mMetadataStreams.getSize());
}

MP4VRMediaPacket* MP4MediaStreamManager::getNextAudioFrame(MP4MediaStream& stream)
{
    if (!stream.hasFilledPackets())
    {
        bool_t segmentChanged = false;  // the value is not used in local file playback
        mMediaFormatParser->readFrame(stream, segmentChanged, -1);
    }
    return stream.peekNextFilledPacket();
}

MP4VRMediaPacket* MP4MediaStreamManager::getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs)
{
    if (!stream.hasFilledPackets())
    {
        bool_t segmentChanged = false;  // the value is not used in local file playback
        mMediaFormatParser->readFrame(stream, segmentChanged, currentTimeUs);
    }
    return stream.peekNextFilledPacket();
}
MP4VRMediaPacket* MP4MediaStreamManager::getMetadataFrame(MP4MediaStream& aStream, int64_t aCurrentTimeUs)
{
    if (!aStream.hasFilledPackets())
    {
        bool_t segmentChanged = false;  // the value is not used in local file playback
        mMediaFormatParser->readTimedMetadataFrame(aStream, segmentChanged, aCurrentTimeUs);
    }
    return aStream.peekNextFilledPacket();
}

Error::Enum MP4MediaStreamManager::updateMetadata(const MP4MediaStream& metadataStream,
                                                  const MP4VRMediaPacket& metadataFrame)
{
    Error::Enum result = Error::OK;
    // handle overlay metadata packages
    if (StringCompare(metadataStream.getFormat()->getFourCC(), "dyol") == ComparisonResult::EQUAL)
    {
        MP4VR::OverlayConfigSample dyolSample((char*) metadataFrame.buffer(), metadataFrame.dataSize());

        MP4VR::OverlayConfigProperty ovlyConfig;
        MP4VR::OverlayConfigProperty* ovlyConfigPtr = OMAF_NULL;
        if (mMediaFormatParser->getPropertyOverlayConfig(metadataStream.getTrackId(), metadataFrame.sampleId(),
                                                         ovlyConfig) == Error::OK)
        {
            ovlyConfigPtr = &ovlyConfig;
            OMAF_LOG_D("updateMetadata %lld %d", metadataFrame.presentationTimeUs(), metadataFrame.sampleId());
            for (MP4MediaStream* refStream : metadataStream.getRefStreams(MP4VR::FourCC("cdsc")))
            {
                MP4VR::OverlayConfigProperty refTrackConfig;
                MP4VR::OverlayConfigProperty* refTrackConfigPtr = OMAF_NULL;

                if (mMediaFormatParser->getPropertyOverlayConfig(refStream->getTrackId(), 0, refTrackConfig) ==
                    Error::OK)
                {
                    refTrackConfigPtr = &refTrackConfig;
                }
                else
                {
                    OMAF_LOG_W("No overlayproperty");
                    result = Error::OK_SKIPPED;
                    continue;
                }

                if (!mMediaFormatParser->getMetadataParser().updateOverlayMetadata(*refStream, refTrackConfigPtr,
                                                                                   ovlyConfigPtr, dyolSample))
                {
                    result = Error::OK_SKIPPED;
                }
            }
        }
        else
        {
            result = Error::OK_SKIPPED;
        }
    }

    // handle invo
    if (StringCompare(metadataStream.getFormat()->getFourCC(), "invo") == ComparisonResult::EQUAL)
    {
        MP4VR::InitialViewingOrientationSample invo((char*) metadataFrame.buffer(), metadataFrame.dataSize());

        for (MP4MediaStream* refStream : metadataStream.getRefStreams(MP4VR::FourCC("cdsc")))
        {
            if (!mMediaFormatParser->getMetadataParser().updateInitialViewingOrientationMetadata(*refStream, invo))
            {
                result = Error::OK_SKIPPED;
            }
        }
    }
    return result;
}

bool_t MP4MediaStreamManager::getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const
{
    if (mActiveVideoStreams.isEmpty())
    {
        return false;
    }
    aRotation = mActiveVideoStreams.at(0)->getViewpointGCSRotation();
    return true;
}

bool_t MP4MediaStreamManager::getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const
{
    if (mActiveVideoStreams.isEmpty())
    {
        return false;
    }
    aSwitchControls = mActiveVideoStreams.at(0)->getViewpointSwitchControls();
    return true;
}

Error::Enum MP4MediaStreamManager::selectInitialViewpoint()
{
    // check if invp exists. If not, select from video streams with the least value
    for (MP4MediaStream* metadata : mMetadataStreams)
    {
        if (StringCompare(metadata->getFormat()->getFourCC(), "invp") == ComparisonResult::EQUAL)
        {
            MP4VR::InitialViewpointConfigProperty invpProperty;

            invpProperty.idOfInitialViewpoint;
            for (MP4VideoStream* video : mAllVideoStreams)
            {
                if (video->isViewpoint() && video->getViewpointId() == invpProperty.idOfInitialViewpoint)
                {
                    // move to front in the list
                    OMAF_LOG_V("Found");
                }
            }

            return Error::OK;
        }
    }
    uint32_t smallestId = OMAF_UINT32_MAX;
    MP4VideoStreams noVpStreams;
    for (size_t i = 0; i < mAllVideoStreams.getSize(); i++)
    {
        MP4VideoStream* video = mAllVideoStreams.at(i);
        if (video->isViewpoint())
        {
            Mp4Viewpoint* vp = findViewpoint(video->getViewpointId());
            if (vp == OMAF_NULL)
            {
                vp = OMAF_NEW_HEAP(Mp4Viewpoint);
                vp->viewpointLabel = video->getViewpointId(vp->viewpointId);
                vp->viewpointSwitchingList = video->getViewpointSwitchConfig();
                mViewpoints.add(vp);
                if (video->getViewpointId() < smallestId)
                {
                    smallestId = video->getViewpointId();
                }
            }
            vp->videoStreams.add(video);
        }
        else
        {
            noVpStreams.add(video);
        }
    }
    MP4AudioStreams noVPAudioStreams;
    for (size_t i = 0; i < mAllAudioStreams.getSize(); i++)
    {
        MP4AudioStream* audio = mAllAudioStreams.at(i);
        if (audio->isViewpoint())
        {
            Mp4Viewpoint* vp = findViewpoint(audio->getViewpointId());
            OMAF_ASSERT(vp != OMAF_NULL, "No non-video viewpoints supported");
            if (vp)
            {
                vp->audioStreams.add(audio);
            }
        }
        else
        {
            noVPAudioStreams.add(audio);
        }
    }

    MP4MetadataStreams noVPMetadataStreams;
    for (size_t i = 0; i < mAllMetadataStreams.getSize(); i++)
    {
        MP4MediaStream* meta = mAllMetadataStreams.at(i);
        if (meta->isViewpoint())
        {
            Mp4Viewpoint* vp = findViewpoint(meta->getViewpointId());
            OMAF_ASSERT(vp != OMAF_NULL, "No non-video viewpoints supported");
            if (vp)
            {
                vp->metadataStreams.add(meta);
            }
        }
        else
        {
            noVPMetadataStreams.add(meta);
        }
    }

    mCurrentViewpointIndex = OMAF_UINT32_MAX;
    if (smallestId < OMAF_UINT32_MAX)
    {
        mVideoStreams.add(noVpStreams);
        mAudioStreams.add(noVPAudioStreams);
        mMetadataStreams.add(noVPMetadataStreams);

        // Viewpoint-specific streams will be added in selectStreams

        for (size_t i = 0; i < mViewpoints.getSize(); i++)
        {
            if (mViewpoints.at(i)->viewpointId == smallestId)
            {
                mCurrentViewpointIndex = i;
                break;
            }
        }
    }
    else
    {
        // add all
        mVideoStreams.add(mAllVideoStreams);
        mAudioStreams.add(mAllAudioStreams);
        mMetadataStreams.add(mAllMetadataStreams);
    }
    return Error::OK;
}

MP4MediaStreamManager::Mp4Viewpoint* MP4MediaStreamManager::findViewpoint(uint32_t aViewpointId)
{
    Mp4Viewpoint* vp = OMAF_NULL;
    for (size_t j = 0; j < mViewpoints.getSize(); j++)
    {
        if (mViewpoints[j]->viewpointId == aViewpointId)
        {
            vp = mViewpoints[j];
            break;
        }
    }
    return vp;
}

size_t MP4MediaStreamManager::getNrOfViewpoints() const
{
    return mViewpoints.getSize();
}

Error::Enum MP4MediaStreamManager::getViewpointId(size_t aIndex, uint32_t& aId) const
{
    if (mViewpoints.isEmpty() || aIndex >= mViewpoints.getSize())
    {
        return Error::ITEM_NOT_FOUND;
    }
    aId = mViewpoints.at(aIndex)->viewpointId;
    return Error::OK;
}

Error::Enum MP4MediaStreamManager::setViewpoint(uint32_t aDestinationId)
{
    if (mViewpoints.isEmpty())
    {
        return Error::ITEM_NOT_FOUND;
    }
    else if (aDestinationId == mViewpoints.at(mCurrentViewpointIndex)->viewpointId)
    {
        return Error::OK_NO_CHANGE;
    }

    size_t index = 0;
    for (; index < mViewpoints.getSize(); index++)
    {
        if (mViewpoints[index]->viewpointId == aDestinationId)
        {
            break;
        }
    }
    if (index >= mViewpoints.getSize())
    {
        return Error::ITEM_NOT_FOUND;
    }

    OMAF_LOG_D("setViewpoint to id %d label %s", aDestinationId, mViewpoints.at(index)->viewpointLabel);

    // get the expected switch time
    mVideoStreams.at(0)->framesUntilNextSyncFrame(mSwitchTimeMs);

    // read switching info
    uint32_t maxSwitchTime = 0;
    uint32_t minSwitchTime = 0;
    for (size_t i = 0; i < mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList.getSize(); i++)
    {
        if (mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList[i].destinationViewpoint == aDestinationId)
        {
            // destination matches, use this switch-info
            // timeline modifications
            // Note that we expect the offsets to be in ms at this point
            if (mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList[i].absoluteOffsetUsed)
            {
                // absolute
                mSwitchTimeMs = mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList[i].absoluteOffset;
            }
            else
            {
                // relative
                mSwitchTimeMs += mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList[i].relativeOffset;
            }
            if (mViewpoints.at(mCurrentViewpointIndex)->viewpointSwitchingList[i].transitionEffect)
            {
            }
            break;
        }
    }
    mNewViewpointIndex = index;

    return Error::OK;
}

bool_t MP4MediaStreamManager::isViewpointSwitchReadyToComplete(uint32_t aCurrentPlayTimeMs)
{
    if (mVideoStreams.at(0)->isReferenceFrame())
    {
        return true;
    }
    return false;
}

void_t MP4MediaStreamManager::completeViewpointSwitch(bool_t& aBgAudioContinues)
{
    // First, remove video, audio and metadata streams that are specific to the current viewpoint
    for (MP4VideoStream* video : mViewpoints.at(mCurrentViewpointIndex)->videoStreams)
    {
        mVideoStreams.remove(video);
        mActiveVideoStreams.remove(video);
        video->resetPackets();
        for (CoreProviderSource* source : video->getVideoSources())
        {
            mActiveVideoSources.remove(source);
        }
    }
    for (MP4AudioStream* audio : mViewpoints.at(mCurrentViewpointIndex)->audioStreams)
    {
        mAudioStreams.remove(audio);
        mActiveAudioStreams.remove(audio);
        audio->resetPackets();
    }

    MP4MediaStream* oldBgAudio = mBackgroundAudioStream;
    mBackgroundAudioStream = OMAF_NULL;

    for (MP4MediaStream* meta : mViewpoints.at(mCurrentViewpointIndex)->metadataStreams)
    {
        mMetadataStreams.remove(meta);
        meta->resetPackets();
    }

    // Then, switch the viewpoint
    mCurrentViewpointIndex = mNewViewpointIndex;
    mNewViewpointIndex = OMAF_UINT32_MAX;

    // And add streams from the new viewpoint
    selectStreams();
    if (mBackgroundAudioStream == oldBgAudio)
    {
        aBgAudioContinues = true;
    }
    else
    {
        aBgAudioContinues = false;
    }

    uint64_t seekPosUs = mSwitchTimeMs * 1000;
    if (seekToUs(seekPosUs, SeekDirection::NEXT, SeekAccuracy::NEAREST_SYNC_FRAME))
    {
        OMAF_LOG_V("Starting new viewpoint at %lld", seekPosUs);
    }
    else
    {
        OMAF_LOG_W("Failed starting new viewpoint at %lld", seekPosUs);
    }
}

void_t MP4MediaStreamManager::selectStreams()
{
    if (mCurrentViewpointIndex < OMAF_UINT32_MAX)
    {
        mVideoStreams.add(mViewpoints.at(mCurrentViewpointIndex)->videoStreams);
        mAudioStreams.add(mViewpoints.at(mCurrentViewpointIndex)->audioStreams);
        mMetadataStreams.add(mViewpoints.at(mCurrentViewpointIndex)->metadataStreams);
    }

    for (MP4VideoStream* vStream : mVideoStreams)
    {
        if (vStream->getMode() == VideoStreamMode::BACKGROUND)
        {
            mActiveVideoStreams.add(vStream);
            mActiveVideoSources.add(vStream->getVideoSources());
        }
        else if (!vStream->isUserActivateable())
        {
            mActiveVideoStreams.add(vStream);
            mActiveVideoSources.add(vStream->getVideoSources());
        }
    }

    {
        MP4AudioStreams audioStreams(mAudioStreams);
        for (MP4VideoStream* vStream : mVideoStreams)
        {
            MP4RefStreams refStreams = vStream->getRefStreams(MP4VR::FourCC("soun"));
            if (!refStreams.isEmpty())
            {
                // remove the associated overlay audio from the local audioStreams array as in the end it should
                // have the background only, if any
                audioStreams.remove((MP4AudioStream*) refStreams.at(0));
                if (!vStream->isUserActivateable())
                {
                    // we just have to pick one overlay audio stream even if there were many, as we can't mix them
                    mActiveAudioStreams.add((MP4AudioStream*) refStreams.at(0));
                    OMAF_LOG_D("Activated audio stream %d", (MP4AudioStream*) refStreams.at(0)->getStreamId());
                }
                else
                {
                    refStreams.at(0)->setUserActivateable();
                }
            }
        }
        if (!audioStreams.isEmpty())
        {
            mBackgroundAudioStream = audioStreams.at(0);
        }
    }
    // If there is an always-active overlay with audio, it is on instead of background audio.
    // And when it finishes, we switch to the background audio.

    if (mBackgroundAudioStream != OMAF_NULL)
    {
        mActiveAudioStreams.add(mBackgroundAudioStream);
    }
}

size_t MP4MediaStreamManager::getCurrentViewpointIndex() const
{
    return mCurrentViewpointIndex;
}

Error::Enum MP4MediaStreamManager::readVideoFrames(int64_t currentTimeUs)
{
    // Do nothing here on file playback
    return Error::OK;
}
Error::Enum MP4MediaStreamManager::readAudioFrames()
{
    // Do nothing here on file playback
    return Error::OK;
}
Error::Enum MP4MediaStreamManager::readMetadata(int64_t aCurrentTimeUs)
{
    // Do nothing here on file playback
    return Error::OK;
}

const CoreProviderSourceTypes& MP4MediaStreamManager::getVideoSourceTypes()
{
    return mMediaFormatParser->getVideoSourceTypes();
}

const CoreProviderSources& MP4MediaStreamManager::getVideoSources()
{
    return mActiveVideoSources;
}

const CoreProviderSources& MP4MediaStreamManager::getAllVideoSources()
{
    if (mMediaFormatParser == OMAF_NULL)
    {
        OMAF_LOG_V("getting sources, but parser is not ready!!");
        return mActiveVideoSources;
    }
    return mMediaFormatParser->getVideoSources();
}

bool_t MP4MediaStreamManager::isBuffering()
{
    return false;
}

bool_t MP4MediaStreamManager::isEOS() const
{
    return mMediaFormatParser->isEOS(mActiveAudioStreams, mVideoStreams);
}
bool_t MP4MediaStreamManager::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    return false;
}

bool_t MP4MediaStreamManager::seekToUs(uint64_t& seekPosUs, SeekDirection::Enum direction, SeekAccuracy::Enum accuracy)
{
    return mMediaFormatParser->seekToUs(seekPosUs, mAudioStreams, mVideoStreams, direction, accuracy);
}

bool_t MP4MediaStreamManager::seekToFrame(int32_t seekFrameNr, uint64_t& seekPosUs)
{
    return mMediaFormatParser->seekToFrame(seekFrameNr, seekPosUs, mAudioStreams, mVideoStreams);
}

uint64_t MP4MediaStreamManager::getNextVideoTimestampUs()
{
    return mMediaFormatParser->getNextVideoTimestampUs(*mVideoStreams[0]);
}

// this is not a full seek as it is done for video only. It is used in alternate tracks case to find the time when
// the next track is ready to switch and pre-seek the next track for the switch
int64_t MP4MediaStreamManager::findAndSeekSyncFrameMs(uint64_t currentTimeMs,
                                                      MP4MediaStream* videoStream,
                                                      SeekDirection::Enum direction)
{
    // first find the sample that corresponds to the current time instant
    uint32_t sampleId = 0;
    uint64_t finalTimeMs = 0;
    videoStream->findSampleIdByTime(currentTimeMs, finalTimeMs, sampleId);
    bool_t segmentChanged;  // not used in this context
    // then seek to the next/prev sync frame in the stream
    return mMediaFormatParser->seekToSyncFrame(sampleId, videoStream, direction, segmentChanged);
}

MediaInformation MP4MediaStreamManager::getMediaInformation()
{
    if (mMediaFormatParser != OMAF_NULL)
    {
        return mMediaFormatParser->getMediaInformation();
    }
    else
    {
        return MediaInformation();
    }
}

MP4MediaStream* MP4MediaStreamManager::createStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(MP4MediaStream)(aFormat);
}
MP4VideoStream* MP4MediaStreamManager::createVideoStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(MP4VideoStream)(aFormat);
}
MP4AudioStream* MP4MediaStreamManager::createAudioStream(MediaFormat* aFormat) const
{
    return OMAF_NEW_HEAP(MP4AudioStream)(aFormat);
}

void_t MP4MediaStreamManager::destroyStream(MP4MediaStream* aStream) const
{
    OMAF_DELETE_HEAP(aStream);
}

OMAF_NS_END
