
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRDeviceInfo.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(DashProvider)

static const uint32_t DOWNLOAD_BUFFERING_WAIT = 100;

DashProvider::DashProvider()
: mAuxiliaryDownloadManager(OMAF_NULL)
{
    mDownloadManager = OMAF_NEW_HEAP(DashDownloadManager);
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

Error::Enum DashProvider::loadAuxiliaryStream(PathName &uri)
{
    if (mAuxiliaryDownloadManager == OMAF_NULL)
    {
        mAuxiliaryDownloadManager = OMAF_NEW_HEAP(DashDownloadManager);

        BasicSourceInfo sourceOverride;
        sourceOverride.sourceDirection = SourceDirection::MONO;
        sourceOverride.sourceType = SourceType::IDENTITY_AUXILIARY;
        mAuxiliaryDownloadManager->disableABR();
        Error::Enum result = mAuxiliaryDownloadManager->init(uri, sourceOverride);
        if (result == Error::OK)
        {
            setAuxliaryState(VideoProviderState::LOADING);
        }
        else
        {
            setAuxliaryState(VideoProviderState::STREAM_ERROR);
        }
        return result;
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashProvider::playAuxiliary()
{
    if (mAuxiliaryDownloadManager != OMAF_NULL && !mAuxiliaryStreamPlaying)
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::PLAY_AUXILIARY;
        mPendingUserActionMutex.unlock();

        if (!mUserActionSync.wait(ASYNC_OPERATION_TIMEOUT))
        {
            OMAF_ASSERT(false, "Auxiliary stream play timed out");
        }

        return Error::OK;
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashProvider::stopAuxiliary()
{
    if (mAuxiliaryDownloadManager != OMAF_NULL)
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::STOP_AUXILIARY;
        mPendingUserActionMutex.unlock();

        if (!mUserActionSync.wait(ASYNC_OPERATION_TIMEOUT))
        {
            OMAF_ASSERT(false, "Auxiliary stream stop timed out");
        }

        return Error::OK;
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashProvider::pauseAuxiliary()
{
    if (mAuxiliaryDownloadManager != OMAF_NULL && mAuxiliaryStreamPlaying)
    {
        mPendingUserActionMutex.lock();
        mPendingUserAction = PendingUserAction::PAUSE_AUXILIARY;
        mPendingUserActionMutex.unlock();

        if (!mUserActionSync.wait(ASYNC_OPERATION_TIMEOUT))
        {
            OMAF_ASSERT(false, "Auxiliary stream pause timed out");
        }

        return Error::OK;
    }
    else
    {
        return Error::INVALID_STATE;
    }
}

Error::Enum DashProvider::seekToMsAuxiliary(uint64_t seekTargetMS)
{
    if (mDownloadManager == OMAF_NULL)
    {
        return Error::INVALID_STATE;
    }
    mSeekTargetUsAuxiliary = seekTargetMS * 1000;
    return Error::OK;
}

uint64_t DashProvider::selectSources(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical, CoreProviderSources &base, CoreProviderSources &enhancement)
{
    float32_t longitude;
    float32_t latitude;
    float32_t roll;
    eulerAngles(headTransform.orientation, longitude, latitude, roll, EulerAxisOrder::YXZ);

    streamid_t baseLayerStreamId = OMAF_UINT8_MAX;

    // does the tile picking, if necessary
    // the result of tile picking is visible in streams
    const MP4VideoStreams& streams = mDownloadManager->selectSources(toDegrees(longitude),
                                         toDegrees(latitude),
                                         toDegrees(roll),
                                         fovHorizontal, fovVertical, baseLayerStreamId);

    base.clear();
    enhancement.clear();

    // Loop through the streams to find out what are required and what optional
    uint64_t validFromPts = 0;
    for (MP4VideoStreams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        MP4VideoStream* videoStream = *it;
        streamid_t streamId = videoStream->getStreamId();

        if (videoStream->getMode() == VideoStreamMode::BASE)
        {
            base.add(videoStream->getVideoSources(validFromPts));
        }
        else if (videoStream->getMode() == VideoStreamMode::ENHANCEMENT_NORMAL)
        {
            enhancement.add(videoStream->getVideoSources());
        }
        else if (videoStream->getMode() == VideoStreamMode::ENHANCEMENT_FAST_FORWARD)
        {
            //OMAF_LOG_D("check if stream %d is in sync", streamId);
            // check if late => set seeking (commonly in the end), if not late => change to REQUIRED
            if (!videoStream->isEoF() && mVideoDecoder->syncStreams(baseLayerStreamId, streamId))
            {
                // streams synced
                OMAF_LOG_D("Stream %d synced", streamId);
                videoStream->setMode(VideoStreamMode::ENHANCEMENT_NORMAL);
                enhancement.add(videoStream->getVideoSources());
            }
            else
            {
                // for other streams this is handled automatically, but skipped streams need to take care of it themselves
                mVideoDecoder->activateDecoder(streamId);
            }

        }
    }
    //OMAF_LOG_D("Rendering: required %zd, optional %zd", base.getSize(), enhancement.getSize());
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
    return mDownloadManager->setInitialViewport(toDegrees(longitude),
        toDegrees(latitude),
        toDegrees(roll),
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
        uint32_t enhCodecs = 0;
        mDownloadManager->getNrRequiredVideoCodecs(baseCodecs, enhCodecs);
        if (baseCodecs > 0)
        {
            mVideoDecoder->createVideoDecoders(mMediaInformation.baseLayerCodec, baseCodecs);
        }
        if (mMediaInformation.videoType == VideoType::VIEW_ADAPTIVE && enhCodecs > 0)
        {
            mVideoDecoder->createVideoDecoders(mMediaInformation.enchancementLayerCodec, enhCodecs);
        }

        setState(VideoProviderState::LOADED);

        bool_t invoRead = false;

        while(1)
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

            if ((mAudioFull && mVideoDecoderFull) || getState() != VideoProviderState::PLAYING)
            {
    #if OMAF_PLATFORM_ANDROID
                if (!mParserThreadControlEvent.wait(20))
                {
                    //OMAF_LOG_D("Event timeout");
                }
    #else
                Thread::sleep(0);
    #endif
            }

            if (getState() == VideoProviderState::STOPPED || getState() == VideoProviderState::CLOSING)
            {
                break;
            }
            else if (getState() == VideoProviderState::PAUSED
                     || getState() == VideoProviderState::END_OF_FILE
                     || getState() == VideoProviderState::LOADED)
            {
                Thread::sleep(20);
                continue;
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
                setState(VideoProviderState::END_OF_FILE);
                stopPlayback();
                mDownloadManager->stopDownload();
                Thread::sleep(20);
                continue;
            }
            else if (downloadManagerState == DashDownloadManagerState::IDLE
                     || downloadManagerState == DashDownloadManagerState::INITIALIZED)
            {
                // DownloadManager hasn't started to download things yet
                // TODO: Is this ever reached?
                OMAF_LOG_D("Segment downloader Not yet loaded successfully");
                Thread::sleep(20);
                continue;
            }

            // If the download is buffering, the state is set to BUFFERING and the method
            // returns once buffering is over or a user action is given
            waitForDownloadBuffering();

            // Buffering can trigger error state or end of stream
            if (getState() == VideoProviderState::STREAM_ERROR ||
                mDownloadManager->getState() == DashDownloadManagerState::END_OF_STREAM ||
                mDownloadManager->getState() == DashDownloadManagerState::STREAM_ERROR)
            {
                continue;
            }

            // There might be a pending user action after or during buffering
            if (mPendingUserAction != PendingUserAction::INVALID)
            {
                continue;
            }
        
            bool_t startPlaybackPending = false;
            bool_t auxiliaryPlayPending = false;
        
            if (getState() == VideoProviderState::BUFFERING)
            {
                startPlaybackPending = true;
            }

            if (mSourceTypes.isEmpty())
            {
                // Lock needed?
                mSourceTypes = mDownloadManager->getVideoSourceTypes();
                OMAF_ASSERT(!mSourceTypes.isEmpty(), "No sourcetypes returned by DownloadManager");
            
                // At this point we can initialize the audio, this should be called only once
                const MP4AudioStreams streams = mDownloadManager->getAudioStreams();
                if (!streams.isEmpty())
                {
                    initializeAudioFromMP4(streams);
                }
                else
                {
                    mAudioInputBuffer->initializeForNoAudio();
                }
            }

            if (mAuxiliaryDownloadManager != OMAF_NULL)
            {
                if (mAuxiliaryPlaybackState == VideoProviderState::LOADING)
                {
                    Error::Enum result = mAuxiliaryDownloadManager->checkInitialize();
                    if (result == Error::OK)
                    {
                        setAuxliaryState(VideoProviderState::LOADED);
                        mAuxiliarySyncPending = true;
                        mAuxiliaryAudioInitPending = true;
                        mMediaInformationAuxiliary = mAuxiliaryDownloadManager->getMediaInformation();
                    }
                    else if (result == Error::NOT_READY)
                    {
                        // Skip
                    }
                    else
                    {
                        DashDownloadManagerState::Enum auxDlManagerState = mAuxiliaryDownloadManager->getState();
                        if (auxDlManagerState == DashDownloadManagerState::STREAM_ERROR)
                        {
                            setAuxliaryState(VideoProviderState::STREAM_ERROR);
                            OMAF_DELETE_HEAP(mAuxiliaryDownloadManager);
                            mAuxiliaryDownloadManager = OMAF_NULL;
                        }
                        else if (auxDlManagerState == DashDownloadManagerState::CONNECTION_ERROR)
                        {
                            setAuxliaryState(VideoProviderState::CONNECTION_ERROR);
                            OMAF_DELETE_HEAP(mAuxiliaryDownloadManager);
                            mAuxiliaryDownloadManager = OMAF_NULL;
                        }
                    }
                }
                else
                {
                    if (mSeekTargetUsAuxiliary != OMAF_UINT64_MAX)
                    {
                        doSeekAuxiliary();
                    }
                    if (mAuxiliaryStreamPlaying)
                    {
                        mAuxiliaryDownloadManager->updateStreams(0);
                        DashDownloadManagerState::Enum auxDlManagerState = mAuxiliaryDownloadManager->getState();
                        if (auxDlManagerState == DashDownloadManagerState::INITIALIZED)
                        {
                            mAuxiliaryDownloadManager->startDownload();
                        }

                        if (mAuxiliaryDownloadManager->isBuffering())
                        {
                            setAuxliaryState(VideoProviderState::BUFFERING);
                        }
                        else if (auxDlManagerState == DashDownloadManagerState::END_OF_STREAM)
                        {
                            setAuxliaryState(VideoProviderState::END_OF_FILE);
                            mAuxiliaryStreamPlaying = false;
                        }
                        else
                        {
                            if (mAuxiliaryPlaybackState != VideoProviderState::PLAYING)
                            {
                                auxiliaryPlayPending = true;
                            }
                        }
                    }
                    else if (mAuxiliaryPlaybackState != VideoProviderState::END_OF_FILE)
                    {
                        setAuxliaryState(VideoProviderState::PAUSED);
                    }
                }

            }

            if (!invoRead)
            {
                invoRead = retrieveInitialViewingOrientation(mDownloadManager, -1); // -1 == read the next (first) available metadata frame
            }

            bool_t buffering = true;
            bool_t endOfFile = false;

            while (buffering)
            {
                MP4StreamManager* auxManager = (mAuxiliaryPlaybackState == VideoProviderState::PLAYING || auxiliaryPlayPending) ? mAuxiliaryDownloadManager : OMAF_NULL;

                bool_t bufferingAudio = false;
                PacketProcessingResult::Enum resultAudio = PacketProcessingResult::OK;
                if (!mAudioFull)
                {
                    resultAudio = processMP4Audio(*mDownloadManager, mAudioInputBuffer);
                }

                if (auxManager != OMAF_NULL)
                {
                    if (mAuxiliaryAudioInitPending)
                    {
                        initializeAuxiliaryAudio(auxManager->getAudioStreams());
                        mAuxiliaryAudioInitPending = false;
                    }
                    resultAudio = processMP4Audio(*auxManager, mAuxiliaryAudioInputBuffer);
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

                PacketProcessingResult::Enum result = processMP4Video(*mDownloadManager, auxManager);

                if (result == PacketProcessingResult::END_OF_FILE)
                {
                    buffering = bufferingAudio;
                }
                else if (result == PacketProcessingResult::BUFFERING)
                {
                    if (getState() != VideoProviderState::BUFFERING)
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
                    // TODO waitForDownloadBuffering should prevent entering this loop, but it doesn't
                    mDownloadManager->updateStreams(mVideoDecoder->getLatestPTS());
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
            if (auxiliaryPlayPending)
            {
                setAuxliaryState(VideoProviderState::PLAYING);
                startAuxiliaryPlayback(false);
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

            // Don't break to also stop the auxiliary stream
        case PendingUserAction::STOP_AUXILIARY:
            if (mAuxiliaryDownloadManager != OMAF_NULL)
            {
                mAuxiliaryStreamPlaying = false;
                mAuxiliarySynchronizer.stopSyncRunning();
                setAuxliaryState(VideoProviderState::STOPPED);
                mAuxiliaryDownloadManager->stopDownload();
                OMAF_DELETE_HEAP(mAuxiliaryDownloadManager);
                mAuxiliaryDownloadManager = OMAF_NULL;
                mAuxiliaryStreamInitialized = false;
                {
                    Spinlock::ScopeLock lock(mAuxiliarySourcesLock);
                    mAuxiliarySources.clear();
                }
                mDownloadManager->setBandwidthOverhead(0);
            }
            break;

        case PendingUserAction::PAUSE:
            setState(VideoProviderState::PAUSED);
            stopPlayback();

            mDownloadManager->stopDownload();

            if (mDownloadManager->isDynamicStream())
            {
                clearDownloadedContent(mDownloadManager);
            }

            // Don't break to also pause the auxiliary stream
        case PendingUserAction::PAUSE_AUXILIARY:
            if (mAuxiliaryDownloadManager != OMAF_NULL)
            {
                setAuxliaryState(VideoProviderState::PAUSED);
                stopAuxiliaryPlayback();
                if (mPendingUserAction == PendingUserAction::PAUSE_AUXILIARY)
                {
                    mAuxiliaryStreamPlaying = false;
                }
                mAuxiliaryDownloadManager->stopDownload();
                if (mAuxiliaryDownloadManager->isDynamicStream())
                {
                     clearDownloadedContent(mAuxiliaryDownloadManager);
                }
                mDownloadManager->setBandwidthOverhead(0);
            }
            break;

        case PendingUserAction::PLAY:
            setState(VideoProviderState::BUFFERING);
            mDownloadManager->startDownload();

        case PendingUserAction::PLAY_AUXILIARY:
            if (!(mPendingUserAction == PendingUserAction::PLAY && !mAuxiliaryStreamPlaying))
            {
                if (mAuxiliaryDownloadManager != OMAF_NULL)
                {
                    if (mAuxiliaryPlaybackState != VideoProviderState::LOADING)
                    {
                        setAuxliaryState(VideoProviderState::BUFFERING);
                        mAuxiliaryDownloadManager->startDownload();
                    }
                    mAuxiliaryStreamPlaying = true;
                    mAuxiliarySynchronizer.setSyncRunning(false);
                    mDownloadManager->setBandwidthOverhead(mAuxiliaryDownloadManager->getCurrentBitrate());
                }
            }
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
        mAudioSyncPending = true;
    }
    else
    {
        for (MP4VideoStreams::ConstIterator it = videostreams.begin(); it != videostreams.end(); ++it)
        {
            mVideoDecoder->flushStream((*it)->getStreamId());
        }
        mAuxiliaryAudioInputBuffer->flush();
        mAuxiliarySyncPending = true;
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
    
    if (getState() != VideoProviderState::PAUSED)
    {
        mDownloadManager->startDownload();
    }
}

void_t DashProvider::doSeekAuxiliary()
{
    if (mAuxiliaryDownloadManager != OMAF_NULL)
    {
        stopAuxiliaryPlayback();
        mAuxiliaryDownloadManager->stopDownload();
        clearDownloadedContent(mAuxiliaryDownloadManager);

        uint64_t seekMs = mSeekTargetUsAuxiliary / 1000;
        Error::Enum seekResult = mAuxiliaryDownloadManager->seekToMs(seekMs);

        mSeekTargetUsAuxiliary = OMAF_UINT64_MAX;

        if (mAuxiliaryPlaybackState != VideoProviderState::PAUSED)
        {
            mAuxiliaryDownloadManager->startDownload();
        }
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
        else
        {
            // See how long we have been waiting so far, complain if too long, and we haven't encountered connection errors yet
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
