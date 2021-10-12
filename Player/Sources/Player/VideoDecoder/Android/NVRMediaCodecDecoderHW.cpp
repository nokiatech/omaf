
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
#include "NVRMediaCodecDecoderHW.h"
#include "Foundation/Android/NVRAndroid.h"
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"

#include "Foundation/NVRFixedArray.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(MediaCodecDecoderHW)

const size_t MAX_ALLOCATED_CODECS = 10;

/**
 * Helper class to handle AMediaCodecs stored in codec pool safely.
 *
 * Friend of CodecPool.
 */
class CodecPool;
class CodecEntry
{
public:
    // Codec instance allocated, flushing and stopping is done when codec is returned to pool
    AMediaCodec* codec;

    // Frame index of the last frame that has been decoded successfully
    // (for making it available when stream reach EOS) and codec is released
    ssize_t lastDecodedFrameIndex;

    /**
     * Creates codec for given mime type
     */
    CodecEntry(MimeType aType)
        : mFree(true), lastDecodedFrameIndex(-1)
    {
        mType = aType;
        codec = AMediaCodec_createDecoderByType(mType.getData());
        OMAF_ASSERT(codec != NULL, "Could not create video decoder!");
    }

    /**
     * Recreate codec with different mime type if old mime type doesn't match with the new one
     */
    void_t setMimeType(MimeType newType)
    {
        if (newType != mType)
        {
            AMediaCodec_flush(codec);
            AMediaCodec_stop(codec);
            AMediaCodec_delete(codec);
            codec = AMediaCodec_createDecoderByType(newType.getData());
            OMAF_ASSERT(codec != NULL, "Could not create video decoder!");
        }
    }

private:
    friend class CodecPool;

    /**
     * Releases codec to be free again and updates last decoded frame to be available in
     * decoders image stream to allow SurfaceTexture->updateTexImage() read the image data.
     *
     * Also flushes and stops the codec.
     */
    void_t releaseCodec()
    {

        OMAF_LOG_D("Release codec mFree = %d, lastDecodedFrameIndex = %d", mFree, lastDecodedFrameIndex);
        // release and update for being able to be fetched by updateTexImage, looks like buffer can
        // be read at anytime, even after flushing
        if (lastDecodedFrameIndex >= 0)
        {
            media_status_t status = AMediaCodec_releaseOutputBuffer(codec, lastDecodedFrameIndex, true);
            if (status != AMEDIA_OK)
            {
                OMAF_LOG_E("MediaCodec releaseOutputBuffer failed with status = %d, lastDecodedFrameIndex = %d", status, lastDecodedFrameIndex);
                OMAF_ASSERT(false, "Release output buffer failed!");
            }
        }

        if (!mFree) {
            media_status_t status = AMediaCodec_stop(codec);
            if (status != AMEDIA_OK)
            {
                OMAF_LOG_E("MediaCodec stop failed with status = %d", status);
                OMAF_ASSERT(false, "Stop failed!");
            }
        }

        lastDecodedFrameIndex = -1;
        mFree = true;
    }

    MimeType mType;
    bool_t mFree;
};

class CodecPool
{
public:
    /**
     * Gets codec from pool.
     *
     * @param type MimeType of codec to fetch
     * @return Wrapper for using codec safely. NULL if pool is full.
     */
    CodecEntry* get(MimeType type)
    {
        // get free codec
        auto freeCodec = findFreeCodec(type);

        // if not found, allocate new if still room
        if (freeCodec == OMAF_NULL && mAllocatedCodecs.getSize() < MAX_ALLOCATED_CODECS)
        {
            freeCodec = OMAF_NEW_HEAP(CodecEntry)(type);
            mAllocatedCodecs.add(freeCodec);
        }

        // find any type of free codec to return
        if (freeCodec == OMAF_NULL)
        {
            freeCodec = findFirstFreeCodec();
        }

        // mark codec used and verify its mime type
        if (freeCodec != OMAF_NULL)
        {
            freeCodec->setMimeType(type);
            freeCodec->mFree = false;
            return freeCodec;
        }
        else
        {
            // pool was full
            return OMAF_NULL;
        }
    }

    /**
     * Returns codec to the pool and release its output buffers for use.
     *
     * @param codec Pointer to CodecEntry got from CodecPool::get()
     */
    void_t release(CodecEntry* codec)
    {
        codec->releaseCodec();
    }

private:
    // returns index of free codec to use for given mime type or NULL if not found
    CodecEntry* findFreeCodec(MimeType type)
    {
        // find free codec at first
        for (int32_t i = 0; i < mAllocatedCodecs.getSize(); i++)
        {
            auto entry = mAllocatedCodecs.at(i);
            if (entry->mFree && entry->mType == type)
            {
                return entry;
            }
        }
        return OMAF_NULL;
    }

    // returns index of first free codec of any mime type or -1 if none found
    CodecEntry* findFirstFreeCodec()
    {
        for (int32_t i = 0; i < mAllocatedCodecs.getSize(); i++)
        {
            auto entry = mAllocatedCodecs.at(i);
            if (entry->mFree)
            {
                return entry;
            }
        }
        return OMAF_NULL;
    }

    typedef FixedArray<CodecEntry*, MAX_ALLOCATED_CODECS> CodecAllocationList;

    CodecAllocationList mAllocatedCodecs;
};

static CodecPool codecPool;

MediaCodecDecoderHW::MediaCodecDecoderHW(FrameCache& frameCache)
    : mInputEOS(false)
    , mState(DecoderHWState::INVALID)
    , mFrameCache(frameCache)
    , mCodec(OMAF_NULL)
{
}

MediaCodecDecoderHW::~MediaCodecDecoderHW()
{
    OMAF_ASSERT(mCodec == OMAF_NULL, "Decoder not shutdown");
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
    OMAF_LOG_I("Creating MediaCodecDecoderHW (not initializing codec yet) decoder for mime type: %s",
               mMimeType.getData());
    OMAF_ASSERT(mState == DecoderHWState::INVALID, "Incorrect state");
    mMimeType = mimeType;
    mState = DecoderHWState::IDLE;
    return Error::OK;
}

Error::Enum MediaCodecDecoderHW::initialize(const DecoderConfig& config)
{
    OMAF_LOG_D(">>>>>>>>>>>>>> Initializing decoder");
    OMAF_ASSERT(config.mimeType == mMimeType, "Incorrect mimetype");
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
    OMAF_LOG_D("<<<<<<<<<<<<<<< Deinitializing decoder");
    flush();
    mInputEOS = false;
    mState = DecoderHWState::IDLE;
}

void_t MediaCodecDecoderHW::flush()
{
    OMAF_LOG_D("========== Flushing decoder");
    // some frame and decoder flushing for seeking
    mFrameCache.flushFrames(mDecoderConfig.streamId);
    if (mCodec)
    {
        AMediaCodec_flush(mCodec->codec);
    }
    mInputEOS = false;
    if (mState == DecoderHWState::EOS)
    {
        mState = DecoderHWState::STARTED;
    }
}

void_t MediaCodecDecoderHW::freeCodec()
{
    if (mCodec != OMAF_NULL)
    {
        OMAF_LOG_D("========== Releasing codec");
        codecPool.release(mCodec);
        mCodec = OMAF_NULL;
    }
}

void_t MediaCodecDecoderHW::destroyInstance()
{
    OMAF_LOG_D("========== Destroying decoder");
    OMAF_ASSERT(mState == DecoderHWState::IDLE, "Wrong state");
    freeCodec();
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
    ssize_t inbufIndex = AMediaCodec_dequeueInputBuffer(mCodec->codec, 1000);

    if (inbufIndex >= 0)
    {
        media_status_t status = AMediaCodec_queueInputBuffer(mCodec->codec, (size_t) inbufIndex, 0, 0, OMAF_UINT64_MAX,
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

bool_t MediaCodecDecoderHW::isInputEOS() const
{
    return mInputEOS;
}

// NOTE: probably should be refactored to be done inside the pool if required
Error::Enum MediaCodecDecoderHW::configureCodec()
{

    // make sure that codec is in stopped state and ready to be (re)configured
    media_status_t status = AMediaCodec_stop(mCodec->codec);
    if (status != AMEDIA_OK)
    {
        OMAF_LOG_E("Trying to stop MediaCodec before re-configure failed with status = %d", status);
        OMAF_ASSERT(false, "Stop failed!");
    }

    uint32_t timeBefore = Time::getClockTimeMs();
    OMAF_ASSERT(mState == DecoderHWState::INITIALIZED, "Incorrect state");

    AMediaFormat* androidMediaFormat = AMediaFormat_new();

    AMediaFormat_setString(androidMediaFormat, AMEDIAFORMAT_KEY_MIME, mMimeType.getData());

    OMAF_ASSERT((mMimeType == VIDEO_H264_MIME_TYPE || mMimeType == VIDEO_HEVC_MIME_TYPE), "Unsupported mimetype");

    if (mMimeType == VIDEO_H264_MIME_TYPE)
    {
        // H.264 configuration is put in 1 buffer
        // (https://developer.android.com/reference/android/media/MediaCodec.html)
        AMediaFormat_setBuffer(androidMediaFormat, "csd-0", mDecoderConfig.spsData, mDecoderConfig.spsSize);
        AMediaFormat_setBuffer(androidMediaFormat, "csd-1", mDecoderConfig.ppsData, mDecoderConfig.ppsSize);
    }
    else if (mMimeType == VIDEO_HEVC_MIME_TYPE)
    {
        // HEVC/H.265 configuration is put in 1 buffer
        // (https://developer.android.com/reference/android/media/MediaCodec.html)
        AMediaFormat_setBuffer(androidMediaFormat, "csd-0", mDecoderConfig.configInfoData,
                               mDecoderConfig.configInfoSize);
    }

    OMAF_LOG_V("configureCodec for %d x %d", mDecoderConfig.width, mDecoderConfig.height);
    AMediaFormat_setInt32(androidMediaFormat, AMEDIAFORMAT_KEY_WIDTH, mDecoderConfig.width);
    AMediaFormat_setInt32(androidMediaFormat, AMEDIAFORMAT_KEY_HEIGHT, mDecoderConfig.height);

    status =
        AMediaCodec_configure(mCodec->codec, androidMediaFormat, mOutputTexture.nativeWindow, NULL, 0);


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
    media_status_t status = AMediaCodec_start(mCodec->codec);

    if (status != AMEDIA_OK)
    {
        OMAF_LOG_E("MediaCodec start failed, status = %d", status);

        mState = DecoderHWState::ERROR_STATE;

        return Error::OPERATION_FAILED;
    }
    mState = DecoderHWState::STARTED;
    return Error::OK;
}

DecodeResult::Enum MediaCodecDecoderHW::decodeFrame(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking)
{
    // try to reserve hardware codec from pool
    if (mCodec == OMAF_NULL)
    {
        mCodec = codecPool.get(mMimeType);
    }

    // if not able to get codec, stop...
    if (mCodec == OMAF_NULL)
    {
        return DecodeResult::NOT_READY;
    }

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
        freeCodec();
        return DecodeResult::NOT_READY;
    }
    if (mState == DecoderHWState::CONFIGURED)
    {
        Error::Enum result = startCodec();
        if (result != Error::OK)
        {
            freeCodec();
            OMAF_LOG_E("Starting codec failed. Should never happen!");
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
        ssize_t inbufIndex = AMediaCodec_dequeueInputBuffer(mCodec->codec, 0);  // don't wait

        if (inbufIndex >= 0)
        {
            size_t bufsize;
            uint8_t* buf = AMediaCodec_getInputBuffer(mCodec->codec, (size_t) inbufIndex, &bufsize);
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

            media_status_t status = AMediaCodec_queueInputBuffer(mCodec->codec, (size_t) inbufIndex, 0, copiedSize,
                                                                 (uint64_t) packet->presentationTimeUs(), 0);

            if (status != AMEDIA_OK)
            {
                OMAF_LOG_W("MediaCodec queueInputBuffer() error = %d", status);
            }
        }
        else
        {
            // Note: it is perfectly fine to fail when no bytes have yet been copied from the packet... (in which case
            // we must return false, and the provider should try again with the SAME packet)
            OMAF_ASSERT(totalCopiedSize == 0, "");
            break;
        }
    }
    if (mediaPacketConsumed)
    {
        return DecodeResult::PACKET_ACCEPTED;
    }
    else
    {
        // keep codec reserved and don't reinitialize when trying to start decode next time
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
        ((DecoderFrameAndroid*) (outputFrame))->decoder = OMAF_NULL;

        // Get a new decoded output buffer
        AMediaCodecBufferInfo frameInfo;
        nextFrameOutputBufferIndex = AMediaCodec_dequeueOutputBuffer(mCodec->codec, &frameInfo, 0);

        if (nextFrameOutputBufferIndex >= 0)
        {
            if (frameInfo.size > 0)
            {
                uint64_t pts = frameInfo.presentationTimeUs;
                DecoderFrameAndroid* frameAndroid = (DecoderFrameAndroid*) (outputFrame);
                frameAndroid->decoderOutputIndex = nextFrameOutputBufferIndex;
                frameAndroid->decoder = this;
                frameAndroid->duration = 0;
                frameAndroid->streamId = mDecoderConfig.streamId;
                frameAndroid->pts = frameInfo.presentationTimeUs;
                frameAndroid->consumed = false;

                // OMAF_LOG_D("fetched frame %llu for stream %d", frameInfo.presentationTimeUs,
                // mDecoderConfig.streamId);
                mFrameCache.addDecodedFrame(frameAndroid);
                outputFrame = OMAF_NULL;

                // store last frame buffer index, which should be updated when codec is released on EOS
                mCodec->lastDecodedFrameIndex = nextFrameOutputBufferIndex;
            }
            else
            {
                // The buffer contains no data. Immediately release it without rendering.
                AMediaCodec_releaseOutputBuffer(mCodec->codec, (size_t) nextFrameOutputBufferIndex, false);
                nextFrameOutputBufferIndex = AMEDIACODEC_INFO_TRY_AGAIN_LATER;
            }

            if (frameInfo.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
            {
                // When the video packet was input (with AMediaCodec_queueInputBuffer), it was marked EOS
                OMAF_LOG_I("Video stream output reached EOS");
                freeCodec();
                mState = DecoderHWState::EOS;
            }
        }

        // Handle outputbuffers that don't contain video data
        if (nextFrameOutputBufferIndex < 0 && nextFrameOutputBufferIndex != AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            if (nextFrameOutputBufferIndex == AMEDIACODEC_INFO_OUTPUT_BUFFERS_CHANGED)
            {
                OMAF_LOG_V("%d Output buffers changed", mDecoderConfig.streamId);
            }
            else if (nextFrameOutputBufferIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
            {
                AMediaFormat* format = AMediaCodec_getOutputFormat(mCodec->codec);
                OMAF_LOG_V("Format of %d changed to: %s", mDecoderConfig.streamId, AMediaFormat_toString(format));
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
    DecoderFrameAndroid* frameAndroid = (DecoderFrameAndroid*) frame;
    OMAF_ASSERT(mOutputTexture.surface != OMAF_NULL, "No output texture set");
    if (frameAndroid->decoderOutputIndex == -1)
    {
        return;
    }
    // OMAF_LOG_D("%d releaseOutputBuffer %llu", frame->streamId, frame->pts);
    if (mCodec != OMAF_NULL)
    {
        // release and upload only if codec was not already liberated...
        media_status_t mediaStatus =
            AMediaCodec_releaseOutputBuffer(mCodec->codec, (size_t) frameAndroid->decoderOutputIndex, upload);
        mCodec->lastDecodedFrameIndex = -1;
        if (mediaStatus != AMEDIA_OK)
        {
            OMAF_LOG_W("MediaCodec releaseOutputBuffer() %d, error = %d", frameAndroid->decoderOutputIndex,
                       mediaStatus);
        }
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