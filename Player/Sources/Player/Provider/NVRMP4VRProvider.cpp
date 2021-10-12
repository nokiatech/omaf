
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
#include "Provider/NVRMP4VRProvider.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMP4MediaStreamManager.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MP4VRProvider)

MP4VRProvider::MP4VRProvider()
{
    mMP4MediaStreamManager = OMAF_NEW_HEAP(MP4MediaStreamManager);
    mMediaStreamManager = mMP4MediaStreamManager;
}

MP4VRProvider::~MP4VRProvider()
{
    setState(VideoProviderState::CLOSING);

#if OMAF_PLATFORM_ANDROID
    mParserThreadControlEvent.signal();
#endif

    if (mParserThread.isValid())
    {
        mParserThread.stop();
        mParserThread.join();
    }

    destroyInstance();

    mMP4MediaStreamManager->resetStreams();
    OMAF_DELETE_HEAP(mMP4MediaStreamManager);
}
Error::Enum MP4VRProvider::openFile(PathName& uri)
{
    OMAF_LOG_D("Loading from uri: %s", uri.getData());
    Error::Enum result = mMP4MediaStreamManager->openInput(uri);
    if (result == Error::FILE_NOT_FOUND)
    {
        OMAF_LOG_E("Failed opening the file");
    }
    const MP4VideoStreams& vstreams = mMediaStreamManager->getVideoStreams();

    if (vstreams.getSize() == 0)
    {
        OMAF_LOG_E("Audio only offline clips not supported");
        return Error::NOT_SUPPORTED;
    }

    if (result != Error::OK)
    {
        OMAF_LOG_E("Loading failed: %d", result);
        return result;
    }

    // audios, we should either init audio now properly (unless starving audio won't cause problems), or re-init it when
    // an overlay starts
    MP4AudioStreams main, additional;
    mMediaStreamManager->getAudioStreams(main, additional);
    initializeAudioFromMP4(main, additional);

    mMediaInformation = mMP4MediaStreamManager->getMediaInformation();
    mMediaInformation.streamType = StreamType::LOCAL_FILE;
    const CoreProviderSources& sources = mMP4MediaStreamManager->getVideoSources();
    if (sources.getSize() == 1)
    {
        // Only a single source so must be monoscopic
        mMediaInformation.isStereoscopic = false;
    }
    else if (sources.getSize() == 2)
    {
        // Two sources, stereoscopic
        mMediaInformation.isStereoscopic = true;
    }
    else if (sources.getSize() > 2)
    {
        // More than two sources, should be stereo
        mMediaInformation.isStereoscopic = true;
    }
    MP4VideoStream* videoStream = *vstreams.begin();
    MP4VRMediaPacket* firstPacket = mMediaStreamManager->getNextVideoFrame(*videoStream, 0);
    if (firstPacket != OMAF_NULL)
    {
        mFirstFramePTSUs = firstPacket->presentationTimeUs();
    }

    return result;
}

const CoreProviderSources& MP4VRProvider::getAllSources() const
{
    return mMP4MediaStreamManager->getAllVideoSources();
}

uint64_t MP4VRProvider::selectSources(HeadTransform headTransform,
                                      float32_t fovHorizontal,
                                      float32_t fovVertical,
                                      CoreProviderSources& required,
                                      CoreProviderSources& optional)
{
    required.clear();
    optional.clear();
    for (MP4VideoStream* stream : mMediaStreamManager->getVideoStreams())
    {
        if (stream->getMode() == VideoStreamMode::BACKGROUND)
        {
            required.add(stream->getVideoSources());
        }
        else
        {
            optional.add(stream->getVideoSources());
        }
    }


    return 0;
}

const CoreProviderSourceTypes& MP4VRProvider::getSourceTypes()
{
    return mMediaStreamManager->getVideoSourceTypes();
}

void_t MP4VRProvider::parserThreadCallback()
{
    OMAF_ASSERT(getState() == VideoProviderState::LOADING, "Wrong state");

    Error::Enum result = openFile(mSourceURI);

    if (result != Error::OK)
    {
        setState(VideoProviderState::STREAM_ERROR);
    }
    else
    {
        if (mStreamInitialPositionMS != 0 && mStreamInitialPositionMS <= getMediaInformation().durationUs / 1000)
        {
            uint64_t initialSeekTargetUs = mStreamInitialPositionMS * 1000;
            if (!mMP4MediaStreamManager->seekToUs(initialSeekTargetUs, SeekDirection::PREVIOUS,
                                                  SeekAccuracy::NEAREST_SYNC_FRAME))
            {
                OMAF_LOG_W("Seeking to initial position failed");
            }
            else
            {
                Streams streamIds;
                const MP4VideoStreams& streams = mMediaStreamManager->getVideoStreams();
                for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
                {
                    MP4VideoStream* videoStreamContext = *it;
                    videoStreamContext->resetPackets();
                    streamIds.add(videoStreamContext->getStreamId());
                }
                mFirstDisplayFramePTSUs = initialSeekTargetUs;
                mVideoDecoder->seekToPTS(streamIds, mFirstDisplayFramePTSUs);
            }
            mStreamInitialPositionMS = 0;
        }
        else
        {
            mFirstDisplayFramePTSUs = mFirstFramePTSUs;
        }

        setState(VideoProviderState::LOADED);

        retrieveInitialViewingOrientation(0);
        bool_t viewpointSwitchStarted = false;

        while (1)
        {
            VideoProviderState::Enum stateBeforeUserAction = getState();

            if (mPendingUserAction != PendingUserAction::INVALID)
            {
                handlePendingUserRequest();
            }

            bool_t playPending = false;
            if (getState() == VideoProviderState::BUFFERING && stateBeforeUserAction != VideoProviderState::PLAYING)
            {
                playPending = true;
            }

            if (((!mAudioUsed || mAudioFull) && mVideoDecoderFull) ||
                !(getState() == VideoProviderState::PLAYING || getState() == VideoProviderState::BUFFERING ||
                  getState() == VideoProviderState::SWITCHING_VIEWPOINT))
            {
#if OMAF_PLATFORM_ANDROID
                if (!mParserThreadControlEvent.wait(20))
                {
                    // OMAF_LOG_D("Event timeout");
                }
#else
                Thread::sleep(0);
#endif
            }

            if (getState() == VideoProviderState::STOPPED || getState() == VideoProviderState::CLOSING ||
                getState() == VideoProviderState::STREAM_ERROR)
            {
                break;
            }
            else if (mSeekTargetUs != OMAF_UINT64_MAX || mSeekTargetFrame != OMAF_UINT64_MAX)
            {
                VideoProviderState::Enum stateBeforeSeek = getState();

                doSeek();
                if (stateBeforeSeek == VideoProviderState::PLAYING)
                {
                    playPending = true;
                }
            }
            else if (getState() == VideoProviderState::PAUSED || getState() == VideoProviderState::END_OF_FILE)
            {
                // Not seeking and paused or EoF, no need to parse more packets
                continue;
            }
            else if (mSwitchViewpoint)
            {
                // We need to do the switch via state outside the handlePendingUserRequest to let the renderer thread do
                // its work in parallel.
                // mNextViewpointId contains the id, now we just double-check that the id is valid
                if (mMP4MediaStreamManager->setViewpoint(mNextViewpointId) == Error::OK)
                {
                    viewpointSwitchStarted = true;
                    setState(VideoProviderState::SWITCHING_VIEWPOINT);
                }
                mSwitchViewpoint = false;
            }

            bool_t buffering = true;
            bool_t endOfFile = false;
            bool_t waitForAudio = false;

            VideoProviderState::Enum stateBeforeBuffering = getState();

            while (buffering)
            {
                if (viewpointSwitchStarted)
                {
                    Spinlock::ScopeLock lock(mStreamLock);
                    if (mMP4MediaStreamManager->isViewpointSwitchReadyToComplete(
                            (uint32_t)(mSynchronizer.getElapsedTimeUs() / 1000)))
                    {
                        // mPreviousStreams must not be cleared in mp4 case, since streams are not deleted when
                        // switching viewpoint
                        // viewpoint if we want to continue playing the current viewpoint until the new viewpoint is
                        // ready)
                        enter();
                        bool_t backgroundAudioContinues = false;
                        mMP4MediaStreamManager->completeViewpointSwitch(backgroundAudioContinues);

                        mPreparedSources.clear();
                        leave();
                        viewpointSwitchStarted = false;

                        MP4AudioStreams main, additional;
                        mMediaStreamManager->getAudioStreams(main, additional);
                        if (backgroundAudioContinues)
                        {
                            // just add possible new additional audio streams
                            addAudioStreams(additional);
                        }
                        else
                        {
                            // Re-init the audio subsystem

                            initializeAudioFromMP4(main, additional);
                            if (mAudioUsed)
                            {
                                mAudioInputBuffer->startPlayback();
                            }

                            mSynchronizer.reset(mAudioUsed);
                        }
                        setState(VideoProviderState::PLAYING);
                        stateBeforeBuffering = VideoProviderState::PLAYING;
                    }
                }

                if (mAudioUsed && !mAudioFull)
                {
                    processMP4Audio(mAudioInputBuffer);
                }

                // NOTE: not sure if anything should be done here with return value
                processOverlayMetadataStream();

                PacketProcessingResult::Enum processResult = processMP4Video();

                if (processResult == PacketProcessingResult::BUFFERING)
                {
                    buffering = true;
                    if (getState() != VideoProviderState::LOADED && getState() != VideoProviderState::BUFFERING)
                    {
                        setState(VideoProviderState::BUFFERING);
                        waitForAudio = true;
                    }
                }
                else if (processResult == PacketProcessingResult::END_OF_FILE)
                {
                    stopPlayback();
                    setState(VideoProviderState::END_OF_FILE);
                    break;
                }
                else if (processResult == PacketProcessingResult::ERROR)
                {
                    setState(VideoProviderState::STREAM_ERROR);
                    break;
                }
                else
                {
                    buffering = false;
                }

                if (mPendingUserAction != PendingUserAction::INVALID || mSeekTargetUs != OMAF_UINT64_MAX ||
                    mSeekTargetFrame != OMAF_UINT64_MAX)
                {
                    break;
                }
            }

            if (mPendingUserAction != PendingUserAction::INVALID || getState() == VideoProviderState::STREAM_ERROR)
            {
                continue;
            }

            if (getState() == VideoProviderState::BUFFERING && stateBeforeBuffering == VideoProviderState::PLAYING)
            {
                playPending = true;
            }

            if (playPending)
            {
                startPlayback(waitForAudio);
                setState(VideoProviderState::PLAYING);
            }
            else if (getState() == VideoProviderState::BUFFERING || getState() == VideoProviderState::LOADED)
            {
                setState(VideoProviderState::PAUSED);
            }
            else if (mOverlayAudioPlayPending)
            {
                OMAF_LOG_D("Start audio playback");
                mOverlayAudioPlayPending = false;
                mAudioInputBuffer->startPlayback();
            }
        }
    }

    // Ensure that there's nothing waiting for an event at this point
    mUserActionSync.signal();
}

MP4AudioStream* MP4VRProvider::getAudioStream()
{
    const MP4AudioStreams& astreams = mMediaStreamManager->getAudioStreams();

    return astreams[0];
}

bool_t MP4VRProvider::isSeekable()
{
    return true;
}

bool_t MP4VRProvider::isSeekableByFrame()
{
    return false;
}


void_t MP4VRProvider::doSeek()
{
    OMAF_ASSERT(mSeekTargetUs != OMAF_UINT64_MAX || mSeekTargetFrame != OMAF_UINT64_MAX, "Do seek called incorrectly");
    // EOF and seek targets are protected by control mutex

    const MP4VideoStreams& videoStreams = mMediaStreamManager->getVideoStreams();
    const MP4AudioStreams& audioStreams = mMediaStreamManager->getAudioStreams();

    Streams streamIds;

    for (MP4VideoStreams::ConstIterator it = videoStreams.begin(); it != videoStreams.end(); ++it)
    {
        streamIds.add((*it)->getStreamId());
    }

    if (!mVideoDecoder->isActive(streamIds))
    {
        mSeekTargetUs = OMAF_UINT64_MAX;
        mSeekTargetFrame = OMAF_UINT64_MAX;
        mSeekResult = Error::INVALID_STATE;
        OMAF_LOG_W("Seeking failed due to stream being inactive");
        mSeekSyncronizeEvent.signal();
        return;
    }

    if (mSeekTargetUs > mMediaInformation.durationUs)
    {
        OMAF_LOG_W("Seek failed, trying to seek outside the clip");
        mSeekTargetUs = OMAF_UINT64_MAX;

        mSeekResult = Error::OPERATION_FAILED;
        mSeekSyncronizeEvent.signal();
        return;
    }
    stopPlayback();

    int64_t seekTargetUs = mSeekTargetUs;
    mSeekTargetUs = OMAF_UINT64_MAX;

    int32_t seekTargetFrame = mSeekTargetFrame;
    mSeekTargetFrame = OMAF_UINT64_MAX;

    uint64_t seekTimeUs = 0;

    if (seekTargetUs >= 0)
    {
        seekTimeUs = seekTargetUs;

        if (!mMP4MediaStreamManager->seekToUs(seekTimeUs, SeekDirection::PREVIOUS, mSeekAccuracy))
        {
            // seek failed, do not touch stream positions
            OMAF_LOG_W("Seeking failed");
            mSeekResult = Error::OPERATION_FAILED;
            mSeekSyncronizeEvent.signal();
            return;
        }

        OMAF_LOG_D("seek to I frame at %lld, target was: %lld", seekTimeUs, seekTargetUs);
        mSynchronizer.reset(mAudioUsed);
    }
    else
    {
        // seek by frame number
        seekTimeUs = OMAF_UINT64_MAX;

        if (!mMP4MediaStreamManager->seekToFrame(seekTargetFrame, seekTimeUs))
        {
            // seek failed, do not touch stream positions
            OMAF_LOG_W("Seeking failed");
            mSeekResult = Error::OPERATION_FAILED;
            mSeekSyncronizeEvent.signal();
            return;
        }

        OMAF_LOG_D("seek to I frame at %lld", seekTimeUs);
        mSynchronizer.reset(mAudioUsed);
    }

    if (mAudioInputBuffer != OMAF_NULL)
    {
        // flush all buffered data
        mAudioInputBuffer->flush();
        mAudioFull = false;
    }

    setState(VideoProviderState::BUFFERING);
    mSeekResultMs = seekTimeUs / 1000;
    mSeekResult = Error::OK;
    mSeekSyncronizeEvent.signal();

    for (MP4VideoStreams::ConstIterator it = videoStreams.begin(); it != videoStreams.end(); ++it)
    {
        MP4VideoStream* videoStreamContext = *it;
        videoStreamContext->resetPackets();
    }

    for (MP4AudioStreams::ConstIterator it = audioStreams.begin(); it != audioStreams.end(); ++it)
    {
        MP4AudioStream* audioStreamContext = *it;
        audioStreamContext->resetPackets();
    }

    mVideoDecoder->seekToPTS(streamIds, seekTimeUs);
    bool_t firstDisplayFrameSet = false;
    while (true)
    {
        bool_t shouldSleep = false;
        bool_t seeking = false;

        if (mMediaStreamManager->getAudioStreams().isEmpty())
        {
            // No audio so just set the sync point to the seek target
            mSynchronizer.setVideoSyncPoint(seekTimeUs);
            OMAF_LOG_V("no audio");
        }
        else
        {
            if (mAudioUsed)
            {
                while (!mAudioFull)
                {
                    processMP4Audio(mAudioInputBuffer);
                }
            }
        }

        if (!firstDisplayFrameSet)
        {
            mFirstDisplayFramePTSUs = seekTimeUs;
            firstDisplayFrameSet = true;
        }

        for (MP4VideoStreams::ConstIterator it = videoStreams.begin(); it != videoStreams.end(); ++it)
        {
            MP4VideoStream* videoStream = *it;
            MP4VRMediaPacket* videoPacket =
                mMediaStreamManager->getNextVideoFrame(*videoStream, mSynchronizer.getElapsedTimeUs());
            streamid_t streamID = videoStream->getStreamId();

            if (videoPacket == OMAF_NULL)
            {
                // notify decoder about end of stream
                mVideoDecoder->setInputEOS(streamID);
            }
            else if (videoPacket->presentationTimeUs() <= seekTimeUs)
            {
                // At this point send only packets that are within our seek range to the decoder
                // Mark that we're still seeking
                seeking = true;

                DecodeResult::Enum decodeResult = mVideoDecoder->decodeMediaPacket(streamID, videoPacket, true);
                if (decodeResult == DecodeResult::PACKET_ACCEPTED)
                {
                    videoStream->popFirstFilledPacket();
                    videoStream->returnEmptyPacket(videoPacket);
                }
                else if (decodeResult == DecodeResult::NOT_READY || decodeResult == DecodeResult::DECODER_FULL)
                {
                    // We have a packet which should be decoded but decoder is full, so we need to sleep a bit
                    shouldSleep = true;
                }
                else if (decodeResult == DecodeResult::DECODER_ERROR)
                {
                    setState(VideoProviderState::STREAM_ERROR);
                    seeking = false;
                    break;
                }
            }
        }

        mVideoDecoder->preloadTexturesForPTS(streamIds, Streams(), Streams(), seekTargetUs);

        // All of the streams have reached the seek target so break
        if (!seeking)
        {
            break;
        }

        // Some of the streams can't feed frames to the decoder so sleep a bit
        if (shouldSleep)
        {
            Thread::sleep(10);
        }
    }

    OMAF_LOG_D("seeking done");
}

// called in client thread
uint64_t MP4VRProvider::durationMs() const
{
    return mMediaInformation.durationUs / 1000;
}

void_t MP4VRProvider::handlePendingUserRequest()
{
    OMAF_ASSERT(mPendingUserAction != PendingUserAction::INVALID, "Incorrect user action");

    Mutex::ScopeLock lock(mPendingUserActionMutex);

    if (mPendingUserAction == PendingUserAction::STOP)
    {
        setState(VideoProviderState::STOPPED);
        stopPlayback();
    }
    else if (mPendingUserAction == PendingUserAction::PAUSE)
    {
        setState(VideoProviderState::PAUSED);
        stopPlayback();
    }
    else if (mPendingUserAction == PendingUserAction::PLAY)
    {
        setState(VideoProviderState::BUFFERING);
    }
    else if (mPendingUserAction == PendingUserAction::CONTROL_OVERLAY)
    {
        handleOverlayControl(mPendingOverlayControl);
    }

    mUserActionSync.signal();
    mPendingUserAction = PendingUserAction::INVALID;
}


OMAF_NS_END
