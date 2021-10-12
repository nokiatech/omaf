
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
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "VideoDecoder/NVRVideoDecoderHW.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

#if OMAF_VIDEO_DECODER_NULL
#include "VideoDecoder/Null/NVRVideoDecoderNull.h"
#elif OMAF_PLATFORM_ANDROID
#include "VideoDecoder/Android/NVRMediaCodecDecoder.h"
#elif OMAF_PLATFORM_WINDOWS
#include "VideoDecoder/Windows/NVRMediaFoundationDecoder.h"
#elif OMAF_PLATFORM_UWP
#include "VideoDecoder/UWP/NVRMediaFoundationDecoder.h"
#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(VideoDecoderManager);

VideoDecoderManager* VideoDecoderManager::sInstance = OMAF_NULL;

VideoDecoderManager* VideoDecoderManager::getInstance()
{
    if (sInstance == OMAF_NULL)
    {
#if OMAF_VIDEO_DECODER_NULL
//    sInstance = VideoDecoderNull::createDecoder(format);
#else
#if OMAF_PLATFORM_ANDROID
        sInstance = OMAF_NEW_HEAP(MediaCodecDecoder);
#elif OMAF_PLATFORM_WINDOWS
        sInstance = OMAF_NEW_HEAP(MediaFoundationDecoder);
#elif OMAF_PLATFORM_UWP
        sInstance = OMAF_NEW_HEAP(MediaFoundationDecoder);
#else
        sInstance = OMAF_NEW_HEAP(VideoDecoderLibAV);
#endif
#endif
    }
    return sInstance;
}

void_t VideoDecoderManager::destroyInstance()
{
    OMAF_DELETE_HEAP(sInstance);
    sInstance = OMAF_NULL;
}

VideoDecoderManager::VideoDecoderManager()
    : mByteStreamHeadersMode(true)
    , mSharedStreamID(InvalidStreamId)
    , mPrevSharedStreamID(InvalidStreamId)
    , mLatestPts(0)
{
    while (mDecoders.getSize() != MAX_STREAM_COUNT)
    {
        mDecoders.add(DecoderState());
    }
    mFrameCache = FrameCache::createFrameCache();
}

VideoDecoderManager::~VideoDecoderManager()
{
    for (DecoderStateList::Iterator it = mDecoders.begin(); it != mDecoders.end(); ++it)
    {
        if ((*it).decoderHW != OMAF_NULL)
        {
            OMAF_DELETE_HEAP((*it).decoderHW);
        }
    }
    OMAF_DELETE_HEAP(mFrameCache);
}

Error::Enum VideoDecoderManager::initializeStream(streamid_t stream, const DecoderConfig& config)
{
    if ((stream == mSharedStreamID || stream == mPrevSharedStreamID) && mDecoders.at(stream).decoderHW != OMAF_NULL)
    {
        return Error::OK_SKIPPED;
    }
    OMAF_ASSERT(mDecoders.at(stream).decoderHW == OMAF_NULL, "Decoder for stream already created");
    mDecoders.at(stream).config = config;
    mFrameCache->initializeStream(stream, config);
    mDecoders.at(stream).free = false;
    return Error::OK;
}

void_t VideoDecoderManager::shutdownStream(streamid_t stream)
{
    if (mDecoders.at(stream).decoderActive)
    {
        deactivateStream(stream);
    }
    mFrameCache->shutdownStream(stream);
    mFrameCache->destroyTexture(stream);
    mDecoders.at(stream).textureActive = false;
    mDecoders.at(stream).free = true;
    OMAF_LOG_V("shutdownStream %d", stream);
}

Error::Enum VideoDecoderManager::activateStream(streamid_t stream, uint64_t currentPTS)
{
    if (mDecoders.at(stream).decoderActive)
    {
        return Error::OK;
    }
    OMAF_ASSERT(mDecoders.at(stream).config.streamId != OMAF_UINT8_MAX, "Invalid stream ID in config");
    uint32_t start = Time::getClockTimeMs();
    VideoDecoderHW* decoder = reserveVideoDecoder(mDecoders.at(stream).config);
    if (decoder == OMAF_NULL)
    {
        OMAF_LOG_E("Video decoder reserve failed");
        OMAF_DELETE_HEAP(decoder);
        return Error::OPERATION_FAILED;
    }
    mDecoders.at(stream).decoderHW = decoder;
    mDecoders.at(stream).decoderActive = true;
    // Make sure that older than current frames are discarded
    mFrameCache->setDiscardTarget(stream, currentPTS);
    OMAF_LOG_V("activateStream took: %d", Time::getClockTimeMs() - start);
    return Error::OK;
}

void_t VideoDecoderManager::deactivateStream(streamid_t stream)
{
    uint32_t start = Time::getClockTimeMs();
    OMAF_ASSERT(mDecoders.at(stream).decoderHW != OMAF_NULL, "No decoder for stream");
    mDecoders.at(stream).decoderActive = false;
    if (mDecoders.at(stream).frameCacheActive)
    {
        mDecoders.at(stream).frameCacheActive = false;
// If the frame cache wasn't activated, then the decoder hasn't been fed any packets
// so decoder flushing is needed only when frame cache has been active

// Android and Windows decoders don't store FrameCache frames internally between decode calls
// so the FrameCache should be flushed first to avoid error on Android
#if OMAF_PLATFORM_ANDROID || OMAF_PLATFORM_WINDOWS
        mFrameCache->deactivateStream(stream);
        mDecoders.at(stream).decoderHW->flush();
#else
        // iOS and MacOS still store FrameCache frames in the decoder so flush the decoder first
        mDecoders.at(stream).decoderHW->flush();
        mFrameCache->deactivateStream(stream);
#endif
    }
    releaseVideoDecoder(mDecoders.at(stream).decoderHW);
    mDecoders.at(stream).decoderHW = OMAF_NULL;
    OMAF_LOG_V("doDeactivateStream took: %d", Time::getClockTimeMs() - start);
}

bool_t VideoDecoderManager::isActive(const Streams& streams)
{
    for (Streams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        DecoderState& decoder = mDecoders.at(*it);
        if (!decoder.frameCacheActive || !decoder.textureActive || !decoder.frameCacheActive)
        {
            return false;
        }
    }
    return true;
}

DecodeResult::Enum VideoDecoderManager::decodeMediaPacket(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking)
{
    OMAF_ASSERT(mDecoders.at(stream).decoderHW != OMAF_NULL, "No decoder for stream");
    // Frame cache textures need to be active before feeding in packets
    if (!mDecoders.at(stream).frameCacheActive)
    {
        return DecodeResult::NOT_READY;
    }
    else
    {
        return mDecoders.at(stream).decoderHW->decodeFrame(stream, packet, seeking);
    }
}

void_t VideoDecoderManager::seekToPTS(const Streams& streams, uint64_t seekTargetPTS)
{
    for (Streams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        mFrameCache->setDiscardTarget(*it, seekTargetPTS);
        flushStream(*it);
    }
}


bool_t VideoDecoderManager::isBuffering(const Streams& streams)
{
    bool_t buffering = false;

    for (Streams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        bool streamBuffering = false;
        streamid_t stream = (*it);
        DecoderState& decoderState = mDecoders.at(stream);
        OMAF_ASSERT(decoderState.decoderHW != OMAF_NULL, "No decoder for stream");
        // First check decoder EoS, don't buffer in EoS
        if (decoderState.decoderHW->isEOS())
        {
            streamBuffering = false;
        }
        else
        {
            size_t decodedFrameCount = mFrameCache->getDecodedFrameCount(stream);
            if (!decoderState.initialBufferingDone)
            {
                streamBuffering = decodedFrameCount < mInitialBufferingThreshold;
                if (!streamBuffering)
                {
                    decoderState.initialBufferingDone = true;
                }
            }
            else
            {
                streamBuffering = false;
            }
        }
        if (streamBuffering)
        {
            buffering = true;
            break;
        }
    }
    return buffering;
}

void_t VideoDecoderManager::flushStream(streamid_t stream)
{
    if (mDecoders.at(stream).decoderHW != OMAF_NULL)
    {
        mDecoders.at(stream).decoderHW->flush();
    }
    // mDecoders.at(stream).initialBufferingDone = false;
    mFrameCache->flushFrames(stream);
}

bool_t VideoDecoderManager::isEOS(streamid_t stream)
{
    if (mDecoders.at(stream).decoderHW == OMAF_NULL)
    {
        return false;
    }
    bool_t decoderEoS = mDecoders.at(stream).decoderHW->isEOS();
    if (decoderEoS && (mFrameCache->getDecodedFrameCount(stream) == 0))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void_t VideoDecoderManager::setInputEOS(streamid_t stream)
{
    OMAF_ASSERT(mDecoders.at(stream).decoderHW != OMAF_NULL, "No decoder for stream");
    mDecoders.at(stream).decoderHW->setInputEOS();
}

bool_t VideoDecoderManager::isInputEOS(streamid_t stream) const
{
    OMAF_ASSERT(mDecoders.at(stream).decoderHW != OMAF_NULL, "No decoder for stream");
    return mDecoders.at(stream).decoderHW->isInputEOS();
}

Error::Enum VideoDecoderManager::preloadTexturesForPTS(const Streams& baseStreams,
                                                       const Streams& additionalStreams,
                                                       const Streams& skippedStreams,
                                                       uint64_t targetPTSUs)
{
    Streams allStreams;
    allStreams.add(baseStreams);
    allStreams.add(additionalStreams);
    allStreams.add(skippedStreams);
    mFrameCache->clearDiscardedFrames(allStreams);
    return Error::OK;
}

void_t VideoDecoderManager::activateDecoder(const streamid_t streamId)
{
    DecoderState& decoderState = mDecoders.at(streamId);

    // Stream not active so return without changes
    if (!decoderState.decoderActive)
    {
        return;
    }

    if (!decoderState.frameCacheActive)
    {
        mFrameCache->activateStream(streamId);
        decoderState.frameCacheActive = true;
    }

    if (!decoderState.textureActive)
    {
        mFrameCache->createTexture(streamId, decoderState.config);
        decoderState.textureActive = true;
    }
}

Error::Enum VideoDecoderManager::uploadTexturesForPTS(const Streams& baseStreams,
                                                      const Streams& additionalStreams,
                                                      uint64_t targetPTSUs,
                                                      TextureLoadOutput& output)
{
    for (Streams::ConstIterator it = baseStreams.begin(); it != baseStreams.end(); ++it)
    {
        activateDecoder(*it);
    }
    for (Streams::ConstIterator it = additionalStreams.begin(); it != additionalStreams.end(); ++it)
    {
        activateDecoder(*it);
    }

    output.updatedStreams.clear();

    FrameList frames;

    // First get the required frames
    Error::Enum result = mFrameCache->getSynchedFramesForPTS(baseStreams, additionalStreams, targetPTSUs, frames);
    
	for (auto frame : frames)
    {
        OMAF_LOG_D(">>> Stream %d (%dx%d) synched frame pts: %d dts: %d duration: %d uploadTime: %d consumed: %d staged: %d flushed: %d", 
			frame->streamId, 
			frame->width, 
			frame->height, 
			frame->pts, 
			frame->dts, 
			frame->duration, 
			frame->uploadTime, 
			frame->consumed, 
			frame->staged, 
			frame->flushed);
    }
    
	Streams allStreams;
    allStreams.add(baseStreams);
    allStreams.add(additionalStreams);
    if (result != Error::OK)
    {
        mFrameCache->cleanUpOldFrames(allStreams, targetPTSUs);
        return result;
    }

    // Upload all required
    for (FrameList::ConstIterator it = frames.begin(); it != frames.end(); ++it)
    {
        DecoderFrame* decoderFrame = *it;

        output.updatedStreams.add(decoderFrame->streamId);
        mFrameCache->uploadFrame(decoderFrame, targetPTSUs);
    }

    DecoderFrame* decoderFrame = *frames.begin();
    mLatestPts = decoderFrame->pts;
    output.pts = decoderFrame->pts;
    output.frameDuration = decoderFrame->duration;

    mFrameCache->cleanUpOldFrames(allStreams, targetPTSUs);
    return Error::OK;
}

const VideoFrame& VideoDecoderManager::getCurrentVideoFrame(streamid_t stream)
{
    return mFrameCache->getCurrentVideoFrame(stream);
}

streamid_t VideoDecoderManager::generateUniqueStreamID(bool_t aMakeItShared)
{
    for (uint32_t i = 0; i < mDecoders.getSize(); i++)
    {
        if (mDecoders[i].free)
        {
            mDecoders.at(i).free = false;
            if (aMakeItShared)
            {
                // let's store the previous one too as it may be still needed if this is a viewpoint switch
                mPrevSharedStreamID = mSharedStreamID;
                mSharedStreamID = i;
                OMAF_LOG_V("Generating a new shared stream id %d, old %d", mSharedStreamID, mPrevSharedStreamID);
            }
            else
            {
                OMAF_LOG_V("Generating a new unique stream id %d", i);
            }
            return i;
        }
    }

    return -1;
}

// the shared streams must have the same configuration (width, height etc)
streamid_t VideoDecoderManager::getSharedStreamID()
{
    if (mSharedStreamID == OMAF_UINT8_MAX)
    {
        mSharedStreamID = generateUniqueStreamID();
        OMAF_LOG_V("Generating a shared stream id %d", mSharedStreamID);
    }
    return mSharedStreamID;
}

void_t VideoDecoderManager::releaseSharedStreamID(streamid_t aStreamId)
{
    OMAF_LOG_V("releaseSharedStreamID: %d", aStreamId);
    if (mPrevSharedStreamID == aStreamId)
    {
        mPrevSharedStreamID = InvalidStreamId;
    }
    else if (mSharedStreamID == aStreamId)
    {
        mSharedStreamID = InvalidStreamId;
    }
}

bool_t VideoDecoderManager::syncStreams(streamid_t anchorStream, streamid_t stream)
{
    return mFrameCache->syncStreams(anchorStream, stream);
}

uint64_t VideoDecoderManager::getLatestPTS()
{
    return mLatestPts;
}

OMAF_NS_END
