
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
#include "DashProvider/NVRDashProvider.h"
#include "DashProvider/NVRDashDownloadManager.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(DashProvider)

static const uint32_t DOWNLOAD_BUFFERING_WAIT = 100;

DashProvider::DashProvider()
{
    mDownloadManager = OMAF_NEW_HEAP(DashDownloadManager);
    mMediaStreamManager = mDownloadManager;  // base class reference for common operations
}

DashProvider::~DashProvider()
{
    setState(VideoProviderState::CLOSING);

    if (mParserThread.isValid())
    {
        mParserThread.stop();
        mParserThread.join();
    }

    stopPlayback();

    mDownloadManager->stopDownload();
    mDownloadManager->clearDownloadedContent();
    destroyInstance();
    OMAF_DELETE_HEAP(mDownloadManager);

    // then calls ~ProviderBase
}

const CoreProviderSourceTypes& DashProvider::getSourceTypes()
{
    return mSourceTypes;
}

const CoreProviderSources& DashProvider::getAllSources() const
{
    return mDownloadManager->getAllSources();
}

uint64_t DashProvider::selectSources(HeadTransform headTransform,
                                     float32_t fovHorizontal,
                                     float32_t fovVertical,
                                     CoreProviderSources& base,
                                     CoreProviderSources& additional)
{
    float32_t longitude;
    float32_t latitude;
    float32_t roll;
    eulerAngles(headTransform.orientation, longitude, latitude, roll, EulerAxisOrder::YXZ);

#if OMAF_ABR_LOGGING
    OMAF_LOG_ABR("TRACKING %d %f %f %f", mSynchronizer.getElapsedTimeUs() / 1000, toDegrees(longitude),
                 toDegrees(latitude), toDegrees(roll));
#endif

    // does the tile picking, if necessary
    // the result of tile picking is visible in streams
    const MP4VideoStreams& streams = mDownloadManager->selectSources(toDegrees(longitude), toDegrees(latitude),
                                                                     toDegrees(roll), fovHorizontal, fovVertical);

    base.clear();
    additional.clear();

    // Loop through the streams to find out what are required and what optional
    uint64_t validFromPts = 0;
    for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        MP4VideoStream* videoStream = *it;

        if (videoStream->getMode() == VideoStreamMode::BACKGROUND)
        {
            base.add(videoStream->getVideoSources(validFromPts));
        }
        else if (videoStream->getMode() == VideoStreamMode::OVERLAY)
        {
            additional.add(videoStream->getVideoSources());
        }
    }
    // OMAF_LOG_D("Rendering: required %zd, optional %zd", base.getSize(), additional.getSize());
    return validFromPts;
}

bool_t DashProvider::setInitialViewport(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical)
{
    if (mDownloadManager->getState() == DashDownloadManagerState::IDLE)
    {
        // too early
        return false;
    }
    float32_t longitude;
    float32_t latitude;
    float32_t roll;
    eulerAngles(headTransform.orientation, longitude, latitude, roll, EulerAxisOrder::YXZ);

    streamid_t baseLayerStreamId = OMAF_UINT8_MAX;

    // does the initial tile picking
    return mDownloadManager->setInitialViewport(toDegrees(longitude), toDegrees(latitude), toDegrees(roll),
                                                fovHorizontal, fovVertical);
}

void_t DashProvider::parserThreadCallback()
{
    OMAF_ASSERT(getState() == VideoProviderState::LOADING, "Incorrect state");
    BasicSourceInfo sourceOverride;
    Error::Enum error = mDownloadManager->init(mSourceURI, sourceOverride);

    if (error == Error::OK)
    {
        error = mDownloadManager->waitForInitializeCompletion();
    }

    if (error != Error::OK)
    {
        OMAF_LOG_D("DownloadManager error: %d ", error);
        if (error == Error::FILE_NOT_FOUND)
        {
            setState(VideoProviderState::STREAM_NOT_FOUND);
        }
        else if (error == Error::NOT_SUPPORTED)
        {
            setState(VideoProviderState::STREAM_ERROR);
        }
        else
        {
            setState(VideoProviderState::CONNECTION_ERROR);
        }
    }
    else
    {
        mMediaInformation = mDownloadManager->getMediaInformation();

        if (mStreamInitialPositionMS != 0)
        {
            mDownloadManager->seekToMs(mStreamInitialPositionMS);
            mStreamInitialPositionMS = 0;
        }

        uint32_t baseCodecs = 0;
        uint32_t addCodecs = 0;
        mDownloadManager->getNrRequiredVideoCodecs(baseCodecs, addCodecs);
        if (baseCodecs > 0)
        {
            mVideoDecoder->createVideoDecoders(mMediaInformation.baseLayerCodec, baseCodecs);
        }
        if (addCodecs > 0)
        {
            mVideoDecoder->createVideoDecoders(mMediaInformation.enchancementLayerCodec, addCodecs);
        }

        setState(VideoProviderState::LOADED);

        bool_t invoRead = false;
        bool_t viewpointSwitchStarted = false;

        while (1)
        {
            if (mSeekTargetUs != OMAF_UINT64_MAX)
            {
                doSeek();
            }

            // User control event pending
            if (mPendingUserAction != PendingUserAction::INVALID)
            {
                handlePendingUserRequest();
            }
            auto providerState = getState();

            if (((!mAudioUsed || mAudioFull) && mVideoDecoderFull) ||
                !(providerState == VideoProviderState::PLAYING || providerState == VideoProviderState::BUFFERING ||
                  providerState == VideoProviderState::SWITCHING_VIEWPOINT))
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

            if (providerState == VideoProviderState::STOPPED || providerState == VideoProviderState::CLOSING)
            {
                break;
            }
            else if (providerState == VideoProviderState::PAUSED ||
                     providerState == VideoProviderState::END_OF_FILE ||
                     providerState == VideoProviderState::LOADED)
            {
                Thread::sleep(20);
                continue;
            }
            else if (mSwitchViewpoint)
            {
                // We need to do the switch via state outside the handlePendingUserRequest to let the renderer thread do
                // its work in parallel. In this case it needs to set the viewport to tile picker before we can start
                // the new viewpoint. There is sync for that in the viewpoint initialization
                if (mDownloadManager->setViewpoint(mNextViewpointId) == Error::OK)
                {
                    viewpointSwitchStarted = true;
                    // the viewport is required when starting a new viewpoint download. This is
                    // DASH-specific requirement
                    mSignalViewport = true;
                    setState(VideoProviderState::SWITCHING_VIEWPOINT);
                }
                mSwitchViewpoint = false;
            }

            mDownloadManager->updateStreams(mVideoDecoder->getLatestPTS());
            DashDownloadManagerState::Enum downloadManagerState = mDownloadManager->getState();

            if (downloadManagerState == DashDownloadManagerState::STREAM_ERROR)
            {
                // Downloader in error state
                setState(VideoProviderState::STREAM_ERROR);
                break;
            }
            else if (downloadManagerState == DashDownloadManagerState::CONNECTION_ERROR)
            {
                setState(VideoProviderState::CONNECTION_ERROR);
                break;
            }
            else if (downloadManagerState == DashDownloadManagerState::END_OF_STREAM)
            {
                mDownloadManager->stopDownload();
            }
            else if (downloadManagerState == DashDownloadManagerState::IDLE ||
                     downloadManagerState == DashDownloadManagerState::INITIALIZED)
            {
                // DownloadManager hasn't started to download things yet
                OMAF_LOG_D("Segment downloader Not yet loaded successfully");
                Thread::sleep(20);
                continue;
            }

            if (!viewpointSwitchStarted)
            {
                // If the download is buffering, the state is set to BUFFERING and the method
                // returns once buffering is over or a user action is given
                waitForDownloadBuffering();
            }

            // Buffering can trigger error state or end of stream
            if (getState() == VideoProviderState::STREAM_ERROR ||
                mDownloadManager->getState() == DashDownloadManagerState::STREAM_ERROR)
            {
                continue;
            }

            // There might be a pending user action after or during buffering
            if (mPendingUserAction != PendingUserAction::INVALID)
            {
                continue;
            }
            // Downloadmanager must have finished buffering by now
            // (the waitForDownloadBuffering exits only if there is user action, and if
            // there is, it loops back above and then continues waiting)?
            // So we can start processing the data only after buffering has finished

            bool_t startPlaybackPending = false;

            if (getState() == VideoProviderState::BUFFERING)
            {
                startPlaybackPending = true;
            }

            mDownloadManager->processSegments();
            if (mSourceTypes.isEmpty())
            {
                while (mDownloadManager->notReadyToGo())
                {
                    // just in case, loop until we have enough data processed to have video sources set up
                    mDownloadManager->updateStreams(mVideoDecoder->getLatestPTS());
                    mDownloadManager->processSegments();
                }
                // Lock needed?
                mSourceTypes = mDownloadManager->getVideoSourceTypes();
                OMAF_ASSERT(!mSourceTypes.isEmpty(), "No sourcetypes returned by DownloadManager");
                // At this point we can initialize the audio, this should be called only once per switch
                MP4AudioStreams main, additional;
                mDownloadManager->getAudioStreams(main, additional);
                initializeAudioFromMP4(main, additional);
            }

            if (!invoRead)
            {
                invoRead =
                    retrieveInitialViewingOrientation(-1);  // -1 == read the next (first) available metadata frame
            }

            bool_t buffering = true;
            bool_t endOfFile = false;

            while (buffering)
            {
                if (viewpointSwitchStarted)
                {
                    if (mSignalViewport && mDownloadManager->isViewpointSwitchInitiated())
                    {
                        mSignalViewport = false;  // now the download was started, no need to signal any more
                    }
                    if (mDownloadManager->isViewpointSwitchReadyToComplete(
                            (uint32_t)(mSynchronizer.getElapsedTimeUs() / 1000)))
                    {
                        // mStreamLock is more specific than enter-leave, and is used in provider side in more
                        // occassions than enter-leave In renderer thread it is used inside prepareSources
                        Spinlock::ScopeLock lock(mStreamLock);
                        enter();
                        bool_t backgroundAudioContinues = false;
                        mDownloadManager->completeViewpointSwitch(backgroundAudioContinues);
                        // clear provider-level stream id list, so that stream activation works for the new streams
                        mPreviousStreams.clear();
                        OMAF_LOG_V("Viewpoint switched, clear preparedSources");
                        mPreparedSources.clear();
                        leave();
                        viewpointSwitchStarted = false;

                        MP4AudioStreams main, additional;
                        mDownloadManager->getAudioStreams(main, additional);
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
                            // Resetting synchronizer reduces the possibility to be forced to skip some frames
                            mSynchronizer.reset(mAudioUsed);
                        }
                        setState(VideoProviderState::PLAYING);
                    }
                }

                bool_t bufferingAudio = false;
                PacketProcessingResult::Enum resultAudio = PacketProcessingResult::OK;
                if (!mAudioFull)
                {
                    resultAudio = processMP4Audio(mAudioInputBuffer);
                }

                if (resultAudio == PacketProcessingResult::BUFFERING)
                {
                    if (getState() != VideoProviderState::BUFFERING)
                    {
                        stopPlayback();
                        setState(VideoProviderState::BUFFERING);
                        startPlaybackPending = true;
                        buffering = true;
                    }
                }

                // NOTE: not sure if anything should be done here with return value
                processOverlayMetadataStream();

                PacketProcessingResult::Enum result = processMP4Video();

                if (result == PacketProcessingResult::END_OF_FILE)
                {
                    buffering = bufferingAudio;
                    OMAF_LOG_D("Last video packet read, set state to end of file");
                    // Note: we could get a few more frames rendered by waiting until the decoder EOS is done, but
                    // requires some more changes
                    setState(VideoProviderState::END_OF_FILE);
                    stopPlayback();
                    break;
                }
                else if (result == PacketProcessingResult::BUFFERING)
                {
                    if (viewpointSwitchStarted)
                    {
                        buffering = true;
                    }
                    else if (getState() != VideoProviderState::BUFFERING)
                    {
                        stopPlayback();
                        setState(VideoProviderState::BUFFERING);
                        startPlaybackPending = true;
                        buffering = true;
                    }
                }
                else if (result == PacketProcessingResult::ERROR)
                {
                    setState(VideoProviderState::STREAM_ERROR);
                    buffering = false;
                    break;
                }
                else
                {
                    buffering = bufferingAudio;
                }
                if (buffering)
                {
                    mDownloadManager->updateStreams(mVideoDecoder->getLatestPTS());
                    mDownloadManager->processSegments();
                }
                if (mPendingUserAction != PendingUserAction::INVALID)
                {
                    break;
                }
            }
            if (mPendingUserAction != PendingUserAction::INVALID || getState() == VideoProviderState::STREAM_ERROR)
            {
                continue;
            }

            if (startPlaybackPending)
            {
                setState(VideoProviderState::PLAYING);
                startPlayback(false);
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

void_t DashProvider::handlePendingUserRequest()
{
    OMAF_ASSERT(mPendingUserAction != PendingUserAction::INVALID, "Incorrect user action");

    switch (mPendingUserAction)
    {
    case PendingUserAction::STOP:
        setState(VideoProviderState::STOPPED);
        stopPlayback();
        mDownloadManager->stopDownload();
        break;

    case PendingUserAction::PAUSE:
        setState(VideoProviderState::PAUSED);
        stopPlayback();

        mDownloadManager->stopDownload();

        if (mDownloadManager->isDynamicStream())
        {
            clearDownloadedContent(mDownloadManager);
        }
        break;

    case PendingUserAction::PLAY:
        setState(VideoProviderState::BUFFERING);
        mDownloadManager->startDownload();
        break;
    case PendingUserAction::CONTROL_OVERLAY:
        handleOverlayControl(mPendingOverlayControl);
        break;
    }

    mUserActionSync.signal();
    mPendingUserAction = PendingUserAction::INVALID;
}

void_t DashProvider::clearDownloadedContent(DashDownloadManager* dlManager)
{
    // Clear existing buffers
    dlManager->clearDownloadedContent();

    const MP4VideoStreams& videostreams = dlManager->getVideoStreams();
    for (MP4VideoStreams::ConstIterator it = videostreams.begin(); it != videostreams.end(); ++it)
    {
        MP4VideoStream* videoStreamContext = *it;
        videoStreamContext->resetPackets();
    }

    const MP4AudioStreams& audioStreams = dlManager->getAudioStreams();
    for (MP4AudioStreams::ConstIterator it = audioStreams.begin(); it != audioStreams.end(); ++it)
    {
        MP4AudioStream* audioStreamContext = *it;
        audioStreamContext->resetPackets();
    }

    // And flush video and audio
    if (dlManager == mDownloadManager)
    {
        for (Streams::ConstIterator it = mPreviousStreams.begin(); it != mPreviousStreams.end(); ++it)
        {
            mVideoDecoder->flushStream(*it);
        }

        mAudioInputBuffer->flush();
        mAudioFull = false;
        mSynchronizer.reset(mAudioUsed);
    }
    else
    {
        for (MP4VideoStreams::ConstIterator it = videostreams.begin(); it != videostreams.end(); ++it)
        {
            mVideoDecoder->flushStream((*it)->getStreamId());
        }
    }
}

MP4AudioStream* DashProvider::getAudioStream()
{
    return mDownloadManager->getAudioStreams()[0];
}

bool_t DashProvider::isSeekable()
{
    return mDownloadManager->isSeekable();
}

bool_t DashProvider::isSeekableByFrame()
{
    return false;
}

void_t DashProvider::doSeek()
{
    mDownloadManager->stopDownload();
    clearDownloadedContent(mDownloadManager);

    uint64_t seekMs = mSeekTargetUs / 1000;
    mSeekResult = mDownloadManager->seekToMs(seekMs);
    if (mSeekResult == Error::OK)
    {
        mSeekResultMs = seekMs;
    }
    mSeekTargetUs = OMAF_UINT64_MAX;
    mSeekSyncronizeEvent.signal();

    // if (getState() != VideoProviderState::PAUSED)
    {
        mDownloadManager->startDownload();
    }
}

void_t DashProvider::waitForDownloadBuffering()
{
    int32_t bufferingStartTime = Time::getClockTimeMs();
    int32_t lastNotifyTime = bufferingStartTime;
    while (mDownloadManager->isBuffering())
    {
        mDownloadManager->updateStreams(mVideoDecoder->getLatestPTS());
        if (!mDownloadManager->isBuffering())
        {
            break;
        }
        if (mDownloadManager->getState() == DashDownloadManagerState::STREAM_ERROR)
        {
            setState(VideoProviderState::STREAM_ERROR);
            break;
        }
        if (getState() == VideoProviderState::PLAYING)
        {
            setState(VideoProviderState::BUFFERING);
            stopPlayback();
            if (mDownloadManager->isDynamicStream())
            {
                mDownloadManager->stopDownload();
                clearDownloadedContent(mDownloadManager);
                mDownloadManager->startDownload();
            }
        }
        else if (getState() == VideoProviderState::SWITCHING_VIEWPOINT)
        {
            // not sure if we ever end up here in this state, but at least we should get out as the state can switch
            // back to playing only in the main loop.
            // must not allow SWITCHING_VIEWPOINT, as setting state to buffering will break the switch
            return;
        }
        else
        {
            // See how long we have been waiting so far, complain if too long, and we haven't encountered connection
            // errors yet
            int32_t now = Time::getClockTimeMs();
            int32_t timeSinceStart = now - bufferingStartTime;
            int32_t timeSinceLastNotify = now - lastNotifyTime;

            if (timeSinceStart > 2000 && timeSinceLastNotify > 1000)
            {
                OMAF_LOG_D("Paused for buffering for %d ms", timeSinceStart);
                lastNotifyTime = now;
            }
        }

        if (mPendingUserAction != PendingUserAction::INVALID)
        {
            break;
        }

        Thread::sleep(DOWNLOAD_BUFFERING_WAIT);
    }
}

uint64_t DashProvider::durationMs() const
{
    return mDownloadManager->durationMs();
}


OMAF_NS_END
