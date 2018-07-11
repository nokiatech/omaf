
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
#include "NVRMediaCodecDecoderHW.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRTime.h"
#include "Foundation/Android/NVRAndroid.h"
#include "Foundation/NVRDeviceInfo.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(MediaCodecDecoderHW)

MediaCodecDecoderHW::MediaCodecDecoderHW(FrameCache &frameCache)
: mInputEOS(false)
, mState(DecoderHWState::INVALID)
, mFrameCache(frameCache)
, mMediaCodec(OMAF_NULL)
{
}

MediaCodecDecoderHW::~MediaCodecDecoderHW()
{
    OMAF_ASSERT(mMediaCodec == OMAF_NULL, "Decoder not shutdown");
}

const MimeType& MediaCodecDecoderHW::getMimeType() const
{
    return mMimeType;
}

uint32_t MediaCodecDecoderHW::getWidth() const
{
    return mDecoderConfig.width;
}

uint32_t MediaCodecDecoderHW::getHeight() const
{
    return mDecoderConfig.height;
}

OutputTexture& MediaCodecDecoderHW::getOutputTexture()
{
    return mOutputTexture;
}

Error::Enum MediaCodecDecoderHW::createInstance(const MimeType& mimeType)
{
    OMAF_ASSERT(mState == DecoderHWState::INVALID, "Incorrect state");
    OMAF_ASSERT(mMediaCodec == OMAF_NULL, "Codec already created");
    OMAF_LOG_I("Creating video decoder for mime type: %s", mMimeType.getData());
    mMimeType = mimeType;
    mMediaCodec = AMediaCodec_createDecoderByType(mMimeType.getData());
    if (mMediaCodec == NULL)
    {
        OMAF_LOG_E("Could not create video decoder!");
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }
    mState = DecoderHWState::IDLE;
    return Error::OK;
}

Error::Enum MediaCodecDecoderHW::initialize(const DecoderConfig& config)
{
    OMAF_ASSERT(config.mimeType == mMimeType, "Incorrect mimetype");
    if (mState != DecoderHWState::IDLE)
    {
        // Video size doesn't match so we need to reinitialize the decoder
        if (config.width != mDecoderConfig.width
            || config.height != mDecoderConfig.height)
        {
            deinitialize();

            if (!DeviceInfo::deviceCanReconfigureVideoDecoder())
            {
                OMAF_LOG_D("Have to reinstantiate video decoder");
                uint32_t timeBefore = Time::getClockTimeMs();
                // recreate the decoder for now - until we know why reusing it doesn't work always
                AMediaCodec_delete(mMediaCodec);
                mMediaCodec = OMAF_NULL;
                mState = DecoderHWState::INVALID;
                mMediaCodec = AMediaCodec_createDecoderByType(mMimeType.getData());
                if (mMediaCodec == NULL)
                {
                    OMAF_LOG_E("Could not create video decoder!");
                    mState = DecoderHWState::ERROR_STATE;
                    return Error::OPERATION_FAILED;
                }
                OMAF_LOG_V("Recreate decoder took %d ms", Time::getClockTimeMs() - timeBefore);
            }
        }
        else
        {
            // Reusing an already running decoder, just update the stream id
            mDecoderConfig.streamId = config.streamId;
            return Error::OK;
        }
    }
    mDecoderConfig = config;
    mState = DecoderHWState::INITIALIZED;
    return Error::OK;
}

bool_t MediaCodecDecoderHW::isEOS()
{
    return mState == DecoderHWState::EOS;
}

void_t MediaCodecDecoderHW::deinitialize()
{
    OMAF_ASSERT(mMediaCodec != OMAF_NULL, "No decoder created");
    flush();
    media_status_t status = AMediaCodec_stop(mMediaCodec);
    if (status != AMEDIA_OK)
    {
        OMAF_LOG_E("MediaCodec stop failed with status = %d", status);
        OMAF_ASSERT(false, "Stop failed!");
    }
    mInputEOS = false;
    mState = DecoderHWState::IDLE;

}

void_t MediaCodecDecoderHW::flush()
{
    if (mState == DecoderHWState::STARTED || mState == DecoderHWState::EOS)
    {
        media_status_t status = AMediaCodec_flush(mMediaCodec);
    }
    mInputEOS = false;
    if (mState == DecoderHWState::EOS)
    {
        mState = DecoderHWState::STARTED;
    }
}

void_t MediaCodecDecoderHW::destroyInstance()
{
    OMAF_ASSERT(mState == DecoderHWState::IDLE, "Wrong state");
    OMAF_ASSERT(mMediaCodec != OMAF_NULL, "MediaCodec handle already destroyed");

    AMediaCodec_delete(mMediaCodec);
    mMediaCodec = OMAF_NULL;
    mState = DecoderHWState::INVALID;

    if (mOutputTexture.surface != OMAF_NULL)
    {
        OMAF_DELETE_HEAP(mOutputTexture.surface);
        mOutputTexture.surface = OMAF_NULL;
    }
    if (mOutputTexture.nativeWindow != OMAF_NULL)
    {
        ANativeWindow_release(mOutputTexture.nativeWindow);
        mOutputTexture.nativeWindow = OMAF_NULL;
    }
}

void_t MediaCodecDecoderHW::setInputEOS()
{
    if (mInputEOS)
    {
        return;
    }
    if (mState != DecoderHWState::STARTED)
    {
        OMAF_LOG_D("Trying to set EOS when not started");
        mInputEOS = true;
        mState = DecoderHWState::EOS;
        return;
    }

    // Send an EOS flag in the input buffer.
    // We know all frames have been processed when it comes out from the output side.
    ssize_t inbufIndex = AMediaCodec_dequeueInputBuffer(mMediaCodec, 1000);

    if (inbufIndex >= 0)
    {
        media_status_t status = AMediaCodec_queueInputBuffer(mMediaCodec,
                                                             (size_t)inbufIndex, 0, 0, OMAF_UINT64_MAX,
                                                             AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);

        if (status == AMEDIA_OK)
        {
            OMAF_LOG_I("Video stream input reached EOS");
            mInputEOS = true;
            return;
        }
    }
    else
    {
        // All input buffers might be in use. Can't send the flag until the consumer side does some work.
    }

    // Try again later
    return;
}

Error::Enum MediaCodecDecoderHW::configureCodec()
{
    uint32_t timeBefore = Time::getClockTimeMs();
    OMAF_ASSERT(mState == DecoderHWState::INITIALIZED, "Incorrect state");

    AMediaFormat* androidMediaFormat = AMediaFormat_new();

    AMediaFormat_setString(androidMediaFormat, AMEDIAFORMAT_KEY_MIME, mMimeType.getData());

    OMAF_ASSERT((mMimeType == VIDEO_H264_MIME_TYPE || mMimeType == VIDEO_HEVC_MIME_TYPE), "Unsupported mimetype");

    if (mMimeType == VIDEO_H264_MIME_TYPE)
    {
        // H.264 configuration is put in 1 buffer (https://developer.android.com/reference/android/media/MediaCodec.html)
        AMediaFormat_setBuffer(androidMediaFormat, "csd-0", mDecoderConfig.spsData, mDecoderConfig.spsSize);
        AMediaFormat_setBuffer(androidMediaFormat, "csd-1", mDecoderConfig.ppsData, mDecoderConfig.ppsSize);
    }
    else if (mMimeType == VIDEO_HEVC_MIME_TYPE)
    {
        // HEVC/H.265 configuration is put in 1 buffer (https://developer.android.com/reference/android/media/MediaCodec.html)
        AMediaFormat_setBuffer(androidMediaFormat, "csd-0", mDecoderConfig.configInfoData, mDecoderConfig.configInfoSize);
    }

    OMAF_LOG_V("configureCodec for %d x %d", mDecoderConfig.width, mDecoderConfig.height);
    AMediaFormat_setInt32(androidMediaFormat, AMEDIAFORMAT_KEY_WIDTH, mDecoderConfig.width);
    AMediaFormat_setInt32(androidMediaFormat, AMEDIAFORMAT_KEY_HEIGHT, mDecoderConfig.height);

    media_status_t status = AMediaCodec_configure(mMediaCodec, androidMediaFormat, mOutputTexture.nativeWindow, NULL, 0);

    if (status != AMEDIA_OK)
    {
        OMAF_LOG_E("Could not configure video decoder, status = %d", status);
        AMediaFormat_delete(androidMediaFormat);
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }

    mState = DecoderHWState::CONFIGURED;

    AMediaFormat_delete(androidMediaFormat);
    OMAF_LOG_D("Init tooK: %d", Time::getClockTimeMs() - timeBefore);
    return Error::OK;
}

Error::Enum MediaCodecDecoderHW::startCodec()
{
    OMAF_ASSERT(mState == DecoderHWState::CONFIGURED, "Decoder HW in wrong state");
    media_status_t status = AMediaCodec_start(mMediaCodec);

    if (status != AMEDIA_OK)
    {
        OMAF_LOG_E("MediaCodec start failed, status = %d", status);

        mState = DecoderHWState::ERROR_STATE;

        return Error::OPERATION_FAILED;
    }
    mState = DecoderHWState::STARTED;
    return Error::OK;
}

DecodeResult::Enum MediaCodecDecoderHW::decodeFrame(streamid_t stream, MP4VRMediaPacket *packet, bool_t seeking)
{
    if (mState == DecoderHWState::ERROR_STATE)
    {
        return DecodeResult::DECODER_ERROR;
    }
    if (mState == DecoderHWState::INITIALIZED && mOutputTexture.surface != OMAF_NULL)
    {
        configureCodec();
    }
    if (!(mState == DecoderHWState::CONFIGURED || mState == DecoderHWState::STARTED))
    {
        return DecodeResult::NOT_READY;
    }
    if (mState == DecoderHWState::CONFIGURED)
    {
        Error::Enum result = startCodec();
        if (result != Error::OK)
        {
            return DecodeResult::DECODER_ERROR;
        }
    }
    if (packet->isReConfigRequired())
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
    }

    if (seeking)
    {
        fetchDecodedFrames();
    }

    // Copy the given MediaPacket to the codec, in as many input buffers as necessary
    bool mediaPacketConsumed = false;
    size_t totalCopiedSize = 0;

    while (mediaPacketConsumed == false)
    {
        ssize_t inbufIndex = AMediaCodec_dequeueInputBuffer(mMediaCodec, 0); // Don't wait

        if (inbufIndex >= 0)
        {
            size_t bufsize;
            uint8_t *buf = AMediaCodec_getInputBuffer(mMediaCodec, (size_t)inbufIndex, &bufsize);
            size_t copiedSize = packet->copyTo(totalCopiedSize, buf, bufsize);
            totalCopiedSize += copiedSize;

            if (totalCopiedSize == packet->dataSize())
            {
                mediaPacketConsumed = true;
            }
            else if (totalCopiedSize > packet->dataSize())
            {
                // This should never happen if the app logic is ok
                OMAF_LOG_D("Total copied size %d > packet data size %d", totalCopiedSize, packet->dataSize());
                OMAF_ASSERT_UNREACHABLE();
            }

            media_status_t status = AMediaCodec_queueInputBuffer(mMediaCodec,
                                                                 (size_t)inbufIndex,
                                                                 0,
                                                                 copiedSize,
                                                                 (uint64_t)packet->presentationTimeUs(),
                                                                 0);

            if (status != AMEDIA_OK)
            {
                OMAF_LOG_W("MediaCodec queueInputBuffer() error = %d", status);
            }
        }
        else
        {
            // Note: it is perfectly fine to fail when no bytes have yet been copied from the packet... (in which case we must return false, and the provider should try again with the SAME packet)
            OMAF_ASSERT(totalCopiedSize == 0, "");

            // FIXME: Handle the situation where we consume just a portion of the given media packet, but run out of input buffers before it's completely consumed
            // In that case we shouldn't resume copying the packet from the start, but where we left off
            break;
        }
    }
    if (mediaPacketConsumed)
    {
        return DecodeResult::PACKET_ACCEPTED;
    }
    else
    {
        return DecodeResult::DECODER_FULL;
    }
}


Error::Enum MediaCodecDecoderHW::fetchDecodedFrames()
{
    if (mState != DecoderHWState::STARTED)
    {
        return Error::INVALID_STATE;
    }

    ssize_t nextFrameOutputBufferIndex = 0;

    while (nextFrameOutputBufferIndex >= 0)
    {
        DecoderFrame* outputFrame = mFrameCache.getFreeFrame(mDecoderConfig.streamId);

        if (outputFrame == OMAF_NULL)
        {
            return Error::OK_SKIPPED;
        }
        // need to reset the decoder in case it is skipped below and we are then flushing the codec before next time
        ((DecoderFrameAndroid*)(outputFrame))->decoder = OMAF_NULL;

        // Get a new decoded output buffer
        AMediaCodecBufferInfo frameInfo;
        nextFrameOutputBufferIndex = AMediaCodec_dequeueOutputBuffer(mMediaCodec, &frameInfo, 0);

        if (nextFrameOutputBufferIndex >= 0)
        {
            if (frameInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
            {
                // When the video packet was input (with AMediaCodec_queueInputBuffer), it was marked EOS
                OMAF_LOG_I("Video stream output reached EOS");

                mState = DecoderHWState::EOS;
            }

            if (frameInfo.size > 0)
            {
                uint64_t pts = frameInfo.presentationTimeUs;
                DecoderFrameAndroid* frameAndroid = (DecoderFrameAndroid*)(outputFrame);
                frameAndroid->decoderOutputIndex = nextFrameOutputBufferIndex;
                frameAndroid->decoder = this;
                // TODO: Duration is empty
                frameAndroid->duration = 0;
                frameAndroid->streamId = mDecoderConfig.streamId;
                frameAndroid->pts = frameInfo.presentationTimeUs;
                frameAndroid->consumed = false;

                //OMAF_LOG_D("fetched frame %llu for stream %d", frameInfo.presentationTimeUs, mDecoderConfig.streamId);
                mFrameCache.addDecodedFrame(frameAndroid);
                outputFrame = OMAF_NULL;
            }
            else
            {
                // The buffer contains no data. Immediately release it without rendering.
                AMediaCodec_releaseOutputBuffer(mMediaCodec, (size_t)nextFrameOutputBufferIndex, false);
                nextFrameOutputBufferIndex = AMEDIACODEC_INFO_TRY_AGAIN_LATER;
            }
        }

        // Handle outputbuffers that don't contain video data
        if (nextFrameOutputBufferIndex < 0 &&
            nextFrameOutputBufferIndex != AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            if (nextFrameOutputBufferIndex == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
            {
                OMAF_LOG_V("%d Output buffers changed", mDecoderConfig.streamId);
            }
            else if (nextFrameOutputBufferIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
            {
                AMediaFormat *format = AMediaCodec_getOutputFormat(mMediaCodec);
                // FIXME: DON'T overwite the media format here. This is format for the raw data.
                OMAF_LOG_V("Format of %d changed to: %s", mDecoderConfig.streamId, AMediaFormat_toString(format));
                //			mCurrentMediaStream->setFormat(format);
                //			updateMediaFormat();
            }
            else
            {
                OMAF_LOG_W("Unexpected output buffer info code: %zd", nextFrameOutputBufferIndex);
                return Error::OPERATION_FAILED;
            }

            nextFrameOutputBufferIndex = AMEDIACODEC_INFO_TRY_AGAIN_LATER;
        }
        if (outputFrame != OMAF_NULL)
        {
            mFrameCache.releaseFrame(outputFrame);
        }
    }
    return Error::OK;
}

Error::Enum MediaCodecDecoderHW::createOutputTexture()
{
    if (mOutputTexture.surface != OMAF_NULL)
    {
        return Error::ALREADY_SET;
    }

    mOutputTexture.surface = OMAF_NEW_HEAP(Surface);

    jobject jsurface = mOutputTexture.surface->getJavaObject();

    JNIEnv* jenv = Android::getJNIEnv();
    mOutputTexture.nativeWindow = ANativeWindow_fromSurface(jenv, jsurface);
    return Error::OK;
}

void_t MediaCodecDecoderHW::consumedFrame(DecoderFrame* frame)
{
    releaseOutputBuffer(frame, false);
}

void_t MediaCodecDecoderHW::releaseOutputBuffer(DecoderFrame* frame, bool_t upload)
{
    DecoderFrameAndroid* frameAndroid = (DecoderFrameAndroid*)frame;
    OMAF_ASSERT(mOutputTexture.surface != OMAF_NULL, "No output texture set");
    if (frameAndroid->decoderOutputIndex == -1)
    {
        return;
    }
    //OMAF_LOG_D("%d releaseOutputBuffer %llu", frame->streamId, frame->pts);
    media_status_t mediaStatus = AMediaCodec_releaseOutputBuffer(mMediaCodec, (size_t)frameAndroid->decoderOutputIndex, upload);
    if (mediaStatus != AMEDIA_OK)
    {
        OMAF_LOG_W("MediaCodec releaseOutputBuffer() %d, error = %d", frameAndroid->decoderOutputIndex, mediaStatus);
    }
    frameAndroid->decoderOutputIndex = -1;
    // This frame has been released so set the decoder pointer to null so it won't be called again
    frameAndroid->consumed = true;
    return;
}

DecoderHWState::Enum MediaCodecDecoderHW::getState() const
{
    return mState;
}

OMAF_NS_END
