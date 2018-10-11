
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
#include "Audio/NVRAACAudioRenderer.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "Media/NVRMediaPacket.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(AACAudioRenderer)

    uint32_t sampleRateFromAACSampleRateIndex(uint8_t sampleRateIndex) {
        switch (sampleRateIndex) {
        case 0: return 96000;
        case 1: return 88200;
        case 2: return 64000;
        case 3: return 48000;
        case 4: return 44100;
        case 5: return 32000;
        case 6: return 24000;
        case 7: return 22050;
        case 8: return 16000;
        case 9: return 12000;
        case 10: return 11025;
        case 11: return 8000;
        case 12: return 7350;
        default: return -1;
        }
    }

    AACAudioRenderer::AACAudioRenderer(MemoryAllocator& allocator, AudioRendererObserver& observer)
        : AudioRendererAPI(allocator, observer)
    , mAACBuffers(allocator, 10000)
    , mMaxBufferSize(0)
    , mObserver(observer)
    , mSampleRate(-1)
	, mAudioInputObservers(allocator)
    , mInitialized(false)
    , mPlaying(false)
    {
    }

    AACAudioRenderer::~AACAudioRenderer()
    {
    }

    Error::Enum AACAudioRenderer::init()
    {
        mInitialized = true;
        return Error::OK;
    }
    Error::Enum AACAudioRenderer::reset()
    {
        mInitialized = false;
        mPlaying = false;
        return Error::OK;
    }

    AudioReturnValue::Enum AACAudioRenderer::fetchAACFrame(uint8_t* aBuffer, size_t aBufferSize, size_t& aDataSize)
    {
        // take oldest input AAC buffer
        Spinlock lock(mBufferLock);
        if (!mAACBuffers.hasFilledPackets())
        {
            return AudioReturnValue::OUT_OF_SAMPLES;
        }
        MP4VRMediaPacket* buf = mAACBuffers.peekFilledPacket();
        if (aBufferSize < buf->dataSize())
        {
            return AudioReturnValue::FAIL;
        }
        mAACBuffers.popFilledPacket();
        buf->copyTo(0, aBuffer, aBufferSize);
        aDataSize = buf->dataSize();
        mAACBuffers.pushEmptyPacket(buf);

        for (Array<AudioInputBufferObserver*>::Iterator it = mAudioInputObservers.begin(); it != mAudioInputObservers.end(); ++it)
        {
            (*it)->onSamplesConsumed(this, mStreamId);
        }

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
            for (Array<AudioInputBufferObserver*>::Iterator it = mAudioInputObservers.begin(); it != mAudioInputObservers.end(); ++it)
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
    void_t AACAudioRenderer::setAudioVolumeAuxiliary(float_t volume)
    {

    }

    Error::Enum AACAudioRenderer::initializeForEncodedInput(streamid_t nrStreams, uint8_t* codecData, size_t dataSize)
    {
        if (dataSize != 2) {
            return Error::INVALID_DATA;           
        }

        uint16_t aacHeader  = (((uint16_t)codecData[0]) << 8) + codecData[1];
        mSampleRate = sampleRateFromAACSampleRateIndex((aacHeader >> 7) & 0b00011111);
        mInputChannels = (aacHeader >> 3) & 0b00001111;

        mObserver.onFlush();
        mObserver.onRendererReady();

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

    void_t AACAudioRenderer::startPlayback()
    {
        mObserver.onRendererPlaying();
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
        return AudioState::IDLE;
    }

    Error::Enum AACAudioRenderer::write(streamid_t aStreamId, MP4VRMediaPacket* aPacket)
    {
        Spinlock lock(mBufferLock);
        if (mAACBuffers.filledPacketsSize() + 1 < mAACBuffers.getCapacity())
        {
            MP4VRMediaPacket* buf;
            if (aPacket->dataSize() > mMaxBufferSize)
            {
                mMaxBufferSize = aPacket->dataSize();
            }

            if (!mAACBuffers.hasEmptyPackets(1))
            {
                buf = OMAF_NEW_HEAP(MP4VRMediaPacket)(mMaxBufferSize);
            }
            else
            {
                buf = mAACBuffers.popEmptyPacket();
                if (buf->bufferSize() < aPacket->dataSize())
                {
                    buf->resizeBuffer(aPacket->dataSize());
                }
            }

            aPacket->copyTo(0, buf->buffer(), buf->bufferSize());
            buf->setDataSize(aPacket->dataSize());
            mAACBuffers.pushFilledPacket(buf);

            mPlaying = true;
            mStreamId = aStreamId;
            return Error::OK;
        }
        else
        {
            return Error::BUFFER_OVERFLOW;
        }
    }

    AudioReturnValue::Enum AACAudioRenderer::flush()
    {
        Spinlock lock(mBufferLock);
        while (mAACBuffers.hasFilledPackets())
        {
            mAACBuffers.pushEmptyPacket(mAACBuffers.popFilledPacket());
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
        return mObserver.onGetPlayedTimeUs();
    }

    void_t AACAudioRenderer::setObserver(AudioInputBufferObserver *observer)
    {
        mAudioInputObservers.add(observer);
    }
    void_t AACAudioRenderer::removeObserver(AudioInputBufferObserver *observer)
    {
        mAudioInputObservers.remove(observer);
    }

    AudioInputBuffer* AACAudioRenderer::getAuxiliaryAudioInputBuffer()
    {
        return OMAF_NULL;
   } 

OMAF_NS_END

