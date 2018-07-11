
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


OMAF_NS_BEGIN
    class NullAudioRenderer : public AudioRendererAPI
    {
    public:

        NullAudioRenderer(MemoryAllocator& allocator, AudioRendererObserver& observer);
        virtual ~NullAudioRenderer();

        virtual Error::Enum init(int8_t outChannels, PlaybackMode::Enum playbackMode, AudioOutputRange::Enum outputRange);
        virtual Error::Enum reset();

        virtual AudioReturnValue::Enum renderSamples(size_t samplesToRender, float32_t* samples, size_t& samplesRendered, int32_t timeout = 0);
        virtual AudioReturnValue::Enum renderSamples(size_t samplesToRender, int16_t* samples, size_t& samplesRendered, int32_t timeout = 0);
        virtual void_t playbackStarted();

        virtual int32_t getInputSampleRate() const;
        virtual bool_t isReady() const;

        virtual AudioInputBuffer* getAudioInputBuffer();

        virtual Error::Enum setAudioLatency(int64_t latencyUs);

        virtual void_t setAudioVolume(float_t volume);
        virtual void_t setAudioVolumeAuxiliary(float_t volume);

        virtual void_t setHeadTransform(const Quaternion& headTransform);

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

    };

OMAF_NS_END