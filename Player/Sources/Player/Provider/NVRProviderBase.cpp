
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
#include "Provider/NVRProviderBase.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(ProviderBase)


ProviderBase::ProviderBase()
    : mAudioInputBuffer(OMAF_NULL)
    , mVideoDecoder(VideoDecoderManager::getInstance())
    , mUserActionSync(false, false)
    , mPendingUserAction(PendingUserAction::INVALID)
    , mPendingOverlayControl()
    , mStreamInitialPositionMS(0)
    , mSeekTargetUs(OMAF_UINT64_MAX)
    , mSeekTargetFrame(OMAF_UINT64_MAX)
    , mParserThreadControlEvent(false, false)
    , mVideoDecoderFull(false)
    , mAudioFull(false)
    , mFirstVideoPacketRead(false)
    , mFirstFramePTSUs(0)
    , mFirstDisplayFramePTSUs(OMAF_UINT64_MAX)
    , mElapsedTimeUs(0)
    , mState(VideoProviderState::IDLE)
    , mBufferingStartTimeMS(0)
    , mSeekSyncronizeEvent(false, false)
    , mSeekResult(Error::OK)
    , mSelectedSourceGroup(0)
    , mOverlayAudioPlayPending(false)
    , mSignalViewport(false)
    , mSwitchViewpoint(false)
    , mViewpointCoordSysRotation()
    , mCoordSysRotationUpdated(false)
{
}

ProviderBase::~ProviderBase()
{
    OMAF_ASSERT(mAudioInputBuffer == OMAF_NULL, "Destroy instance not called");
    VideoDecoderManager::destroyInstance();
    mVideoDecoder = OMAF_NULL;
}

void_t ProviderBase::destroyInstance()
{
    if (mAudioInputBuffer != OMAF_NULL)
    {
        mAudioInputBuffer->flush();
        mAudioInputBuffer->removeObserver(this);
    }

    for (Streams::ConstIterator it = mPreviousStreams.begin(); it != mPreviousStreams.end(); ++it)
    {
        mVideoDecoder->deactivateStream(*it);
    }
    mPreviousStreams.clear();

    mAudioInputBuffer = OMAF_NULL;
}

void_t ProviderBase::setState(VideoProviderState::Enum state)
{
    if (state == mState)
    {
        return;
    }
    if (mState == VideoProviderState::BUFFERING && state == VideoProviderState::PLAYING)
    {
        if (mBufferingStartTimeMS == 0)
        {
            OMAF_LOG_ABR("BUFFERING(EndTime/StartTime/DurationMS) %d 0 0", Time::getClockTimeMs());
        }
        else
        {
            uint32_t now = Time::getClockTimeMs();
            OMAF_LOG_ABR("BUFFERING(EndTime/StartTime/DurationMS) %d %d %d", now, mBufferingStartTimeMS,
                         now - mBufferingStartTimeMS);
            mBufferingStartTimeMS = 0;
        }
    }
    else if (mState == VideoProviderState::PLAYING && state == VideoProviderState::BUFFERING)
    {
        mBufferingStartTimeMS = Time::getClockTimeMs();
    }

    mState = state;
    OMAF_LOG_D("State set to: %s", VideoProviderState::toString(mState));
}

VideoProviderState::Enum ProviderBase::getState() const
{
    return mState;
}

Error::Enum ProviderBase::loadSource(const PathName& source, uint64_t initialPositionMS)
{
    OMAF_ASSERT(!mParserThread.isRunning(), "Thread already started");

    if (getState() != VideoProviderState::IDLE || mAudioInputBuffer == OMAF_NULL)
    {
        return Error::INVALID_STATE;
    }

    mSourceURI = source;

    mStreamInitialPositionMS = initialPositionMS;

    setState(VideoProviderState::LOADING);

    Thread::EntryFunction function;
    function.bind<ProviderBase, &ProviderBase::threadEntry>(this);

    mParserThread.setPriority(Thread::Priority::INHERIT);
    mParserThread.setName("ProviderBase::ParserThread");
    mParserThread.start(function);

    return Error::OK;
}

Error::Enum ProviderBase::start()
{
    if (mState == VideoProviderState::PLAYING || mState == VideoProviderState::SWITCHING_VIEWPOINT)
    {
        return Error::OK_NO_CHANGE;
    }
    else if (mState == VideoProviderState::LOADING || mState == VideoProviderState::LOADED ||
             mState == VideoProviderState::PAUSED || mState == VideoProviderState::BUFFERING ||
             mState == VideoProviderState::END_OF_FILE)
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::PLAY;
        mPendingUserActionMutex.unlock();

        if (!mUserActionSync.wait(ASYNC_OPERATION_TIMEOUT))
        {
            OMAF_ASSERT(false, "Start timed out");
        }

        return Error::OK;
    }
    // All other states are not allowed
    return Error::INVALID_STATE;
}

Error::Enum ProviderBase::pause()
{
    if (mState == VideoProviderState::PLAYING || mState == VideoProviderState::BUFFERING)
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::PAUSE;
        mPendingUserActionMutex.unlock();

        if (!mUserActionSync.wait(ASYNC_OPERATION_TIMEOUT))
        {
            OMAF_ASSERT(false, "Pause timed out");
        }

        return Error::OK;
    }
    // Note, not allowing pausing while switching viewpoint
    return Error::INVALID_STATE;
}

const OverlayState ProviderBase::overlayState(uint32_t ovlyId) const
{
    // overlay source must be part of selected sources
    auto ovly = getOverlaySource(ovlyId);
    OverlayState state{};
    state.enabled = ovly->active;
    state.distance = ovly->distance;
    return state;
}

Error::Enum ProviderBase::controlOverlay(const OverlayControl cmd)
{
    mPendingUserActionMutex.lock();
    mPendingUserAction = PendingUserAction::CONTROL_OVERLAY;
    mPendingOverlayControl = cmd;
    mPendingUserActionMutex.unlock();
    return Error::OK;
}

Error::Enum ProviderBase::handleOverlayControl(const OverlayControl cmd)
{
    OverlaySource* ovly = getOverlaySource(cmd.id);
    OMAF_LOG_V("handleOverlayControl with overlay %d stream %d", cmd.id, ovly->mediaSource->streamIndex);

    switch (cmd.command)
    {
    case OverlayUserInteraction::SWITCH_ON_OFF:
    {
        if (cmd.enable)
        {
            ovly->active = true;
        }
        else
        {
            ovly->active = false;
        }
        controlOverlayStream(ovly->mediaSource->streamIndex, cmd.enable);
    }
    break;
    case OverlayUserInteraction::DEPTH:
    {
        // adjust distance...
        ovly->distance = ovly->distance * cmd.distance;
        // controlOverlayStream with ovly->active, but now when overlay duplications are prevented in lower layers, it
        // really does not cause any impact anywhere.
    }
    break;
    case OverlayUserInteraction::CHANGE_VIEWPOINT:
    {
        mSwitchViewpoint = true;
        mNextViewpointId = cmd.destinationViewpointId;
    }
    break;
    default:
        break;
    }

    Spinlock::ScopeLock lock(mStreamLock);

    return Error::OK;
}

Error::Enum ProviderBase::stop()
{
    if (mState == VideoProviderState::STOPPED || mState == VideoProviderState::INVALID)
    {
        return Error::INVALID_STATE;
    }
    else
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::STOP;
        mPendingUserActionMutex.unlock();

        if (mParserThread.isRunning())
        {
            if (!mUserActionSync.wait(500))
            {
                OMAF_ASSERT(false, "Stop timed out");
            }
        }

        OMAF_LOG_D("Stop done");

        return Error::OK;
    }
}

Error::Enum ProviderBase::next()
{
    // this is a very basic viewpoint switching control, it ignores possible user controls and time limits
    size_t nrOfViewpoints = mMediaStreamManager->getNrOfViewpoints();
    if (nrOfViewpoints > 1)
    {
        size_t current = mMediaStreamManager->getCurrentViewpointIndex();
        size_t next = 0;
        if (current < nrOfViewpoints - 1)
        {
            next = current + 1;
        }
        else
        {
            next = 0;
        }
        if (mMediaStreamManager->getViewpointId(next, mNextViewpointId) != Error::OK)
        {
            return Error::OK_SKIPPED;
        }
        mSwitchViewpoint = true;
        return Error::OK;
    }
    return Error::OK_SKIPPED;
}

Error::Enum ProviderBase::nextSourceGroup()
{
    // in prepare sources... probably needs to be changed when real entity group
    // based selection is implemented
    mSelectedSourceGroup++;
    return Error::OK;
}

Error::Enum ProviderBase::prevSourceGroup()
{
    // in prepare sources... probably needs to be changed when real entity group
    // based selection is implemented
    mSelectedSourceGroup--;
    return Error::OK;
}

// called in client thread
Error::Enum ProviderBase::seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy)
{
    if (getState() != VideoProviderState::LOADED && getState() != VideoProviderState::PLAYING &&
        getState() != VideoProviderState::BUFFERING && getState() != VideoProviderState::PAUSED &&
        getState() != VideoProviderState::END_OF_FILE)
    {
        return Error::INVALID_STATE;
    }

    if (!isSeekable())
    {
        return Error::NOT_SUPPORTED;
    }

    mSeekAccuracy = seekAccuracy;
    uint64_t seekTargetInUs = (aSeekMs * 1000);  // mFirstFramePTSUs;
    // is there case where mFirstFramePTSUs value actually would fix something? This breaks Android seek after
    // suspend/stop with GearVR HMD removal. since first packet PTS is not 0, but basically first downloaded media
    // segment PTS from middle of static stream playback. OMAF_LOG_D("seekToMs: aSeekMs: %lld, mFirstFramePTSUs: %lld,
    // seekTargetInUs: %lld ", aSeekMs, mFirstFramePTSUs, seekTargetInUs);

    // if (seekTargetInUs > (durationMs() * 1000) && durationMs() != 0)
    //{
    //    return Error::OPERATION_FAILED;
    //}

    mSeekTargetUs = seekTargetInUs;
    if (!mSeekSyncronizeEvent.wait(ASYNC_OPERATION_TIMEOUT))
    {
        OMAF_ASSERT(false, "SeekToMS timed out");
    }
    aSeekMs = mSeekResultMs;
    return mSeekResult;
}

// called in client thread
Error::Enum ProviderBase::seekToFrame(uint64_t frame)
{
    if (!isSeekableByFrame())
    {
        return Error::NOT_SUPPORTED;
    }
    if (frame > mMediaInformation.numberOfFrames)
    {
        return Error::OPERATION_FAILED;
    }

    mSeekTargetFrame = static_cast<int32_t>(frame);

    if (!mSeekSyncronizeEvent.wait(ASYNC_OPERATION_TIMEOUT))
    {
        OMAF_ASSERT(false, "SeekToMS timed out");
    }

    return mSeekResult;
}

void_t ProviderBase::startPlayback(bool_t waitForAudio)
{
    mSynchronizer.setSyncRunning(waitForAudio);

    if (mAudioInputBuffer != OMAF_NULL && mAudioInputBuffer->isInitialized())
    {
        mAudioInputBuffer->startPlayback();
        mAudioFull = false;
        mAudioUsed = true;
    }
    else
    {
        mAudioUsed = false;
    }
}

void_t ProviderBase::stopPlayback()
{
    if (mAudioInputBuffer != OMAF_NULL)
    {
        mAudioInputBuffer->stopPlayback();
    }
    mSynchronizer.stopSyncRunning();
}

uint64_t ProviderBase::elapsedTimeMs() const
{
    if (mElapsedTimeUs < mSynchronizer.getFrameOffset())
    {
        return mElapsedTimeUs / 1000;
    }
    else
    {
        return (mElapsedTimeUs - mSynchronizer.getFrameOffset()) / 1000;
    }
}

PacketProcessingResult::Enum ProviderBase::processMP4Video()
{
    bool_t endOfFile = false;
    uint64_t currentPTS = mSynchronizer.getElapsedTimeUs();
    const MP4VideoStreams& streams = mMediaStreamManager->getVideoStreams();

    if (streams.isEmpty())
    {
        return PacketProcessingResult::OK;  //??
    }

    Error::Enum readResult = mMediaStreamManager->readVideoFrames(currentPTS);
    if (readResult != Error::OK)
    {
        return PacketProcessingResult::ERROR;
    }

    Streams baseStreamIds;
    Streams enhStreamIds;
    Streams skippedStreamIds;
    bool_t allVideoDecodersFull = true;
    bool_t buffering = false;

    Streams currentStreams;
    for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        currentStreams.add((*it)->getStreamId());
    }

    Streams usedStreams;

    // First check all the streams that are no longer in use and deactivate them
    for (Streams::ConstIterator it = mPreviousStreams.begin(); it != mPreviousStreams.end(); ++it)
    {
        streamid_t streamId = *it;
        if (!currentStreams.contains(streamId))
        {
            Spinlock::ScopeLock lock(mStreamLock);
            OMAF_LOG_D("deactivate %d", streamId);
            mVideoDecoder->deactivateStream(streamId);
        }
    }

    for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        MP4VideoStream* videoStream = *it;

        streamid_t streamId = videoStream->getStreamId();
        bool_t isActivatedNow = false;

        if (videoStream->getMode() == VideoStreamMode::BACKGROUND && mVideoDecoder->isEOS(streamId))
        {
            usedStreams.add(streamId);
            endOfFile = true;
        }
        else
        {
            MP4VRMediaPacket* videoPacket = mMediaStreamManager->getNextVideoFrame(*videoStream, currentPTS);
            if (videoPacket == OMAF_NULL)
            {
                if (!mPreviousStreams.contains(streamId))
                {
                    OMAF_LOG_D("activate %d for EOS", streamId);
                    Error::Enum result = mVideoDecoder->activateStream(streamId, currentPTS);
                    if (result != Error::OK)
                    {
                        return PacketProcessingResult::ERROR;
                    }
                    isActivatedNow = true;
                }
                usedStreams.add(streamId);

                if (videoStream->getMode() == VideoStreamMode::BACKGROUND)
                {
                    if (mMediaStreamManager->isEOS() || mMediaStreamManager->isReadyToSignalEoS(*videoStream))
                    {
                        // notify decoder about end of stream
                        if (!mVideoDecoder->isInputEOS(streamId))
                        {
                            OMAF_LOG_V("Set EOS after empty packet from base layer stream %d",
                                       videoStream->getStreamId());
                            mVideoDecoder->setInputEOS(streamId);
                        }
                    }
                    else
                    {
                        OMAF_LOG_V("Buffering since empty packet");
                        buffering = true;
                    }
                }
            }
            else
            {
                // If the stream is new, it needs to be activated (better to do it now when there is some data for it)
                if (!mPreviousStreams.contains(streamId))
                {
                    OMAF_LOG_D("activate %d", streamId);
                    Error::Enum result;
                    if (mSynchronizer.isSyncRunning())
                    {
                        result = mVideoDecoder->activateStream(streamId, currentPTS);
                    }
                    else
                    {
                        result = mVideoDecoder->activateStream(streamId, 0);
                    }
                    if (result != Error::OK)
                    {
                        return PacketProcessingResult::ERROR;
                    }
                    isActivatedNow = true;

                    mCoordSysRotationUpdated =
                        mMediaStreamManager->getViewpointGlobalCoordSysRotation(mViewpointCoordSysRotation);
                }
                usedStreams.add(streamId);

                // When reading the first video frame, set the synchronizer time to the PTS of the first video fram
                // This enables the drawing the
                if (!mFirstVideoPacketRead)
                {
                    mFirstVideoPacketRead = true;
                    mFirstFramePTSUs = videoPacket->presentationTimeUs();
                    OMAF_LOG_D("Found first frame at %lld", videoPacket->presentationTimeUs());
                }
                DecodeResult::Enum result = mVideoDecoder->decodeMediaPacket(streamId, videoPacket);
                if (result == DecodeResult::PACKET_ACCEPTED)
                {
                    // The packet was copied to the decoder, remove 1st packet from filled queue
                    // OMAF_LOG_D("decodeMediaPacket %lld for stream %d", videoPacket->presentationTimeUs(), streamId);
                    videoStream->popFirstFilledPacket();
                    videoStream->returnEmptyPacket(videoPacket);
                    allVideoDecodersFull = false;
                    mSynchronizer.setVideoSyncPoint(videoPacket->presentationTimeUs());
                }
                else if (result == DecodeResult::DECODER_FULL || result == DecodeResult::NOT_READY)
                {
                    // Decoder can't handle more data. try again later.
                }
                else if (result == DecodeResult::DECODER_ERROR)
                {
                    return PacketProcessingResult::ERROR;
                }
            }
        }

        if (isActivatedNow || mPreviousStreams.contains(streamId))
        {
            // old streams mapped to lists for preloadTextures; not yet activated streams must not get there (they don't
            // have a decoder), but old streams without new data must get in EOS case
            if (videoStream->getMode() == VideoStreamMode::BACKGROUND)
            {
                baseStreamIds.add(streamId);
            }
            else if (videoStream->getMode() == VideoStreamMode::OVERLAY)
            {
                enhStreamIds.add(streamId);
            }
            else
            {
                skippedStreamIds.add(streamId);
            }
        }
    }

    if (allVideoDecodersFull)
    {
        mVideoDecoderFull = true;
    }

#ifdef _DEBUG
    // playhead and no pictures are shown - at least on slow machines
    buffering |= mVideoDecoder->isBuffering(baseStreamIds);
#endif
    Error::Enum preloadStatus =
        mVideoDecoder->preloadTexturesForPTS(baseStreamIds, enhStreamIds, skippedStreamIds, currentPTS);
    if (preloadStatus != Error::OK && preloadStatus != Error::OK_SKIPPED && preloadStatus != Error::OK_NO_CHANGE &&
        preloadStatus != Error::ITEM_NOT_FOUND)
    {
        return PacketProcessingResult::ERROR;
    }
    mPreviousStreams.clear();
    mPreviousStreams.add(usedStreams);
    if (buffering)
    {
        return PacketProcessingResult::BUFFERING;
    }
    else if (endOfFile)
    {
        return PacketProcessingResult::END_OF_FILE;
    }
    else
    {
        return PacketProcessingResult::OK;
    }
}

PacketProcessingResult::Enum ProviderBase::processMP4Audio(AudioInputBuffer* audioInputBuffer)
{
    OMAF_ASSERT_NOT_NULL(mAudioInputBuffer);

    const MP4AudioStreams& astreams = mMediaStreamManager->getAudioStreams();

    if (astreams.isEmpty())
    {
        return PacketProcessingResult::OK;
    }

    int32_t limit = AUDIO_REQUEST_THRESHOLD;
    bool_t buffering = false;

    mMediaStreamManager->readAudioFrames();
    for (MP4AudioStream* audio : astreams)
    {
        for (int32_t count = 0; count < limit; count++)
        {
            MP4VRMediaPacket* audioPacket = mMediaStreamManager->getNextAudioFrame(*audio);

            if (audioPacket == OMAF_NULL)
            {
                if (mMediaStreamManager->isEOS())
                {
                    // No more audio packets to read so mark as full
                    mAudioFull = true;
                    buffering = true;
                }
                break;
            }
            // OMAF_LOG_D("write audio stream %d pts %lld, elapsed %lld", audio->getStreamId(),
            // audioPacket->presentationTimeUs(), mSynchronizer.getElapsedTimeUs());
            Error::Enum success = audioInputBuffer->write(audio->getStreamId(), audioPacket);

            if (success == Error::OK)
            {
                mSynchronizer.setAudioSyncPoint(audioPacket->presentationTimeUs());

                // OMAF_LOG_D("wrote audio");
                audio->popFirstFilledPacket();
                audio->returnEmptyPacket(audioPacket);
            }
            else if (success == Error::BUFFER_OVERFLOW)
            {
                // OMAF_LOG_V("Audio full for stream %d", audio->getStreamId());
                mAudioFull = true;
                break;
            }
            else
            {
                OMAF_LOG_W("Failed writing audio, error %d", success);
                break;
            }
        }
    }
    if (buffering)
    {
        return PacketProcessingResult::BUFFERING;
    }
    else
    {
        return PacketProcessingResult::OK;
    }
}

PacketProcessingResult::Enum ProviderBase::processOverlayMetadataStream()
{
    uint64_t currentPTS = mSynchronizer.getElapsedTimeUs();
    mMediaStreamManager->readMetadata(currentPTS);

    for (MP4MediaStream* stream : mMediaStreamManager->getMetadataStreams())
    {
        MP4VRMediaPacket* metadataSamplePacket = mMediaStreamManager->getMetadataFrame(*stream, currentPTS);
        if (metadataSamplePacket != OMAF_NULL)
        {
            OMAF_LOG_D("Got metadata sample from streamId: %d sampleId: %d", stream->getStreamId(), metadataSamplePacket->sampleId());
            if (mMediaStreamManager->updateMetadata(*stream, *metadataSamplePacket) == Error::OK)
            {
                stream->popFirstFilledPacket();
                stream->returnEmptyPacket(metadataSamplePacket);
            }
        }
    }

    return PacketProcessingResult::OK;
}

bool_t ProviderBase::setInitialViewport(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical)
{
    // is relevant for viewport dependent operation only, skipped
    return true;
}

const CoreProviderSources& ProviderBase::getPastBackgroundSources(uint64_t aPts)
{
    return mMediaStreamManager->getPastBackgroundVideoSourcesAt(aPts);
}

Streams ProviderBase::extractStreamsFromSources(const CoreProviderSources& sources)
{
    Streams streams;
    for (CoreProviderSources::ConstIterator it = sources.begin(); it != sources.end(); ++it)
    {
        if ((*it)->category == SourceCategory::VIDEO)
        {
            VideoFrameSource* videoSource = (VideoFrameSource*) (*it);
            if (!streams.contains(videoSource->streamIndex))
            {
                streams.add(videoSource->streamIndex);
            }
        }
    }
    return streams;
}

const CoreProviderSources& ProviderBase::getSources() const
{
    return mPreparedSources;
}

const CoreProviderSources& ProviderBase::getAllSources() const
{
    // dummy base class
    return mPreparedSources;
}

bool_t ProviderBase::getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const
{
    Spinlock::ScopeLock lock(mStreamLock);

    if (mMediaStreamManager)
    {
        if (mMediaStreamManager->getViewpointUserControls(aSwitchControls))
        {
            // Filter the list of user controls based on the set activation time
            uint32_t nowMs = (uint32_t)(mSynchronizer.getElapsedTimeUs() / 1000);
            for (size_t i = 0; i < aSwitchControls.getSize(); i++)
            {
                ViewpointSwitchControl& switchCtrl = aSwitchControls[i];
                if ((switchCtrl.switchActivationStart && nowMs < switchCtrl.switchActivationStartTime) ||
                    (switchCtrl.switchActivationEnd && nowMs > switchCtrl.switchActivationEndTime))
                {
                    aSwitchControls.removeAt(i);
                }
            }
            return true;
        }
    }
    return false;
}

Error::Enum ProviderBase::prepareSources(HeadTransform currentHeadtransform,
                                         float32_t fovHorizontal,
                                         float32_t fovVertical)
{
    VideoProviderState::Enum state = getState();

    if (state != VideoProviderState::PAUSED && state != VideoProviderState::PLAYING &&
        state != VideoProviderState::END_OF_FILE && state != VideoProviderState::BUFFERING &&
        state != VideoProviderState::LOADED && state != VideoProviderState::SWITCHING_VIEWPOINT)
    {
#ifndef OMAF_VIEWPORT_DONT_FORCE_ORIGIN_TO_0  // OMAF player should force origin to (0,0,0), but in some use cases that
                                              // may not be ideal
        if (!mViewingOrientationOffset.valid)
        {
            // make sure we start at yaw = 0, no matter where the user is watching. If there's signaled initial
            // viewing orientation, it will be applied when the state is such that this step is skipped, and it will
            // override this one
            Quaternion sensorOffset = currentHeadtransform.orientation;

            // We take the current sensor as the new origin for yaw/azimuth. Hence the current yaw should be
            // inverted.
            float32_t yaw, pitch, roll;
            eulerAngles(sensorOffset, yaw, pitch, roll, EulerAxisOrder::YXZ);
            sensorOffset = makeQuaternion(0.f, -yaw, 0.f, EulerAxisOrder::XYZ);

            mViewingOrientationOffset.orientation = sensorOffset;

            mViewingOrientationOffset.valid = true;

            setupInitialViewingOrientation(currentHeadtransform, 0);
        }
#endif
        if (!setInitialViewport(currentHeadtransform, fovHorizontal, fovVertical))
        {
            mViewingOrientationOffset.valid = false;  // triggers to retry
        }
        return Error::OK_SKIPPED;
    }
    else if (mSignalViewport)
    {
        OMAF_LOG_V("Signaling initial viewport");
        setInitialViewport(currentHeadtransform, fovHorizontal, fovVertical);
    }

    if (!mStreamLock.tryLock())
    {
        return Error::OK_SKIPPED;
    }

    CoreProviderSources allSelectedSources;
    CoreProviderSources requiredSources;
    CoreProviderSources requiredVideoSources;
    CoreProviderSources additionalSources;

    uint64_t elapsedTimeUs = mSynchronizer.getElapsedTimeUs();
    mElapsedTimeUs = elapsedTimeUs;

    // initial viewing orientation should be added to currentHeadTransform before selecting sources
    setupInitialViewingOrientation(currentHeadtransform, elapsedTimeUs);

    uint64_t sourceValidFromPts =
        selectSources(currentHeadtransform, fovHorizontal, fovVertical, requiredSources, additionalSources);
    int32_t maxSourceGroupIndex = -1;
    for (CoreProviderSources::ConstIterator it = requiredSources.begin(); it != requiredSources.end(); ++it)
    {
        maxSourceGroupIndex = maxSourceGroupIndex > (*it)->sourceGroup ? maxSourceGroupIndex : (*it)->sourceGroup;
    }

    if (mSelectedSourceGroup < 0)
    {
        mSelectedSourceGroup = maxSourceGroupIndex;
    }
    else if (maxSourceGroupIndex >= 0)
    {
        mSelectedSourceGroup = mSelectedSourceGroup % (maxSourceGroupIndex + 1);
    }

    for (CoreProviderSources::ConstIterator it = requiredSources.begin(); it != requiredSources.end(); ++it)
    {
        CoreProviderSource* source = *it;
        if (mSelectedSourceGroup == source->sourceGroup || source->sourceGroup < 0)
        {
            requiredVideoSources.add(source);
        }
    }

    allSelectedSources.add(requiredVideoSources);
    allSelectedSources.add(additionalSources);

    Streams requiredStreams = extractStreamsFromSources(requiredVideoSources);
    Streams additionalStreams = extractStreamsFromSources(additionalSources);


    // Draw first frame hack
    if (mFirstDisplayFramePTSUs != OMAF_UINT64_MAX && elapsedTimeUs < mFirstDisplayFramePTSUs)
    {
        elapsedTimeUs = mFirstDisplayFramePTSUs;
    }

	// OMAF_LOG_D("============ Getting textures for time (looks like both eyes might have different input frames): %d", elapsedTimeUs);
    TextureLoadOutput textureLoadOutput;
    Error::Enum uploadResult =
        mVideoDecoder->uploadTexturesForPTS(requiredStreams, additionalStreams, elapsedTimeUs, textureLoadOutput);

    if (uploadResult == Error::OK)
    {
        uint64_t pts = OMAF_UINT64_MAX;
        if (!requiredStreams.isEmpty())
        {
            pts = mVideoDecoder->getCurrentVideoFrame(requiredStreams.at(0)).pts;
        }
        if (pts < sourceValidFromPts)
        {
            OMAF_LOG_D("Source pts %llu is for a newer frame than %llu", sourceValidFromPts, pts);
            // Take older sources for the background
            allSelectedSources.clear();
            allSelectedSources.add(getPastBackgroundSources(pts));
            allSelectedSources.add(additionalSources);
        }
        else
        {
            //            OMAF_LOG_D("Creating prepared sources for pts %lld", sourceValidFromPts);
        }
        // update core provider sources, with latest data from stream

        mFirstDisplayFramePTSUs = OMAF_UINT64_MAX;
        mPreparedSources.clear();

        CoreProviderSources backgroundSources;
        CoreProviderSources sphereRelativeSources;
        CoreProviderSources viewportRelativeSources;

        for (CoreProviderSources::ConstIterator it = allSelectedSources.begin(); it != allSelectedSources.end(); ++it)
        {
            CoreProviderSource* coreProviderSource = *it;

            if (coreProviderSource->category == SourceCategory::VIDEO)
            {
                // for VideoFrameSource, if new texture was uploaded for frames source referring to certain
                // stream get video frame from decoder and add updated VideoFrameSource to prepared sources list
                VideoFrameSource* videoFrameSource = (VideoFrameSource*) coreProviderSource;
                streamid_t streamId = videoFrameSource->streamIndex;

                if (textureLoadOutput.updatedStreams.contains(streamId))
                {
                    videoFrameSource->videoFrame = mVideoDecoder->getCurrentVideoFrame(streamId);
                    // Video resolution can change so update it
                    mMediaInformation.width = videoFrameSource->widthInPixels;
                    mMediaInformation.height = videoFrameSource->heightInPixels;
                    if (mViewingOrientationOffset.valid)
                    {
                        // initial viewing orientation has to be stored to the source to pass it to the
                        // renderer. And we need to calculate it in provider due to viewport dependent operation

                        videoFrameSource->forcedOrientation.orientation = currentHeadtransform.orientation;
                        videoFrameSource->forcedOrientation.valid = true;
                    }

                    switch (videoFrameSource->mediaPlacement)
                    {
                    case SourceMediaPlacement::BACKGROUND:
                        backgroundSources.add(videoFrameSource);
                        break;
                    case SourceMediaPlacement::SPHERE_RELATIVE_OVERLAY:
                        sphereRelativeSources.add(videoFrameSource);
                        break;
                    case SourceMediaPlacement::VIEWPORT_RELATIVE_OVERLAY:
                        viewportRelativeSources.add(videoFrameSource);
                        break;
                    }
                }
            }
        }
        mPreparedSources.add(backgroundSources);
        mPreparedSources.add(sphereRelativeSources);
        mPreparedSources.add(viewportRelativeSources);

#if OMAF_ABR_LOGGING
        // >> TEST code for analysing VD performance
        OMAF_LOG_ABR("Rendered frame %lld", pts);
        // << TEST code for analysing VD performance
#endif
        // OMAF_LOG_V("Rendered frame %lld source %lld at %lld", pts, sourceValidFromPts, elapsedTimeUs);
        mVideoDecoderFull = false;
    }


    mStreamLock.unlock();

#if OMAF_PLATFORM_ANDROID
    mParserThreadControlEvent.signal();
#endif
    return uploadResult;
}

Error::Enum ProviderBase::setAudioInputBuffer(AudioInputBuffer* inputBuffer)
{
    if (mAudioInputBuffer != OMAF_NULL)
    {
        return Error::ALREADY_SET;
    }
    mAudioInputBuffer = inputBuffer;
    mSynchronizer.setAudioSource(inputBuffer);

    if (mAudioInputBuffer)
    {
        // connect to new buffer.
        mAudioInputBuffer->setObserver(this);
    }
    return Error::OK;
}

void ProviderBase::enter()
{
    mResourcesInUse.wait();
}

void ProviderBase::leave()
{
    mResourcesInUse.signal();
}

void_t ProviderBase::initializeAudioFromMP4(const MP4AudioStreams& aMainAudioStreams,
                                            const MP4AudioStreams& aAdditionalAudioStreams)
{
    OMAF_ASSERT_NOT_NULL(mAudioInputBuffer);

    if (aMainAudioStreams.isEmpty() && aAdditionalAudioStreams.isEmpty())
    {
        if (mAudioUsed)
        {
            // it was used, but not any more
            mAudioInputBuffer->stopPlayback();
            mAudioInputBuffer->flush();
            mAudioInputBuffer->removeAllStreams();
        }
        else
        {
            // make sure the audio renderer is not running
            mAudioInputBuffer->initializeForNoAudio();
        }
        mAudioUsed = false;
    }
    else
    {
        mAudioUsed = true;
        for (MP4AudioStreams::ConstIterator ss = aMainAudioStreams.begin(); ss != aMainAudioStreams.end(); ++ss)
        {
            if ((*ss)->getFormat()->getMimeType() == AUDIO_AAC_MIME_TYPE)
            {
                size_t size = 0;
                uint8_t* config = const_cast<uint8_t*>((*ss)->getFormat()->getDecoderConfigInfo(size));
                if (mAudioInputBuffer->initializeForEncodedInput((*ss)->getStreamId(), config, size) == Error::OK)
                {
                    // no further main audio streams can be played
                    break;
                }
            }
        }
        for (MP4AudioStreams::ConstIterator ss = aAdditionalAudioStreams.begin(); ss != aAdditionalAudioStreams.end();
             ++ss)
        {
            size_t size = 0;
            uint8_t* config = const_cast<uint8_t*>((*ss)->getFormat()->getDecoderConfigInfo(size));

            mAudioInputBuffer->addStream((*ss)->getStreamId(), config, size);
        }
    }
    mAudioFull = false;
}

void_t ProviderBase::addAudioStreams(const MP4AudioStreams& aAdditionalAudioStreams)
{
    for (MP4AudioStreams::ConstIterator ss = aAdditionalAudioStreams.begin(); ss != aAdditionalAudioStreams.end(); ++ss)
    {
        size_t size = 0;
        uint8_t* config = const_cast<uint8_t*>((*ss)->getFormat()->getDecoderConfigInfo(size));

        mAudioInputBuffer->addStream((*ss)->getStreamId(), config, size);
        mAudioUsed = true;
    }
}

/*
 *  Audio callbacks
 */
// called in audio thread
void_t ProviderBase::onPlaybackStarted(AudioInputBuffer* caller, int64_t bufferingOffset)
{
    // now when we know how much audio data is needed to start the playback, we can start tracking audio buffers 1
    // by 1 mMinAudioBufferLevelReached = true; OMAF_LOG_D("buffers needed to start audio playback %d, latency %lld,
    // ", mMinAudioBufferLevel, bufferingOffset);
    if (caller == mAudioInputBuffer)
    {
        mSynchronizer.playbackStarted(bufferingOffset);
    }
}

// called in audio thread
void_t ProviderBase::onSamplesConsumed(AudioInputBuffer* caller, streamid_t /*streamId*/)
{
    mAudioFull = false;
}

void_t ProviderBase::onOutOfSamples(AudioInputBuffer* caller, streamid_t streamId)
{
    OMAF_LOG_D("onOutOfSamples at state %d! buffers", mState);
    mAudioFull = false;

#if OMAF_PLATFORM_ANDROID
    mParserThreadControlEvent.signal();
#endif
}

// called in audio thread
void_t ProviderBase::onAudioErrorOccurred(AudioInputBuffer* caller, Error::Enum error)
{
}

const Quaternion ProviderBase::viewingOrientationOffset() const
{
    return mViewingOrientationOffset.valid ? mViewingOrientationOffset.orientation : QuaternionIdentity;
}

Thread::ReturnValue ProviderBase::threadEntry(const Thread& thread, void_t* userData)
{
    parserThreadCallback();
    return 0;
}

const MediaInformation& ProviderBase::getMediaInformation()
{
    return mMediaInformation;
}

bool_t ProviderBase::retrieveInitialViewingOrientation(int64_t aCurrentTimeUs)
{
    bool_t done = false;
    mMediaStreamManager->readMetadata(aCurrentTimeUs);
    const MP4MetadataStreams& aStreams = mMediaStreamManager->getMetadataStreams();

    for (MP4MetadataStreams::ConstIterator it = aStreams.begin(); it != aStreams.end(); ++it)
    {
        //       to setup initial viewing orientation of background track as well
        if (StringCompare((*it)->getFormat()->getFourCC(), "invo") == ComparisonResult::EQUAL)
        {
            MP4MediaStream* invoStream = *it;

            const MP4RefStreams& refStreams = invoStream->getRefStreams(MP4VR::FourCC("cdsc"));
            // skip if trackreferences (timed metadata with trackref are updated in MP4Parser)
            if (refStreams.getSize() > 0)
            {
                continue;
            }

            MP4VRMediaPacket* packet = mMediaStreamManager->getMetadataFrame(**it, aCurrentTimeUs);
            if (packet != OMAF_NULL)
            {
                MP4VR::InitialViewingOrientationSample readSample((char*) packet->buffer(),
                                                                  (uint32_t) packet->bufferSize());
                {
                    // in theory this can overwrite an unread previous orientation, but in practice the period
                    // should be far less than the input+output queues of the video decoder
                    OMAF_ASSERT(!mLatestSignaledViewingOrientation.valid, "INVO metadata overwriting previous sample");
                    // in theory, invo is track specific, so in multiple video trak case you could have multiple
                    // invo tracks too. But 1) OMAF is designed for single track use, and 2) what would be the use
                    // case to have different invo for different parallel video tracks??
                    OMAF_ASSERT(mLatestSignaledViewingOrientation.streamId == invoStream->getStreamId() ||
                                    mLatestSignaledViewingOrientation.streamId == OMAF_UINT8_MAX,
                                "INVO metadata for multiple tracks/streams not supported");
                    OMAF_ASSERT(mLatestSignaledViewingOrientation.streamId == invoStream->getStreamId() ||
                                    mLatestSignaledViewingOrientation.streamId == OMAF_UINT8_MAX,
                                "INVO metadata for multiple tracks/streams not supported");
                    Spinlock::ScopeLock lock(mLatestSignaledViewingOrientation.lock);
                    mLatestSignaledViewingOrientation.cAzimuth = readSample.region.centreAzimuth;
                    mLatestSignaledViewingOrientation.cElevation = readSample.region.centreElevation;
                    mLatestSignaledViewingOrientation.cTilt = readSample.region.centreTilt;
                    mLatestSignaledViewingOrientation.valid = true;
                    mLatestSignaledViewingOrientation.timestampUs = packet->presentationTimeUs();
                    mLatestSignaledViewingOrientation.refresh = readSample.refreshFlag;
                    mLatestSignaledViewingOrientation.streamId = invoStream->getStreamId();
                    OMAF_LOG_V("Read new viewing orientation azimuth (%d), elevation (%d), refresh %d",
                               mLatestSignaledViewingOrientation.cAzimuth / 65536,
                               mLatestSignaledViewingOrientation.cElevation / 65536,
                               mLatestSignaledViewingOrientation.refresh);
                    done = true;
                }
                invoStream->popFirstFilledPacket();
                invoStream->returnEmptyPacket(packet);
            }
        }
    }
    return done;
}

OverlaySource* ProviderBase::getOverlaySource(uint32_t ovlyId) const
{
    const CoreProviderSources& sources = getAllSources();
    for (CoreProviderSource* source : sources)
    {
        if (source->type == SourceType::OVERLAY)
        {
            auto ovly = (OverlaySource*) source;
            if (ovly->overlayParameters.overlayId == ovlyId)
            {
                return ovly;
            }
        }
    }

    return OMAF_NULL;
}

Error::Enum ProviderBase::controlOverlayStream(streamid_t aVideoStreamId, bool_t aEnable)
{
    OMAF_LOG_V("controlOverlayStream %d value %d", aVideoStreamId, aEnable);
    streamid_t stoppedStream = InvalidStreamId;
    bool_t needToPausePlayback = false;
    bool_t previousOverlayStopped = false;
    if (aEnable)
    {
        // we are enabling an overlay, first check if we need to disable an existing (user activateable) overlay
        mMediaStreamManager->switchOverlayVideoStream(aVideoStreamId, mSynchronizer.getElapsedTimeUs(),
                                                      previousOverlayStopped);

        OMAF_LOG_D("Check if we need to enable new audio");
        MP4AudioStream* newStream =
            mMediaStreamManager->switchAudioStream(aVideoStreamId, stoppedStream, previousOverlayStopped);
        mAudioFull = false;
        if (stoppedStream != InvalidStreamId)
        {
            mAudioInputBuffer->removeStream(stoppedStream, needToPausePlayback);
            if (needToPausePlayback)
            {
                mAudioInputBuffer->stopPlayback();
                mAudioInputBuffer->flush();
                mAudioUsed = false;
            }
        }
        if (newStream != OMAF_NULL)
        {
            size_t size = 0;
            uint8_t* config = const_cast<uint8_t*>(newStream->getFormat()->getDecoderConfigInfo(size));

            mAudioInputBuffer->addStream(newStream->getStreamId(), config, size);
            if (mAudioInputBuffer->getState() != AudioState::PLAYING)
            {
                // delay start until there are enough data in audioinputbuffer
                mOverlayAudioPlayPending = true;
            }
            mAudioUsed = true;
        }
    }
    else
    {
        // else we are disabling an overlay

        mMediaStreamManager->stopOverlayVideoStream(aVideoStreamId);

        // stop also its audio, if exists
        if (mMediaStreamManager->stopOverlayAudioStream(aVideoStreamId, stoppedStream) &&
            stoppedStream != InvalidStreamId)
        {
            OMAF_LOG_D("Stop audio when closing overlay");
            mAudioFull = false;
            mAudioInputBuffer->removeStream(stoppedStream, needToPausePlayback);
            if (needToPausePlayback)
            {
                mAudioInputBuffer->stopPlayback();
                mAudioInputBuffer->flush();
                mAudioUsed = false;
            }
        }
    }
    return Error::OK;
}


void_t ProviderBase::setupInitialViewingOrientation(HeadTransform& aCurrentHeadTransform, uint64_t aTimestampUs)
{
    if (mCoordSysRotationUpdated)
    {
        // delta, i.e. relative to reference coordinate system
        mViewingOrientationOffset.orientation = OMAF::makeQuaternion(
            OMAF::toRadians((float64_t) mViewpointCoordSysRotation.pitch / 65536),
            OMAF::toRadians((float64_t) mViewpointCoordSysRotation.yaw / 65536),
            OMAF::toRadians((float64_t) mViewpointCoordSysRotation.roll / 65536), OMAF::EulerAxisOrder::YXZ);

        mViewingOrientationOffset.valid = true;
        mCoordSysRotationUpdated = false;
    }
    if (mLatestSignaledViewingOrientation.valid &&
        (mLatestSignaledViewingOrientation.refresh ||
         aTimestampUs >= mLatestSignaledViewingOrientation.timestampUs))  // this criteria may cause a 1 frame delay
                                                                          // for the initial viewing orientation to
                                                                          // take effect
    {
        // we get absolute azimuth, not a delta. However, we need to translate that to delta, in relation to the
        // viewing orientation when the requested viewing orientation takes place
        Quaternion sensor = aCurrentHeadTransform.orientation;

        // We take the current sensor as the new origin for yaw/azimuth. Hence the current yaw should be inverted.
        // OMAF spec says the player should obey all given coordinates in non-sensor case, but we don't do that at
        // the moment
        float32_t yaw, pitch, roll;

        eulerAngles(sensor, yaw, pitch, roll, EulerAxisOrder::YXZ);
        sensor =
            makeQuaternion(OMAF::toRadians((float64_t) mViewpointCoordSysRotation.pitch / 65536),
                           OMAF::toRadians((float64_t) mViewpointCoordSysRotation.yaw / 65536) - yaw,
                           OMAF::toRadians((float64_t) mViewpointCoordSysRotation.roll / 65536), EulerAxisOrder::XYZ);

        {
            // then calculate the offset: signaled orientation + the inverted current yaw
            Spinlock::ScopeLock lock(mLatestSignaledViewingOrientation.lock);
            mViewingOrientationOffset.orientation =
                OMAF::makeQuaternion(0.f,
                                     OMAF::toRadians((float64_t) mLatestSignaledViewingOrientation.cAzimuth / 65536),
                                     0.f, OMAF::EulerAxisOrder::YXZ) *
                sensor;

            mViewingOrientationOffset.valid = true;
            mLatestSignaledViewingOrientation.valid = false;
            OMAF_LOG_V("Set up new viewing orientation, pts %lld", mLatestSignaledViewingOrientation.timestampUs);
        }
    }
    if (mViewingOrientationOffset.valid)
    {
        // and finally calculate the orientation. Right after resetting the offset, this should have the yaw from
        // the signaled sample, but further changes to head orientation then move around it
        aCurrentHeadTransform.orientation = mViewingOrientationOffset.orientation * aCurrentHeadTransform.orientation;
    }
}

OMAF_NS_END
