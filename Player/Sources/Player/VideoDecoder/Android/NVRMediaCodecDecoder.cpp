
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
#include "VideoDecoder/Android/NVRMediaCodecDecoder.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRPerformanceLogger.h"
#include "VideoDecoder/Android/NVRMediaCodecDecoderHW.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(MediaCodecDecoder);

MediaCodecDecoder::MediaCodecDecoder()
{
    mInitialBufferingThreshold = 1;
}

MediaCodecDecoder::~MediaCodecDecoder()
{
    for (FreeDecoders::Iterator it = mFreeDecoders.begin(); it != mFreeDecoders.end(); ++it)
    {
        MediaCodecDecoderHW* decoder = *it;
        if (decoder->getState() != DecoderHWState::IDLE)
        {
            decoder->deinitialize();
        }
        decoder->destroyInstance();
        OMAF_DELETE_HEAP(decoder);
    }
}

void_t MediaCodecDecoder::createVideoDecoders(VideoCodec::Enum codec, uint32_t decoderCount)
{
    for (uint32_t count = 0; count < decoderCount; count++)
    {
        VideoDecoderHW* decoder = OMAF_NEW_HEAP(MediaCodecDecoderHW)(*mFrameCache);
        MimeType mimeType = UNKNOWN_VIDEO_MIME_TYPE;
        if (codec == VideoCodec::AVC)
        {
            mimeType = VIDEO_H264_MIME_TYPE;
        }
        else if (codec == VideoCodec::HEVC)
        {
            mimeType = VIDEO_HEVC_MIME_TYPE;
        }
        OMAF_ASSERT(mimeType != UNKNOWN_VIDEO_MIME_TYPE, "Invalid video codec");
        Error::Enum result = decoder->createInstance(mimeType);
        if (result == Error::OK)
        {
            mFreeDecoders.add((MediaCodecDecoderHW*) decoder);
        }
        else
        {
            OMAF_DELETE_HEAP(decoder);
        }
    }
}

VideoDecoderHW* MediaCodecDecoder::reserveVideoDecoder(const DecoderConfig& config)
{
    MediaCodecDecoderHW* decoder = OMAF_NULL;
    Error::Enum initResult = Error::OK;
    OMAF_LOG_D("reserveVideoDecoder, # of free decoders %d", mFreeDecoders.getSize());
    // First see if there's a decoder matching both MimeType and resolution
    for (FreeDecoders::Iterator it = mFreeDecoders.begin(); it != mFreeDecoders.end(); ++it)
    {
        if (config.mimeType == (*it)->getMimeType())
        {
            if (config.width == (*it)->getWidth() && config.height == (*it)->getHeight())
            {
                decoder = *it;
                OMAF_LOG_V("--- Using an existing decoder with same MimeType and resolution (%d x %d)", config.width,
                           config.height);
                break;
            }
            else if (decoder == OMAF_NULL || ((*it)->getWidth() == 0) ||
                     (((*it)->getWidth() * (*it)->getHeight() > decoder->getWidth() * decoder->getHeight()) &&
                      decoder->getWidth() > 0))  // always prefer uninitialized decoder
            {
                // take a candidate: one with largest resolution of the free ones
                decoder = *it;
                OMAF_LOG_V("--- Replacing an existing decoder with same MimeType, candidate found (%d x %d vs %d x %d)",
                           decoder->getWidth(), decoder->getHeight(), config.width, config.height);
            }
        }
    }


    // If a decoder was found, reinitialize it (equality of config & need for reconfiguration is checked inside
    // initialize)
    if (decoder != OMAF_NULL)
    {
        mFreeDecoders.remove(decoder);
        OMAF_LOG_V("Replacing an existing decoder (%d x %d vs %d x %d)", decoder->getWidth(), decoder->getHeight(),
                   config.width, config.height);

        initResult = decoder->initialize(config);
    }
    else
    {
        // Still no decoder so create one
        if (!mFreeDecoders.isEmpty())
        {
            // resolution?
            decoder = mFreeDecoders.at(0);
            mFreeDecoders.removeAt(0);
            if (decoder->getState() != DecoderHWState::IDLE)
            {
                decoder->deinitialize();
            }
            decoder->destroyInstance();
            OMAF_DELETE_HEAP(decoder);
        }
        OMAF_LOG_V("Create a decoder (%d x %d)", config.width, config.height);
        decoder = OMAF_NEW_HEAP(MediaCodecDecoderHW)(*mFrameCache);
        decoder->createInstance(config.mimeType);
        initResult = decoder->initialize(config);
    }
    if (initResult != Error::OK)
    {
        OMAF_DELETE_HEAP(decoder);
        return OMAF_NULL;
    }
    return decoder;
}

void_t MediaCodecDecoder::releaseVideoDecoder(VideoDecoderHW* decoder)
{
    MediaCodecDecoderHW* dec = (MediaCodecDecoderHW*)decoder;
    OMAF_LOG_V("Released decoder (%d x %d)", dec->getWidth(), dec->getHeight());
    // NOTE: codec not released, because reserve can decide if it must be reinitialized for use
    mFreeDecoders.add(dec);
}

Error::Enum MediaCodecDecoder::preloadTexturesForPTS(const Streams& baseStreams,
                                                     const Streams& additionalStreams,
                                                     const Streams& skippedStreams,
                                                     uint64_t targetPTSUs)
{
    Streams allStreams;
    allStreams.add(baseStreams);
    allStreams.add(additionalStreams);
    allStreams.add(skippedStreams);
    mFrameCache->clearDiscardedFrames(allStreams);
    Error::Enum result = Error::OK;
    for (Streams::ConstIterator it = baseStreams.begin(); it != baseStreams.end(); ++it)
    {
        result = ((MediaCodecDecoderHW*) mDecoders.at(*it).decoderHW)->fetchDecodedFrames();
        if (result == Error::OPERATION_FAILED)
        {
            return Error::OPERATION_FAILED;
        }
    }

    for (Streams::ConstIterator it = additionalStreams.begin(); it != additionalStreams.end(); ++it)
    {
        ((MediaCodecDecoderHW*) mDecoders.at(*it).decoderHW)->fetchDecodedFrames();
    }
    for (Streams::ConstIterator it = skippedStreams.begin(); it != skippedStreams.end(); ++it)
    {
        ((MediaCodecDecoderHW*) mDecoders.at(*it).decoderHW)->fetchDecodedFrames();
    }

    Mutex& uploadedFramesMutex = mUploadedFramesMutex;
    UploadedFrames& uploadedFrames = mUploadedFrames;

    uploadedFramesMutex.lock();
    bool_t uploadedFramesIsEmpty = uploadedFrames.isEmpty();
    Streams uploadedStreams;
    uploadedStreams.add(mAllUploadedStreams);
    uploadedFramesMutex.unlock();

    if (!uploadedFramesIsEmpty)
    {
        return Error::OK_NO_CHANGE;
    }
    else
    {
        mFrameCache->cleanUpOldFrames(uploadedStreams, targetPTSUs);
        UploadedFrames tempUploadedFrames;
        FrameList framesForPTS;
        Error::Enum result =
            mFrameCache->getSynchedFramesForPTS(baseStreams, additionalStreams, targetPTSUs, framesForPTS);
        if (result != Error::OK)
        {
            return result;
        }

        if (!framesForPTS.isEmpty())
        {
            for (FrameList::Iterator it = framesForPTS.begin(); it != framesForPTS.end(); ++it)
            {
                DecoderFrame* frame = *it;
                UploadedFrame uploadedFrame;
                uploadedFrame.frame = frame;
                uploadedFrame.stream = frame->streamId;
                uploadedFrame.pts = frame->pts;
                bool_t staged = mFrameCache->stageFrame(frame);
                if (staged)
                {
                    MediaCodecDecoderHW* decoderHW = (MediaCodecDecoderHW*) mDecoders.at(frame->streamId).decoderHW;
                    decoderHW->releaseOutputBuffer((DecoderFrameAndroid*) frame, true);
                }
                tempUploadedFrames.add(uploadedFrame);
            }
        }
        {
            Mutex::ScopeLock lock(uploadedFramesMutex);
            uploadedFrames.add(tempUploadedFrames);
        }
        return Error::OK_NO_CHANGE;
    }
}

void_t MediaCodecDecoder::activateDecoder(const streamid_t streamId)
{
    DecoderState& decoderState = mDecoders.at(streamId);
    if (!decoderState.decoderActive)
    {
        return;
    }
    if (!decoderState.frameCacheActive)
    {
        // We need to do some Android specific stuff
        ((MediaCodecDecoderHW*) decoderState.decoderHW)->createOutputTexture();
        decoderState.textureActive = true;
        mFrameCache->activateStream(streamId);
        decoderState.frameCacheActive = true;
    }
}

void_t MediaCodecDecoder::deactivateStream(streamid_t stream)
{
    removeStreamFromUploaded(stream);
    VideoDecoderManager::deactivateStream(stream);
}

bool_t MediaCodecDecoder::syncStreams(streamid_t anchorStream, streamid_t stream)
{
    removeStreamFromUploaded(stream);
    return mFrameCache->syncStreams(anchorStream, stream);
}

Error::Enum MediaCodecDecoder::setupStreams(const Streams& streams)
{
    for (Streams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        activateDecoder(*it);
    }
    return Error::OK;
}


Error::Enum MediaCodecDecoder::uploadTexturesForPTS(const Streams& baseStreams,
                                                    const Streams& additionalStreams,
                                                    uint64_t targetPTSUs,
                                                    TextureLoadOutput& output)
{
    Mutex& uploadedFramesMutex = mUploadedFramesMutex;
    UploadedFrames& uploadedFrames = mUploadedFrames;
    Streams& uploadedStreams = mAllUploadedStreams;
    PerformanceLogger logger("uploadTexturesForPTS", 5);
    Mutex::ScopeLock uploadLock(uploadedFramesMutex);
    logger.printIntervalTime("uploadLock");
    Error::Enum setupResult = setupStreams(baseStreams);
    if (setupResult != Error::OK)
    {
        return setupResult;
    }
    // Should optional setup error be returned?
    setupResult = setupStreams(additionalStreams);

    logger.printIntervalTime("setupStreams");
    Streams allStreams;
    allStreams.add(baseStreams);
    allStreams.add(additionalStreams);
    uploadedStreams.clear();
    uploadedStreams.add(allStreams);
    if (uploadedFrames.isEmpty())
    {
        return Error::OK_NO_CHANGE;
    }

    output.updatedStreams.clear();
    output.pts = (*uploadedFrames.begin()).pts;
    mLatestPts = (*uploadedFrames.begin()).pts;

    for (UploadedFrames::Iterator it = uploadedFrames.begin(); it != uploadedFrames.end(); ++it)
    {
        UploadedFrame& uploadedFrame = *it;
        output.updatedStreams.add((*it).stream);
        DecoderFrame* stagedFrame = mFrameCache->fetchStagedFrame(uploadedFrame.stream);
        // Upload only of there's still a staged frame
        if (stagedFrame == uploadedFrame.frame)
        {
            mFrameCache->uploadFrame((*it).frame, targetPTSUs);
        }
    }
    logger.printIntervalTime("uploadedFrames");
    output.frameDuration = (*uploadedFrames.begin()).frame->duration;
    uploadedFrames.clear();
    return Error::OK;
}

void_t MediaCodecDecoder::removeStreamFromUploaded(streamid_t stream)
{
    mUploadedFramesMutex.lock();
    for (size_t index = 0; index < mUploadedFrames.getSize(); index++)
    {
        if (mUploadedFrames.at(index).stream == stream)
        {
            mUploadedFrames.removeAt(index);
        }
    }
    mUploadedFramesMutex.unlock();
}

OMAF_NS_END