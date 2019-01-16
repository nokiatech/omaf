
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
#include "NVRHeifProvider.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(HeifProvider)

HeifProvider::HeifProvider() : mRequestImageChange(false)
{
    mMediaStreamManager = OMAF_NEW_HEAP(HeifMediaStreamManager);
}

HeifProvider::~HeifProvider()
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

    mMediaStreamManager->resetStreams();
    OMAF_DELETE_HEAP(mMediaStreamManager);

}

Error::Enum HeifProvider::openFile(PathName &uri)
{
    OMAF_LOG_D("Loading from uri: %s", uri.getData());
    
    Error::Enum result = mMediaStreamManager->openInput(uri);
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

    mMediaInformation = mMediaStreamManager->getMediaInformation();
    mMediaInformation.streamType = StreamType::LOCAL_FILE;
    const CoreProviderSources& sources = mMediaStreamManager->getVideoSources();
    if (sources.getSize() == 1)
    {
        // Only a single source so must be monoscopic
        mMediaInformation.isStereoscopic = false;
    }
    else if (sources.getSize() > 1)
    {
        // Two or more sources, should be stereoscopic
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

uint64_t HeifProvider::selectSources(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical, CoreProviderSources &required, CoreProviderSources &optional)
{
    // No source picking so all sources are required
    required.clear();
    required.add(mMediaStreamManager->getVideoSources());
    return 0;
}

const CoreProviderSourceTypes& HeifProvider::getSourceTypes()
{
    return mMediaStreamManager->getVideoSourceTypes();
}

void_t HeifProvider::parserThreadCallback()
{
    OMAF_ASSERT(getState() == VideoProviderState::LOADING, "Wrong state");
    
    Error::Enum result = openFile(mSourceURI);

    if (result != Error::OK)
    {
        setState(VideoProviderState::STREAM_ERROR);
    }
    else
    {
        mFirstDisplayFramePTSUs = mFirstFramePTSUs;

        setState(VideoProviderState::LOADED);

        bool_t done = false;
        while (1)
        {
            // HEIF internal state for changing image inside multi image archive
            // done is waited, to be sure that loading earlier image was ready
            if (done && mRequestImageChange) {
                enter(); // signal that we are now going to change contents of provider
                auto oldState = getState();
                setState(VideoProviderState::LOADING);

                // cleanup decoder from old streams and request next image
                OMAF_LOG_V("----> Clearing previous streams");
                mPreviousStreams.clear();
                OMAF_LOG_V("----> Clearing prepared sources");
                mPreparedSources.clear();
                OMAF_LOG_V("----> Previous streams clear, selecting next image");
                mMediaStreamManager->selectNextImage();
                OMAF_LOG_V("----> Next image changed from media stream manager");

                leave(); // free provider again for reading
                setState(oldState);
                mRequestImageChange = false;
                done = false; // start showing frames again
            }

            VideoProviderState::Enum stateBeforeUserAction = getState();

            if (mPendingUserAction != PendingUserAction::INVALID)
            {
                handlePendingUserRequest();
            }

            if (getState() == VideoProviderState::STOPPED
                || getState() == VideoProviderState::CLOSING
                || getState() == VideoProviderState::STREAM_ERROR)
            {
                break;
            }
            else if (mVideoDecoderFull)
            {
#if OMAF_PLATFORM_ANDROID || OMAF_PLATFORM_IOS
                if (!mParserThreadControlEvent.wait(20))
                {
                    //OMAF_LOG_D("Event timeout");
                }
#else
                Thread::sleep(20);
#endif
            }
            else if (done)
            {
                // EoF, no need to parse more packets
                continue;
            }

            PacketProcessingResult::Enum processResult = processMP4Video(*mMediaStreamManager);

            if (processResult == PacketProcessingResult::END_OF_FILE)
            {
                // shouldn't change the state to EoF as it would close the provider
                // and setting state to paused would show the controls on top of the image
                done = true;
            }
            else if (processResult == PacketProcessingResult::ERROR)
            {
                setState(VideoProviderState::STREAM_ERROR);
                break;
            }

        }
    }

    // Ensure that there's nothing waiting for an event at this point
    mUserActionSync.signal();
}

bool_t HeifProvider::isSeekable()
{
    return false;
}

bool_t HeifProvider::isSeekableByFrame()
{
    return false;
}

MP4AudioStream* HeifProvider::getAudioStream()
{
    const MP4AudioStreams& astreams = mMediaStreamManager->getAudioStreams();

    return astreams[0];
}

// called in client thread
uint64_t HeifProvider::durationMs() const
{
    return mMediaInformation.duration / 1000;
}

Error::Enum HeifProvider::next() {
    mRequestImageChange = true;
    return Error::OK;
}

void_t HeifProvider::handlePendingUserRequest()
{
    OMAF_ASSERT(mPendingUserAction != PendingUserAction::INVALID, "Incorrect user action");

    Mutex::ScopeLock lock(mPendingUserActionMutex);

    if (mPendingUserAction == PendingUserAction::STOP)
    {
        setState(VideoProviderState::STOPPED);
    }
    else if (mPendingUserAction == PendingUserAction::PAUSE)
    {
        setState(VideoProviderState::PAUSED);
    }
    else if (mPendingUserAction == PendingUserAction::PLAY)
    {
        setState(VideoProviderState::PLAYING);
    }

    mUserActionSync.signal();
    mPendingUserAction = PendingUserAction::INVALID;
}

OMAF_NS_END
