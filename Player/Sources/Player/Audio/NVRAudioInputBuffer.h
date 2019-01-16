
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

#include "Audio/NVRAudioTypes.h"
#include "Provider/NVRCoreProviderSources.h"
#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

    struct AudioSourceMetadata1;
    class MP4VRMediaPacket;

    namespace AudioState
    {
        enum Enum
        {
            INVALID = -1,
            IDLE,
            PLAYING,
            PAUSED,

            COUNT
        };
    }
    class AudioInputBuffer;
    class AudioInputBufferObserver
    {
    public:
        
        AudioInputBufferObserver() {}
        virtual ~AudioInputBufferObserver() {}
        
        virtual void_t onPlaybackStarted(AudioInputBuffer* caller, int64_t bufferingOffsetUs) = 0;
        virtual void_t onSamplesConsumed(AudioInputBuffer* caller, streamid_t streamId) = 0;
        virtual void_t onOutOfSamples(AudioInputBuffer* caller, streamid_t streamId) = 0;
        virtual void_t onAudioErrorOccurred(AudioInputBuffer* caller, Error::Enum error) = 0;
    };

    class AudioInputBuffer
    {
    public:
        
        AudioInputBuffer() {}
        virtual ~AudioInputBuffer() {}
        
        virtual Error::Enum initializeForEncodedInput(streamid_t nrStreams, uint8_t* codecData, size_t dataSize) = 0;
        virtual Error::Enum initializeForNoAudio() = 0;

        virtual Error::Enum write(streamid_t streamId, MP4VRMediaPacket* packet) = 0;

        virtual void_t startPlayback() = 0;
        virtual void_t stopPlayback() = 0;

        virtual AudioState::Enum getState() = 0;

        virtual AudioReturnValue::Enum flush() = 0;

        virtual void_t setObserver(AudioInputBufferObserver* observer) = 0;
        virtual void_t removeObserver(AudioInputBufferObserver* observer) = 0;

        virtual bool_t isInitialized() const = 0;

        virtual uint64_t getElapsedTimeUs() const = 0;

        virtual AudioInputBuffer* getAuxiliaryAudioInputBuffer() = 0;

    };
OMAF_NS_END
