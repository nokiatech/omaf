
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "NVRErrorCodes.h"

#include "Audio/NVRAudioRendererAPI.h"
#include "Audio/NVRAudioRendererObserver.h"
#include "Core/Foundation/NVRSpinlock.h"
#include "Media/NVRMediaPacketQueue.h"
#include "Foundation/NVRArray.h"


OMAF_NS_BEGIN
    const int32_t kMaxAacInputBuffers = 100;

    class AACAudioRenderer : public AudioRendererAPI
    {
    public:

        AACAudioRenderer(MemoryAllocator& allocator, AudioRendererObserver& observer);
        virtual ~AACAudioRenderer();

        virtual Error::Enum init();
        virtual Error::Enum reset();

        virtual AudioReturnValue::Enum fetchAACFrame(uint8_t* aBuffer, size_t aBufferSize, size_t& aDataSize);
        virtual uint32_t getInputSampleRate();
        virtual uint32_t getInputChannels();
        virtual void_t playbackStarted();

        virtual bool_t isReady() const;

        virtual AudioInputBuffer* getAudioInputBuffer();

        virtual void_t setAudioVolume(float_t volume);
        virtual void_t setAudioVolumeAuxiliary(float_t volume);

        virtual Error::Enum initializeForEncodedInput(streamid_t nrStreams, uint8_t* codecData, size_t dataSize);
        virtual Error::Enum initializeForNoAudio();

        virtual Error::Enum write(streamid_t streamId, MP4VRMediaPacket* packet);

        virtual void_t startPlayback();
        virtual void_t stopPlayback();

        virtual AudioState::Enum getState();

        virtual AudioReturnValue::Enum flush();

        virtual void_t setObserver(AudioInputBufferObserver* observer);
        virtual void_t removeObserver(AudioInputBufferObserver* observer);

        virtual bool_t isInitialized() const;

        virtual uint64_t getElapsedTimeUs() const;

        virtual AudioInputBuffer* getAuxiliaryAudioInputBuffer();

    private:
        MediaPacketQueue mAACBuffers;
        Spinlock mBufferLock;
        size_t mMaxBufferSize;
        AudioRendererObserver& mObserver;
        uint32_t mSampleRate;
        uint32_t mInputChannels;
        Array<AudioInputBufferObserver *> mAudioInputObservers;
        bool_t mInitialized;
        streamid_t mStreamId;
        bool_t mPlaying;
    };

OMAF_NS_END
