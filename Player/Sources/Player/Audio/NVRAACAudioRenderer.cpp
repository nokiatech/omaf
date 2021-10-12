
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
#include "Audio/NVRAACAudioRenderer.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "Media/NVRMediaPacket.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(AACAudioRenderer)

uint32_t sampleRateFromAACSampleRateIndex(uint8_t sampleRateIndex)
{
    switch (sampleRateIndex)
    {
    case 0:
        return 96000;
    case 1:
        return 88200;
    case 2:
        return 64000;
    case 3:
        return 48000;
    case 4:
        return 44100;
    case 5:
        return 32000;
    case 6:
        return 24000;
    case 7:
        return 22050;
    case 8:
        return 16000;
    case 9:
        return 12000;
    case 10:
        return 11025;
    case 11:
        return 8000;
    case 12:
        return 7350;
    default:
        return -1;
    }
}

AACAudioRenderer::AACAudioRenderer(MemoryAllocator& allocator, AudioRendererObserver& observer)
    : AudioRendererAPI(allocator, observer)
    , mMainAACBuffers(allocator, 10000)
    , mMaxBufferSize(0)
    , mObserver(observer)
    , mSampleRate(-1)
    , mAudioInputObservers(allocator)
    , mInitialized(false)
    , mPlaying(false)
    , mMainStreamId(InvalidStreamId)
{
}

AACAudioRenderer::~AACAudioRenderer()
{
    reset();
}

Error::Enum AACAudioRenderer::init()
{
    return Error::OK;
}
Error::Enum AACAudioRenderer::reset()
{
    OMAF_LOG_V("reset");
    mInitialized = false;
    if (mPlaying)
    {
        mObserver.onRendererPaused();
        mPlaying = false;
    }

    while (mMainAACBuffers.hasFilledPackets())
    {
        mMainAACBuffers.pushEmptyPacket(mMainAACBuffers.popFilledPacket());
    }
    mMainAACBuffers.reset();
    mMainStreamId = InvalidStreamId;
    while (!mAdditionalAudios.isEmpty())
    {
        AdditionalAudio* audio = mAdditionalAudios.front();
        while (audio->mAACBuffers.hasFilledPackets())
        {
            audio->mAACBuffers.pushEmptyPacket(audio->mAACBuffers.popFilledPacket());
        }
        audio->mAACBuffers.reset();
        mAdditionalAudios.remove(audio);
        OMAF_DELETE_HEAP(audio);
    }
    return Error::OK;
}

bool_t AACAudioRenderer::consumeAACFrame(MediaPacketQueue& aBufferQueue,
                                         uint8_t* aOutputBuffer,
                                         size_t aBufferSize,
                                         size_t& aDataSize)
{
    MP4VRMediaPacket* buf = aBufferQueue.peekFilledPacket();
    OMAF_ASSERT(buf != OMAF_NULL, "AAC Buffer queue is empty!");
    if (aOutputBuffer != OMAF_NULL)
    {
        if (aBufferSize < buf->dataSize())
        {
            return false;
        }
        OMAF_ASSERT(buf->dataSize() > 0, "Empty AAC data");
        buf->copyTo(0, aOutputBuffer, aBufferSize);
        aDataSize = buf->dataSize();
    }
    aBufferQueue.popFilledPacket();
    aBufferQueue.pushEmptyPacket(buf);
    return true;
}

AudioReturnValue::Enum AACAudioRenderer::fetchAACFrame(uint8_t* aBuffer, size_t aBufferSize, size_t& aDataSize)
{
    if (!mInitialized || !mPlaying)
    {
        return AudioReturnValue::NOT_INITIALIZED;
    }
    // take oldest input AAC buffer
    mBufferLock.lock();
    bool_t consumed = false;
    if (!mAdditionalAudios.isEmpty())
    {
        size_t index = 0;
        for (; index < mAdditionalAudios.getSize(); index++)
        {
            if (mAdditionalAudios.at(index)->mAACBuffers.hasFilledPackets())
            {
                // OMAF_LOG_V("Consume overlay AAC %zd out of %zd", index, mAdditionalAudios.getSize());
                if (!consumeAACFrame(mAdditionalAudios.at(index)->mAACBuffers, aBuffer, aBufferSize, aDataSize))
                {
                    mBufferLock.unlock();
                    return AudioReturnValue::FAIL;
                }
                consumed = true;
                break;
            }
        }
        if (consumed)
        {
            // to maintain AV sync for background audio, consume one buffer from all other streams too
            size_t tmp;
            index++;
            for (; index < mAdditionalAudios.getSize(); index++)
            {
                if (mAdditionalAudios.at(index)->mAACBuffers.hasFilledPackets())
                {
                    consumeAACFrame(mAdditionalAudios.at(index)->mAACBuffers, OMAF_NULL, 0, tmp);
                }
            }
            if (mMainStreamId != InvalidStreamId)
            {
                // OMAF_LOG_V("Skip background AAC");
                if (mMainAACBuffers.hasFilledPackets())
                {
                    consumeAACFrame(mMainAACBuffers, OMAF_NULL, 0, tmp);
                }
            }
        }
        else if (mMainStreamId != InvalidStreamId)
        {
            // return AudioReturnValue::OUT_OF_SAMPLES;
            // is it safe to assume this stream has stopped producing samples, and fallback to the background audio?
            // OMAF_LOG_D("No data in additional, switch to main stream");
            if (!mMainAACBuffers.hasFilledPackets())
            {
                mBufferLock.unlock();
                return AudioReturnValue::OUT_OF_SAMPLES;
            }
            // OMAF_LOG_V("Consume background AAC");
            if (!consumeAACFrame(mMainAACBuffers, aBuffer, aBufferSize, aDataSize))
            {
                mBufferLock.unlock();
                return AudioReturnValue::FAIL;
            }
            consumed = true;
        }
    }
    else
    {
        if (!mMainAACBuffers.hasFilledPackets())
        {
            mBufferLock.unlock();
            return AudioReturnValue::OUT_OF_SAMPLES;
        }
        // OMAF_LOG_V("Consume background AAC");
        if (!consumeAACFrame(mMainAACBuffers, aBuffer, aBufferSize, aDataSize))
        {
            mBufferLock.unlock();
            return AudioReturnValue::FAIL;
        }
        consumed = true;
    }

    if (!consumed)
    {
        mBufferLock.unlock();
        OMAF_LOG_D("Out of samples");
        return AudioReturnValue::OUT_OF_SAMPLES;
    }
    for (Array<AudioInputBufferObserver*>::Iterator obs = mAudioInputObservers.begin();
         obs != mAudioInputObservers.end(); ++obs)
    {
        (*obs)->onSamplesConsumed(this, mMainStreamId);
        for (AdditionalAudioStreams::Iterator add = mAdditionalAudios.begin(); add != mAdditionalAudios.end(); ++add)
        {
            (*obs)->onSamplesConsumed(this, (*add)->mStreamId);
        }
    }

    mBufferLock.unlock();
    return AudioReturnValue::OK;
}

uint32_t AACAudioRenderer::getInputSampleRate()
{
    return mSampleRate;
}

uint32_t AACAudioRenderer::getInputChannels()
{
    return mInputChannels;
}

void_t AACAudioRenderer::playbackStarted()
{
    if (!mAudioInputObservers.isEmpty())
    {
        for (Array<AudioInputBufferObserver*>::Iterator it = mAudioInputObservers.begin();
             it != mAudioInputObservers.end(); ++it)
        {
            (*it)->onPlaybackStarted(this, 0);
        }
    }
}

bool_t AACAudioRenderer::isReady() const
{
    return true;
}

AudioInputBuffer* AACAudioRenderer::getAudioInputBuffer()
{
    return this;
}

void_t AACAudioRenderer::setAudioVolume(float_t volume)
{
}

Error::Enum AACAudioRenderer::initializeForEncodedInput(streamid_t aStreamId, uint8_t* codecData, size_t dataSize)
{
    if (dataSize != 2 && dataSize != 5)
    {
        return Error::INVALID_DATA;
    }
    if (mInitialized)
    {
        reset();
    }

    uint16_t aacHeader = (((uint16_t) codecData[0]) << 8) + codecData[1];
    mSampleRate = sampleRateFromAACSampleRateIndex((aacHeader >> 7) & 0b00011111);
    mInputChannels = (aacHeader >> 3) & 0b00001111;

    mInitialized = true;
    mObserver.onFlush();
    mObserver.onRendererReady();
    mMainStreamId = aStreamId;

    return Error::OK;
}

Error::Enum AACAudioRenderer::initializeForNoAudio()
{
    if (mInitialized)
    {
        reset();
    }
    return Error::OK;
}


Error::Enum AACAudioRenderer::addStream(streamid_t aStreamId, uint8_t* codecData, size_t dataSize)
{
    OMAF_LOG_V("addStream %d", aStreamId);
    if (aStreamId == InvalidStreamId)
    {
        return Error::INVALID_DATA;
    }
    if (dataSize != 2 && dataSize != 5)
    {
        return Error::INVALID_DATA;
    }

    uint16_t aacHeader = (((uint16_t) codecData[0]) << 8) + codecData[1];
    uint32_t sampleRate = sampleRateFromAACSampleRateIndex((aacHeader >> 7) & 0b00011111);
    uint32_t inputChannels = (aacHeader >> 3) & 0b00001111;

    if (mInitialized)
    {
        if (sampleRate != mSampleRate || inputChannels != mInputChannels)
        {
            // audio parameters must match with the background
            OMAF_LOG_W("Audio samplerate (%d) / channels (%d) don't match", sampleRate, inputChannels);
            return Error::NOT_SUPPORTED;
        }
    }

    AdditionalAudio* audio = OMAF_NEW_HEAP(AdditionalAudio);
    audio->mStreamId = aStreamId;
    // add newest to front ^= stack
    mAdditionalAudios.add(audio, 0);
    OMAF_LOG_V("# of audios now %zd", mAdditionalAudios.getSize());
    if (mMainStreamId == InvalidStreamId && !mInitialized)
    {
        // no background audio
        mSampleRate = sampleRateFromAACSampleRateIndex((aacHeader >> 7) & 0b00011111);
        mInputChannels = (aacHeader >> 3) & 0b00001111;

        mInitialized = true;

        mObserver.onFlush();
        mObserver.onRendererReady();
    }
    return Error::OK;
}

Error::Enum AACAudioRenderer::removeStream(streamid_t aStreamId, bool_t& aNeedToPausePlayback)
{
    OMAF_LOG_V("removeStream %d", aStreamId);
    mBufferLock.lock();
    for (AdditionalAudioStreams::Iterator it = mAdditionalAudios.begin(); it != mAdditionalAudios.end(); ++it)
    {
        if ((*it)->mStreamId == aStreamId)
        {
            AdditionalAudio* audio = *it;
            while (audio->mAACBuffers.hasFilledPackets())
            {
                audio->mAACBuffers.pushEmptyPacket(audio->mAACBuffers.popFilledPacket());
            }
            audio->mAACBuffers.reset();
            mAdditionalAudios.remove(audio);
            OMAF_LOG_V("# of audios now %zd", mAdditionalAudios.getSize());
            OMAF_DELETE_HEAP(audio);
            break;
        }
    }
    mBufferLock.unlock();
    if (mAdditionalAudios.isEmpty() && mMainStreamId == InvalidStreamId)
    {
        // continue playing
        aNeedToPausePlayback = true;
    }
    else
    {
        aNeedToPausePlayback = false;
    }
    return Error::OK;
}

Error::Enum AACAudioRenderer::removeAllStreams()
{
    OMAF_LOG_D("removeAllStreams");
    mMainStreamId = InvalidStreamId;
    while (mMainAACBuffers.hasFilledPackets())
    {
        mMainAACBuffers.pushEmptyPacket(mMainAACBuffers.popFilledPacket());
    }
    mMainAACBuffers.reset();

    while (!mAdditionalAudios.isEmpty())
    {
        bool_t pause;
        removeStream(mAdditionalAudios.at(0)->mStreamId, pause);
    }
    mInitialized = false;
    return Error::OK;
}

void_t AACAudioRenderer::startPlayback()
{
    if (mInitialized && !mPlaying)
    {
        mPlaying = true;
        mObserver.onRendererPlaying();
    }
}

void_t AACAudioRenderer::stopPlayback()
{
    if (mPlaying)
    {
        mPlaying = false;

        mObserver.onRendererPaused();
    }
}

AudioState::Enum AACAudioRenderer::getState()
{
    if (mPlaying)
    {
        return AudioState::PLAYING;
    }
    return AudioState::IDLE;
}

bool_t AACAudioRenderer::storeAACFrame(MediaPacketQueue& aBufferQueue, MP4VRMediaPacket* aPacket)
{
    if (aBufferQueue.filledPacketsSize() + 1 < aBufferQueue.getCapacity())
    {
        MP4VRMediaPacket* buf;
        if (aPacket->dataSize() > mMaxBufferSize)
        {
            mMaxBufferSize = aPacket->dataSize();
        }

        if (!aBufferQueue.hasEmptyPackets(1))
        {
            buf = OMAF_NEW_HEAP(MP4VRMediaPacket)(mMaxBufferSize);
        }
        else
        {
            buf = aBufferQueue.popEmptyPacket();
            if (buf->bufferSize() < aPacket->dataSize())
            {
                buf->resizeBuffer(aPacket->dataSize());
            }
        }

        aPacket->copyTo(0, buf->buffer(), buf->bufferSize());
        buf->setDataSize(aPacket->dataSize());
        aBufferQueue.pushFilledPacket(buf);
        return true;
    }
    else
    {
        return false;
    }
}

Error::Enum AACAudioRenderer::write(streamid_t aStreamId, MP4VRMediaPacket* aPacket)
{
    Spinlock lock(mBufferLock);
    if (aStreamId == mMainStreamId)
    {
        if (!storeAACFrame(mMainAACBuffers, aPacket))
        {
            return Error::BUFFER_OVERFLOW;
        }
    }
    else
    {
        for (AdditionalAudioStreams::Iterator it = mAdditionalAudios.begin(); it != mAdditionalAudios.end(); ++it)
        {
            if ((*it)->mStreamId == aStreamId)
            {
                if (!storeAACFrame((*it)->mAACBuffers, aPacket))
                {
                    return Error::BUFFER_OVERFLOW;
                }
            }
        }
    }
    return Error::OK;
}

AudioReturnValue::Enum AACAudioRenderer::flush()
{
    Spinlock lock(mBufferLock);
    while (mMainAACBuffers.hasFilledPackets())
    {
        mMainAACBuffers.pushEmptyPacket(mMainAACBuffers.popFilledPacket());
    }
    for (AdditionalAudioStreams::Iterator it = mAdditionalAudios.begin(); it != mAdditionalAudios.end(); ++it)
    {
        while ((*it)->mAACBuffers.hasFilledPackets())
        {
            (*it)->mAACBuffers.pushEmptyPacket((*it)->mAACBuffers.popFilledPacket());
        }
    }
    mObserver.onFlush();
    return AudioReturnValue::OK;
}

bool_t AACAudioRenderer::isInitialized() const
{
    return mInitialized;
}

uint64_t AACAudioRenderer::getElapsedTimeUs() const
{
    // if there's only overlay audio, no background audio, it should not be used for synching,
    // as it may not cover the whole duration of the clip and hence can create issues with drift calculations
    if (mMainStreamId != InvalidStreamId)
    {
        return mObserver.onGetPlayedTimeUs();
    }
    else
    {
        return 0;
    }
}

void_t AACAudioRenderer::setObserver(AudioInputBufferObserver* observer)
{
    mAudioInputObservers.add(observer);
}
void_t AACAudioRenderer::removeObserver(AudioInputBufferObserver* observer)
{
    mAudioInputObservers.remove(observer);
}

OMAF_NS_END
