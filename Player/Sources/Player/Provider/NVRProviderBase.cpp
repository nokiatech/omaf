
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
#include "Provider/NVRProviderBase.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(ProviderBase)


ProviderBase::ProviderBase()
: mAudioInputBuffer(OMAF_NULL)
, mAuxiliaryAudioInputBuffer(OMAF_NULL)
, mVideoDecoder(VideoDecoderManager::getInstance())
, mUserActionSync(false, false)
, mPendingUserAction(PendingUserAction::INVALID)
, mStreamInitialPositionMS(0)
, mSeekTargetUs(OMAF_UINT64_MAX)
, mSeekTargetFrame(OMAF_UINT64_MAX)
, mSeekTargetUsAuxiliary(OMAF_UINT64_MAX)
, mParserThreadControlEvent(false,false)
, mVideoDecoderFull(false)
, mAudioFull(false)
, mFirstVideoPacketRead(false)
, mFirstFramePTSUs(0)
, mFirstDisplayFramePTSUs(OMAF_UINT64_MAX)
, mElapsedTimeUs(0)
, mAudioSyncPending(true)
, mState(VideoProviderState::IDLE)
, mSeekSyncronizeEvent(false, false)
, mSeekResult(Error::OK)
, mAuxiliaryPlaybackState(VideoProviderState::INVALID)
, mAuxiliaryStreamPlaying(false)
, mAuxiliaryStreamInitialized(false)
, mAuxiliarySyncPending(true)
, mElapsedTimeUsAuxiliary(0)
, mAuxiliaryAudioInitPending(false)
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

    if (mAuxiliaryAudioInputBuffer != OMAF_NULL)
    {
        mAuxiliaryAudioInputBuffer->flush();
        mAuxiliaryAudioInputBuffer->removeObserver(this);
    }

    for (Streams::ConstIterator it = mPreviousStreams.begin(); it != mPreviousStreams.end(); ++it)
    {
        mVideoDecoder->deactivateStream(*it);
    }

    mAudioInputBuffer = OMAF_NULL;
}

void_t ProviderBase::setState(VideoProviderState::Enum state)
{
    if (state == mState)
    {
        return;
    }
    mState = state;
    OMAF_LOG_D("State set to: %s", VideoProviderState::toString(mState));
}

void_t ProviderBase::setAuxliaryState(VideoProviderState::Enum state)
{
    if (state == mAuxiliaryPlaybackState)
    {
        return;
    }

    mAuxiliaryPlaybackState = state;
    OMAF_LOG_D("Auxiliary state set to: %s", VideoProviderState::toString(mAuxiliaryPlaybackState));
}

VideoProviderState::Enum ProviderBase::getState() const
{
    return mState;
}

Error::Enum ProviderBase::loadSource(const PathName &source, uint64_t initialPositionMS)
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
    if (mState == VideoProviderState::PLAYING)
    {
        return Error::OK_NO_CHANGE;
    }
    else if (mState == VideoProviderState::LOADING
             || mState == VideoProviderState::LOADED
             || mState == VideoProviderState::PAUSED
             || mState == VideoProviderState::BUFFERING
             || mState == VideoProviderState::END_OF_FILE)
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
    if (mState == VideoProviderState::PLAYING ||
        mState == VideoProviderState::BUFFERING)
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

    return Error::INVALID_STATE;
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
    return Error::NOT_SUPPORTED;
}

// called in client thread
Error::Enum ProviderBase::seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy)
{
    if (getState() != VideoProviderState::LOADED
        && getState() != VideoProviderState::PLAYING
        && getState() != VideoProviderState::BUFFERING
        && getState() != VideoProviderState::PAUSED
        && getState() != VideoProviderState::END_OF_FILE)
    {
        return Error::INVALID_STATE;
    }

    if (!isSeekable())
    {
        return Error::NOT_SUPPORTED;
    }

    mSeekAccuracy = seekAccuracy;
    uint64_t seekTargetInUs = (aSeekMs * 1000); //mFirstFramePTSUs;
    // is there case where mFirstFramePTSUs value actually would fix something? This breaks Android seek after suspend/stop with GearVR HMD removal.
    // since first packet PTS is not 0, but basically first downloaded media segment PTS from middle of static stream playback.
    // OMAF_LOG_D("seekToMs: aSeekMs: %lld, mFirstFramePTSUs: %lld, seekTargetInUs: %lld ", aSeekMs, mFirstFramePTSUs, seekTargetInUs);

    if (seekTargetInUs > (durationMs() * 1000) && durationMs() != 0)
    {
        return Error::OPERATION_FAILED;
    }

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
    }
    else
    {
        mAudioFull = true;
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

void_t ProviderBase::startAuxiliaryPlayback(bool_t waitForAudio)
{
    mAuxiliarySynchronizer.setSyncRunning(waitForAudio);

    if (mAuxiliaryAudioInputBuffer != OMAF_NULL && mAuxiliaryAudioInputBuffer->isInitialized())
    {
        mAuxiliaryAudioInputBuffer->startPlayback();
    }
}

void_t ProviderBase::stopAuxiliaryPlayback()
{
    if (mAuxiliaryAudioInputBuffer != OMAF_NULL)
    {
        mAuxiliaryAudioInputBuffer->stopPlayback();
    }
    mAuxiliarySynchronizer.stopSyncRunning();
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

uint64_t ProviderBase::elapsedTimeMsAuxiliary() const
{
    if (mElapsedTimeUsAuxiliary < mAuxiliarySynchronizer.getFrameOffset())
    {
        return mElapsedTimeUsAuxiliary / 1000;
    }
    else
    {
        return (mElapsedTimeUsAuxiliary - mAuxiliarySynchronizer.getFrameOffset()) / 1000;
    }
}

PacketProcessingResult::Enum ProviderBase::processMP4Video(MP4StreamManager &streamManager,
                                                           MP4StreamManager *auxiliaryStreamManager)
{
    bool_t endOfFile = false;
    uint64_t currentPTS = mSynchronizer.getElapsedTimeUs();
    const MP4VideoStreams& streams = streamManager.getVideoStreams();

    if (streams.isEmpty())
    {
        return PacketProcessingResult::OK;//??
    }
    streamManager.readVideoFrames(currentPTS);

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

    if (auxiliaryStreamManager != OMAF_NULL)
    {
        const MP4VideoStreams& auxStreams = auxiliaryStreamManager->getVideoStreams();
        if (auxStreams.getSize() != 1)
        {
            OMAF_LOG_E("Only a single aux stream supported");
        }
        else
        {
            currentStreams.add(auxStreams.at(0)->getStreamId());
        }
    }


    // First check all the streams that are no longer in use and deactivate them
    for (Streams::ConstIterator it = mPreviousStreams.begin(); it != mPreviousStreams.end(); ++it)
    {
        streamid_t streamId = *it;
        if (!currentStreams.contains(streamId))
        {
            OMAF_LOG_D("deactivate %d", streamId);
            mVideoDecoder->deactivateStream(streamId);
        }
    }

    for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        MP4VideoStream* videoStream = *it;

        streamid_t streamId = videoStream->getStreamId();
        bool_t isActivatedNow = false;

        if (videoStream->getMode() == VideoStreamMode::BASE && mVideoDecoder->isEOS(streamId))
        {
            usedStreams.add(streamId);
            endOfFile = true;
        }
        else
        {
            MP4VRMediaPacket *videoPacket = streamManager.getNextVideoFrame(*videoStream,
                                                                            currentPTS);
            if (videoPacket == OMAF_NULL)
            {
                if (videoStream->getMode() != VideoStreamMode::ENHANCEMENT_FAST_FORWARD)
                {
                    // We want to avoid activating enhancement streams too early (before they have any data) so that decoders to existing streams don't need to be flushed. To ensure the frames are fetched from the decoders, including end of file flag, we need to mark the
                    // stream as used stream.
                    // However, as we keep the stream decoding before actually deactivating it, if user quickly turns his head back and forth the framecache can have suitable frames and get synced, and the stream mode may become enh_normal before any new data is downloaded,
                    // hence it gets to mPreviousStreams and we need to activate the stream now, even without any new data is available.
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

                    if (videoStream->getMode() == VideoStreamMode::BASE)
                    {
                        if (streamManager.isEOS() || streamManager.isReadyToSignalEoS(*videoStream))
                        {
                            // notify decoder about end of stream
                            OMAF_LOG_V("Set EOS after empty packet from base layer stream %d",
                                videoStream->getStreamId());
                            mVideoDecoder->setInputEOS(streamId);
                        }
                        else
                        {
                            buffering = true;
                        }
                    }
                    else if (videoStream->getMode() == VideoStreamMode::ENHANCEMENT_NORMAL)
                    {
                        OMAF_LOG_D(
                                "Normal stream %d moved back to fast_forward since run out of data",
                                videoStream->getStreamId());
                        videoStream->setMode(VideoStreamMode::ENHANCEMENT_FAST_FORWARD);
                    }
                }
            }
            else
            {
                // If the stream is new, it needs to be activated (better to do it now when there is some data for it)
                if (!mPreviousStreams.contains(streamId))
                {
                    OMAF_LOG_D("activate %d", streamId);
                    Error::Enum result = mVideoDecoder->activateStream(streamId, currentPTS);
                    if (result != Error::OK)
                    {
                        return PacketProcessingResult::ERROR;
                    }
                    isActivatedNow = true;
                }
                usedStreams.add(streamId);

                // When reading the first video frame, set the synchronizer time to the PTS of the first video fram
                // This enables the drawing the
                if (!mFirstVideoPacketRead)
                {
                    mFirstVideoPacketRead = true;
                    mFirstFramePTSUs = videoPacket->presentationTimeUs();
                    //OMAF_LOG_D("Found first frame at %lld", videoPacket->presentationTimeUs());
                }
                DecodeResult::Enum result = mVideoDecoder->decodeMediaPacket(streamId, videoPacket);
                if (result == DecodeResult::PACKET_ACCEPTED)
                {
                    // The packet was copied to the decoder, remove 1st packet from filled queue
                    //OMAF_LOG_D("decodeMediaPacket %lld for stream %d", videoPacket->presentationTimeUs(), videoStream->getStreamId());
                    videoStream->popFirstFilledPacket();
                    videoStream->returnEmptyPacket(videoPacket);
                    allVideoDecodersFull = false;
                }
                else if (result == DecodeResult::DECODER_FULL || result == DecodeResult::NOT_READY)
                {
                    //Decoder can't handle more data. try again later.
                    //OMAF_LOG_D("Cannot decode, keep");
                }
                else if (result == DecodeResult::DECODER_ERROR)
                {
                    return PacketProcessingResult::ERROR;
                }

                if (mAudioSyncPending && !mAudioInputBuffer->isInitialized())
                {
                    mSynchronizer.setSyncPoint(videoPacket->presentationTimeUs());
                    mAudioSyncPending = false;
                }
                retrieveInitialViewingOrientation(*videoStream);
            }
        }

        if (isActivatedNow || mPreviousStreams.contains(streamId))
        {
            // old streams mapped to lists for preloadTextures; not yet activated streams must not get there (they don't have a decoder), but old streams without new data must get in EOS case
            if (videoStream->getMode() == VideoStreamMode::BASE)
            {
                baseStreamIds.add(streamId);
            }
            else if (videoStream->getMode() == VideoStreamMode::ENHANCEMENT_NORMAL)
            {
                enhStreamIds.add(streamId);
            }
            else
            {
                skippedStreamIds.add(streamId);
            }
        }

    }
    uint64_t currentPTSAuxiliary = 0;
    Streams auxStreams;
    if (auxiliaryStreamManager != OMAF_NULL)
    {
        auxiliaryStreamManager->readVideoFrames(0);
        MP4VideoStream* videoStream = auxiliaryStreamManager->getVideoStreams().at(0);
        streamid_t streamID = videoStream->getStreamId();
        auxStreams.add(streamID);
        currentPTSAuxiliary = mAuxiliarySynchronizer.getElapsedTimeUs();
        MP4VRMediaPacket* videoPacket = auxiliaryStreamManager->getNextVideoFrame(*videoStream, currentPTS);
        if (videoPacket == OMAF_NULL)
        {
            mVideoDecoder->setInputEOS(streamID);
        }
        else
        {
            Error::Enum result = Error::OK;
            if (!mAuxiliaryStreamInitialized)
            {
                {
                    Spinlock::ScopeLock lock(mAuxiliarySourcesLock);
                    mAuxiliarySources.add(videoStream->getVideoSources());
                }

                result = mVideoDecoder->activateStream(streamID, 0);
                mAuxiliaryStreamInitialized = true;
            }
            if (mAuxiliarySyncPending && !mAuxiliaryAudioInputBuffer->isInitialized())
            {
                int64_t packetPTS = videoPacket->presentationTimeUs();
                mAuxiliarySynchronizer.setSyncPoint(packetPTS);
                mAuxiliarySyncPending = false;
            }

            if (result != Error::OK)
            {
                mAuxiliaryPlaybackState = VideoProviderState::STREAM_ERROR;
            }
            else
            {
                DecodeResult::Enum decodeResult = mVideoDecoder->decodeMediaPacket(streamID, videoPacket);
                if (decodeResult == DecodeResult::PACKET_ACCEPTED)
                {
                    // The packet was copied to the decoder, remove 1st packet from filled queue
                    //OMAF_LOG_D("decodeMediaPacket %lld for stream %d", videoPacket->presentationTimeUs(), videoStream->getStreamId());
                    videoStream->popFirstFilledPacket();
                    videoStream->returnEmptyPacket(videoPacket);
                    allVideoDecodersFull = false;
                }
                else if (decodeResult == DecodeResult::DECODER_FULL || decodeResult == DecodeResult::NOT_READY)
                {
                    //Decoder can't handle more data. try again later.
                    //OMAF_LOG_D("Cannot decode, keep");
                }
                else if (decodeResult == DecodeResult::DECODER_ERROR)
                {
                    OMAF_LOG_E("Auxiliary stream decode error");
                    mAuxiliaryPlaybackState = VideoProviderState::STREAM_ERROR;
                }
            }
        }
    }

    if (allVideoDecodersFull)
    {
        mVideoDecoderFull = true;
    }

#ifdef _DEBUG
    //TODO decoder buffering was ignored in ABR task in August 2017. But it may mean the decoder never catches up the playhead and no pictures are shown - at least on slow machines
    buffering |= mVideoDecoder->isBuffering(baseStreamIds);
#endif
    Error::Enum preloadStatus = mVideoDecoder->preloadTexturesForPTS(baseStreamIds, enhStreamIds, skippedStreamIds, currentPTS);
    if (preloadStatus != Error::OK && preloadStatus != Error::OK_SKIPPED && preloadStatus != Error::OK_NO_CHANGE && preloadStatus != Error::ITEM_NOT_FOUND)
    {
        return PacketProcessingResult::ERROR;
    }
    if (auxiliaryStreamManager != OMAF_NULL)
    {
        mVideoDecoder->preloadTexturesForPTS(auxStreams, Streams(), Streams(), currentPTSAuxiliary, true);
    }
    mPreviousStreams.clear();
    mPreviousStreams.add(usedStreams);
    if (buffering)
    {
        return PacketProcessingResult::BUFFERING;
//        return PacketProcessingResult::OK;
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

PacketProcessingResult::Enum ProviderBase::processMP4Audio(MP4StreamManager& streamManager, AudioInputBuffer* audioInputBuffer)
{
    OMAF_ASSERT_NOT_NULL(mAudioInputBuffer);

    const MP4AudioStreams& astreams = streamManager.getAudioStreams();
    if (astreams.isEmpty())
    {
        return PacketProcessingResult::OK;
    }

    MP4AudioStream* audio = astreams.at(0);
    int32_t limit = AUDIO_REQUEST_THRESHOLD;
    bool_t isAuxStream = audioInputBuffer == mAuxiliaryAudioInputBuffer;
    bool_t buffering = false;

    for (int32_t count = 0; count < limit; count++)
    {
        streamManager.readAudioFrames();

        MP4VRMediaPacket* audioPacket = streamManager.getNextAudioFrame(*audio);

        if (audioPacket == OMAF_NULL)
        {
            if (streamManager.isEOS())
            {
                // No more audio packets to read so mark as full
                if (!isAuxStream)
                {
                    mAudioFull = true;
                }
                buffering = true;
                break;
            }
            break;
        }
        //OMAF_LOG_D("write audio %lld", audioPacket->presentationTimeUs());
        const streamid_t defaultAudioStreamId = 0;
        Error::Enum success = audioInputBuffer->write(defaultAudioStreamId, audioPacket);

        if (success == Error::OK)
        {
            if (mAudioSyncPending && !isAuxStream)
            {
                mSynchronizer.setSyncPoint(audioPacket->presentationTimeUs());
                mAudioSyncPending = false;
            }
            else if (mAuxiliarySyncPending && isAuxStream)
            {
                mAuxiliarySynchronizer.setSyncPoint(audioPacket->presentationTimeUs());
                mAuxiliarySyncPending = false;
            }

            //OMAF_LOG_D("wrote audio");
            audio->popFirstFilledPacket();
            audio->returnEmptyPacket(audioPacket);
        }
        else if (success == Error::BUFFER_OVERFLOW)
        {
            if (!isAuxStream)
            {
                mAudioFull = true;
            }
            break;
        }
        else
        {
            OMAF_LOG_W("Failed writing audio, error %d", success);
            break;
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

bool_t ProviderBase::setInitialViewport(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical)
{
    // is relevant for viewport dependent operation only, skipped
    return true;
}

Streams ProviderBase::extractStreamsFromSources(const CoreProviderSources &sources)
{
    Streams streams;
    for (CoreProviderSources::ConstIterator it = sources.begin(); it != sources.end(); ++it)
    {
        if ((*it)->category == SourceCategory::VIDEO)
        {
            VideoFrameSource* videoSource = (VideoFrameSource*)(*it);
            if (!streams.contains(videoSource->streamIndex))
            {
                streams.add(videoSource->streamIndex);
            }
        }
    }
    return streams;
}

const CoreProviderSources& ProviderBase::getSources()
{
    return mPreparedSources;
}

Error::Enum ProviderBase::prepareSources(HeadTransform currentHeadtransform, float32_t fovHorizontal, float32_t fovVertical)
{
    VideoProviderState::Enum state = getState();

    if (state != VideoProviderState::PAUSED
        && state != VideoProviderState::PLAYING
        && state != VideoProviderState::END_OF_FILE
        && state != VideoProviderState::BUFFERING
        && state != VideoProviderState::LOADED)
    {
    
        if (!mViewingOrientationOffset.valid)
        {
            // make sure we start at yaw = 0, no matter where the user is watching. If there's signaled initial viewing orientation, it will be applied when the state is such that this step is skipped, and it will override this one
            Quaternion sensorOffset = currentHeadtransform.orientation;

            // We take the current sensor as the new origin for yaw/azimuth. Hence the current yaw should be inverted. 
            float32_t yaw, pitch, roll;
            eulerAngles(sensorOffset, yaw, pitch, roll, EulerAxisOrder::YXZ);
            sensorOffset = makeQuaternion(0.f, -yaw, 0.f, EulerAxisOrder::YXZ);

            mViewingOrientationOffset.orientation = sensorOffset;

            mViewingOrientationOffset.valid = true;

            setupInitialViewingOrientation(currentHeadtransform, 0);
            if (!setInitialViewport(currentHeadtransform, fovHorizontal, fovVertical))
            {
                mViewingOrientationOffset.valid = false; // triggers to retry
            }
        }
        return Error::OK_SKIPPED;
    }

    CoreProviderSources allSelectedSources;
    CoreProviderSources requiredSources;
    CoreProviderSources requiredVideoSources;
    CoreProviderSources enhancementSources;

    uint64_t elapsedTimeUs = mSynchronizer.getElapsedTimeUs();
    mElapsedTimeUs = elapsedTimeUs;

    // initial viewing orientation should be added to currentHeadTransform before selecting sources
    setupInitialViewingOrientation(currentHeadtransform, elapsedTimeUs);

    uint64_t sourceValidFromPts = selectSources(currentHeadtransform, fovHorizontal, fovVertical, requiredSources, enhancementSources);
    for(CoreProviderSources::ConstIterator it = requiredSources.begin(); it != requiredSources.end(); ++it)
    {
        requiredVideoSources.add(*it);
    }
    allSelectedSources.add(requiredSources);
    allSelectedSources.add(enhancementSources);

    Streams requiredStreams = extractStreamsFromSources(requiredVideoSources);
    Streams enhancementStreams = extractStreamsFromSources(enhancementSources);


    // Draw first frame hack
    if (mFirstDisplayFramePTSUs != OMAF_UINT64_MAX && elapsedTimeUs < mFirstDisplayFramePTSUs)
    {
        elapsedTimeUs = mFirstDisplayFramePTSUs;
    }

    TextureLoadOutput textureLoadOutput;
    Error::Enum uploadResult = mVideoDecoder->uploadTexturesForPTS(
        requiredStreams, enhancementStreams,
        elapsedTimeUs, textureLoadOutput, false);

    if (uploadResult == Error::OK)
    {
        uint64_t pts = OMAF_UINT64_MAX;
        if (!requiredStreams.isEmpty())
        {
            pts = mVideoDecoder->getCurrentVideoFrame(requiredStreams.at(0)).pts;
        }
        if (requiredStreams.isEmpty() || pts < sourceValidFromPts)
        {
            if (pts < sourceValidFromPts)
            {
                OMAF_LOG_D("Source pts %llu is for a newer frame than %llu", sourceValidFromPts,
                           pts);
            }
            // reuse the old sources list but update the texture references
            for (CoreProviderSources::Iterator it = mPreparedSources.begin(); it != mPreparedSources.end(); ++it)
            {
                VideoFrameSource* videoFrameSource = (VideoFrameSource*)(*it);
                streamid_t streamId = videoFrameSource->streamIndex;

                if (textureLoadOutput.updatedStreams.contains(streamId))
                {
                    videoFrameSource->videoFrame = mVideoDecoder->getCurrentVideoFrame(streamId);
                    // Video resolution can change so update it
                    mMediaInformation.width = videoFrameSource->widthInPixels;
                    mMediaInformation.height = videoFrameSource->heightInPixels;
                    if (mViewingOrientationOffset.valid)
                    {
                        // initial viewing orientation has to be stored to the source to pass it to the renderer.
                        // And we need to calculate it in provider due to viewport dependent operation
                        videoFrameSource->forcedOrientation.orientation = currentHeadtransform.orientation;
                        videoFrameSource->forcedOrientation.valid = true;
                    }
                }
            }
        }
        else
        {
            // normal case, create new sources for the new texture(s)

            mFirstDisplayFramePTSUs = OMAF_UINT64_MAX;
            mPreparedSources.clear();
            for (CoreProviderSources::ConstIterator it = allSelectedSources.begin(); it != allSelectedSources.end(); ++it)
            {
                CoreProviderSource* coreProviderSource = *it;

                if (coreProviderSource->category == SourceCategory::VIDEO)
                {
                    VideoFrameSource* videoFrameSource = (VideoFrameSource*)coreProviderSource;
                    streamid_t streamId = videoFrameSource->streamIndex;

                    if (textureLoadOutput.updatedStreams.contains(streamId))
                    {
                        videoFrameSource->videoFrame = mVideoDecoder->getCurrentVideoFrame(streamId);
                        // Video resolution can change so update it
                        mMediaInformation.width = videoFrameSource->widthInPixels;
                        mMediaInformation.height = videoFrameSource->heightInPixels;
                        if (mViewingOrientationOffset.valid)
                        {
                            // initial viewing orientation has to be stored to the source to pass it to the renderer.
                            // And we need to calculate it in provider due to viewport dependent operation
                            videoFrameSource->forcedOrientation.orientation = currentHeadtransform.orientation;
                            videoFrameSource->forcedOrientation.valid = true;
                        }
                        mPreparedSources.add(videoFrameSource);
                    }
                }
            }
        }
        //OMAF_LOG_V("Rendered frame %lld source %lld", pts, sourceValidFromPts);
        mVideoDecoderFull = false;
    }

    mAuxiliarySourcesLock.lock();
    if (!mAuxiliarySources.isEmpty())
    {
        TextureLoadOutput auxTextureLoadOutput;
        // Supporting only a single aux stream at the moment
        CoreProviderSource *auxSource = mAuxiliarySources.at(0);

        if (auxSource->type == SourceType::IDENTITY_AUXILIARY)
        {
            VideoFrameSource* videoFrameSource = (VideoFrameSource*)auxSource;
            Streams auxStreamIds;
            auxStreamIds.add(videoFrameSource->streamIndex);
            uint64_t auxStreamTime = mAuxiliarySynchronizer.getElapsedTimeUs();
            mElapsedTimeUsAuxiliary = auxStreamTime;
            Error::Enum auxUploadResult = mVideoDecoder->uploadTexturesForPTS(auxStreamIds, Streams(), auxStreamTime, auxTextureLoadOutput, true);
            if (auxUploadResult == Error::OK)
            {
                if (auxTextureLoadOutput.updatedStreams.contains(videoFrameSource->streamIndex))
                {
                    videoFrameSource->videoFrame = mVideoDecoder->getCurrentVideoFrame(videoFrameSource->streamIndex);
                    if (!mPreparedSources.contains(videoFrameSource))
                    {
                        mPreparedSources.add(videoFrameSource);
                    }
                }
            }
        }
    }
    mAuxiliarySourcesLock.unlock();

#if OMAF_PLATFORM_ANDROID
    mParserThreadControlEvent.signal();
#endif
    return uploadResult;
}

Error::Enum ProviderBase::setAudioInputBuffer(AudioInputBuffer *inputBuffer)
{
    if (mAudioInputBuffer != OMAF_NULL)
    {
        return Error::ALREADY_SET;
    }
    mAudioInputBuffer = inputBuffer;
    mSynchronizer.setAudioSource(inputBuffer);

    if (mAudioInputBuffer)
    {
        //connect to new buffer.
        mAudioInputBuffer->setObserver(this);
        mAuxiliaryAudioInputBuffer = inputBuffer->getAuxiliaryAudioInputBuffer();
        if (mAuxiliaryAudioInputBuffer != OMAF_NULL)
        {
            mAuxiliarySynchronizer.setAudioSource(mAuxiliaryAudioInputBuffer);
            mAuxiliaryAudioInputBuffer->setObserver(this);
        }
    }
    return Error::OK;
}

void ProviderBase::enter() {
    mResourcesInUse.wait();
}

void ProviderBase::leave() {
    mResourcesInUse.signal();
}

void_t ProviderBase::initializeAuxiliaryAudio(const MP4AudioStreams& audioStreams)
{
    if (audioStreams.isEmpty())
    {
        mAuxiliaryAudioInputBuffer->initializeForNoAudio();
    }
    else
    {
        MP4AudioStream* audioStream = audioStreams.at(0);
        if (audioStream->getFormat()->getMimeType() == AUDIO_AAC_MIME_TYPE)
        {
            size_t size = 0;
            uint8_t* config = const_cast<uint8_t*>(audioStream->getFormat()->getDecoderConfigInfo(size));
            mAuxiliaryAudioInputBuffer->initializeForEncodedInput(1, config, size);
        }
        else
        {
            OMAF_LOG_W("Auxiliary track isn't AAC so ignoring audio");
            mAuxiliaryAudioInputBuffer->initializeForNoAudio();
        }
    }
}

void_t ProviderBase::initializeAudioFromMP4(const MP4AudioStreams &audioStreams)
{
    OMAF_ASSERT_NOT_NULL(mAudioInputBuffer);

    if (audioStreams.isEmpty())
    {
        // make sure the audio renderer is not running
        mAudioInputBuffer->initializeForNoAudio();
    }
    else
    {
        if (audioStreams.getSize() > 1)
        {
            OMAF_LOG_W("Multiple audio tracks detected, only the first one will be used");
        }
        MP4AudioStreams::ConstIterator ss = audioStreams.begin();
        if ((*ss)->getFormat()->getMimeType() == AUDIO_AAC_MIME_TYPE)
        {
            size_t size = 0;
            uint8_t* config = const_cast<uint8_t*>((*ss)->getFormat()->getDecoderConfigInfo(size));
            mAudioInputBuffer->initializeForEncodedInput(1, config, size);
        }
        else
        {
            // let audio renderer to decide what to do; assume some encoded format (like mp3)
            size_t size = 0;
            uint8_t* config = const_cast<uint8_t*>((*ss)->getFormat()->getDecoderConfigInfo(size));
            mAudioInputBuffer->initializeForEncodedInput(1, config, size);
            mMediaInformation.audioFormat = AudioFormat::LOUDSPEAKER;
        }
    }
}

/*
 *  Audio callbacks
 */
// called in audio thread
void_t ProviderBase::onPlaybackStarted(AudioInputBuffer* caller, int64_t bufferingOffset)
{
    // now when we know how much audio data is needed to start the playback, we can start tracking audio buffers 1 by one
    //mMinAudioBufferLevelReached = true;
    //OMAF_LOG_D("buffers needed to start audio playback %d, latency %lld, ", mMinAudioBufferLevel, bufferingOffset);
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

Thread::ReturnValue ProviderBase::threadEntry(const Thread& thread, void_t* userData)
{
    parserThreadCallback();
    return 0;
}

const MediaInformation& ProviderBase::getMediaInformation()
{
    return mMediaInformation;
}

void_t ProviderBase::retrieveInitialViewingOrientation(MP4VideoStream& aStream)
{
    if (aStream.getMetadataStream() != OMAF_NULL)
    {
        MP4MediaStream* invoStream = aStream.getMetadataStream("invo");
        MP4VRMediaPacket* packet = invoStream->peekNextFilledPacket();
        if (packet != OMAF_NULL)
        {
            MP4VR::InitialViewingOrientationSample readSample((char*)packet->buffer(), (uint32_t)packet->bufferSize());
            {
                // in theory this can overwrite an unread previous orientation, but in practice the period should be far less than the input+output queues of the video decoder
                OMAF_ASSERT(!mLatestSignaledViewingOrientation.valid, "INVO metadata overwriting previous sample");
                // in theory, invo is track specific, so in multiple video trak case you could have multiple invo tracks too. But 1) OMAF is designed for single track use, and 2) what would be the use case to have different invo for different parallel video tracks??
                OMAF_ASSERT(mLatestSignaledViewingOrientation.streamId == aStream.getStreamId() || mLatestSignaledViewingOrientation.streamId == OMAF_UINT8_MAX, "INVO metadata for multiple tracks/streams not supported");
                Spinlock::ScopeLock lock(mLatestSignaledViewingOrientation.lock);
                mLatestSignaledViewingOrientation.cAzimuth = readSample.region.centreAzimuth;
                mLatestSignaledViewingOrientation.cElevation = readSample.region.centreElevation;
                mLatestSignaledViewingOrientation.cTilt = readSample.region.centreTilt;
                mLatestSignaledViewingOrientation.valid = true;
                mLatestSignaledViewingOrientation.timestampUs = packet->presentationTimeUs();
                mLatestSignaledViewingOrientation.refresh = readSample.refreshFlag;
                mLatestSignaledViewingOrientation.streamId = aStream.getStreamId();
                OMAF_LOG_V("Read new viewing orientation azimuth (%d), elevation (%d), refresh %d", mLatestSignaledViewingOrientation.cAzimuth / 65536, mLatestSignaledViewingOrientation.cElevation / 65536, mLatestSignaledViewingOrientation.refresh);
            }
            invoStream->popFirstFilledPacket();
            invoStream->returnEmptyPacket(packet);
        }
    }
}

void_t ProviderBase::setupInitialViewingOrientation(HeadTransform& aCurrentHeadTransform, uint64_t aTimestampUs)
{
    if (mLatestSignaledViewingOrientation.valid && mLatestSignaledViewingOrientation.refresh && aTimestampUs >= mLatestSignaledViewingOrientation.timestampUs)  // this criteria may cause a 1 frame delay for the initial viewing orientation to take effect
    {
        // we get absolute azimuth, not a delta. However, we need to translate that to delta, in relation to the viewing orientation when the requested viewing orientation takes place
        Quaternion sensor = aCurrentHeadTransform.orientation;

        // We take the current sensor as the new origin for yaw/azimuth. Hence the current yaw should be inverted. OMAF spec says the player should obey all given coordinates in non-sensor case, but we don't do that at the moment
        float32_t yaw, pitch, roll;
        eulerAngles(sensor, yaw, pitch, roll, EulerAxisOrder::YXZ);
        sensor = makeQuaternion(0.f, -yaw, 0.f, EulerAxisOrder::YXZ);
        
        {
            // then calculate the offset: signaled orientation + the inverted current yaw
            Spinlock::ScopeLock lock(mLatestSignaledViewingOrientation.lock);
            mViewingOrientationOffset.orientation = OMAF::makeQuaternion(0, OMAF::toRadians((float64_t)mLatestSignaledViewingOrientation.cAzimuth / 65536), 0, OMAF::EulerAxisOrder::YXZ) * sensor;

            mViewingOrientationOffset.valid = true;
            mLatestSignaledViewingOrientation.valid = false;
            OMAF_LOG_V("Set up new viewing orientation, pts %lld", mLatestSignaledViewingOrientation.timestampUs);
        }
    }
    if (mViewingOrientationOffset.valid)
    {
        // and finally calculate the orientation. Right after resetting the offset, this should have the yaw from the signaled sample, but further changes to head orientation then move around it
        aCurrentHeadTransform.orientation = mViewingOrientationOffset.orientation * aCurrentHeadTransform.orientation;
    }
}

OMAF_NS_END
