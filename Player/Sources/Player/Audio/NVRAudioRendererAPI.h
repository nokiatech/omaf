
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
#include "Math/OMAFMathTypes.h"
#include "Foundation/NVRAtomicInteger.h"

#include "NVRErrorCodes.h"
#include "Audio/NVRAudioInputBuffer.h"

OMAF_NS_BEGIN
    class AudioRendererObserver;
    class AuxiliaryAudioSource;

    namespace InputType
    {
        enum Enum
        {
            INVALID = -1,
            PCM,
            AAC,
            SPATIAL_AUDIO,

            COUNT
        };
    }
    class AudioRendererAPI
    : public AudioInputBuffer
    {
    public:

        AudioRendererAPI(MemoryAllocator& allocator, AudioRendererObserver& observer) {};
        virtual ~AudioRendererAPI() {};

    public:

        virtual Error::Enum init(int8_t outChannels, PlaybackMode::Enum playbackMode, AudioOutputRange::Enum outputRange) = 0;
        virtual Error::Enum reset() = 0;

        /**
         * @brief renderSamples Returns interleaved samples ready for playback.
         * @param samplesToRender How many samples to read. Usually this would be multiple of channels()
         * @param samples Pointer where to write samples. Sample is in in range [-1, +1] inclusive.
         * @param samplesRendered Number of samples actually read is written here. May be less than samplesToRead,
         *   if data is not available. In case of OK, END_OF_FILE or OUT_OF_SAMPLES this many samples are now valid,
         *   remember to use to those before quitting / waiting. In case of other errors, the value is must be set to valid number, but can be zero.
         * @param timeout How many milliseconds to wait for samples to become available before returning OUT_OF_SAMPLES. Negative value to wait forever. Implementation may choose not to use this value.
         * @return One value specified by return value enum.
         */
        virtual AudioReturnValue::Enum renderSamples(size_t samplesToRender, float32_t* samples, size_t& samplesRendered, int32_t timeout = 0) = 0;
        virtual AudioReturnValue::Enum renderSamples(size_t samplesToRender, int16_t* samples, size_t& samplesRendered, int32_t timeout = 0) = 0;
        virtual void_t playbackStarted() = 0;

        virtual int32_t getInputSampleRate() const = 0;
        virtual bool_t isReady() const = 0;

        virtual AudioInputBuffer* getAudioInputBuffer() = 0;

        virtual Error::Enum setAudioLatency(int64_t latencyUs) = 0;

        virtual void_t setAudioVolume(float_t volume) = 0;
        virtual void_t setAudioVolumeAuxiliary(float_t volume) = 0;

        virtual void_t setHeadTransform(const Quaternion& headTransform) = 0;
		
    };
OMAF_NS_END
