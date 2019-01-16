
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Math/OMAFMathTypes.h"
#include "Foundation/NVRAtomicInteger.h"
#include "Foundation/NVRMemoryAllocator.h"
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

        virtual Error::Enum init() = 0;
        virtual Error::Enum reset() = 0;

        virtual AudioReturnValue::Enum fetchAACFrame(uint8_t* aBuffer, size_t aBufferSize, size_t& aDataSize) = 0;
        virtual uint32_t getInputSampleRate() = 0;
        virtual uint32_t getInputChannels() = 0;

        virtual void_t playbackStarted() = 0;

        virtual bool_t isReady() const = 0;

        virtual AudioInputBuffer* getAudioInputBuffer() = 0;

        virtual void_t setAudioVolume(float_t volume) = 0;
        virtual void_t setAudioVolumeAuxiliary(float_t volume) = 0;
		
    };
OMAF_NS_END
