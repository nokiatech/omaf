
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
#include "Audio/NVRNullAudioRenderer.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"


OMAF_NS_BEGIN
    OMAF_LOG_ZONE(NullAudioRenderer)



    NullAudioRenderer::NullAudioRenderer(MemoryAllocator& allocator, AudioRendererObserver& observer)
        : AudioRendererAPI(allocator, observer)
    {
    }

    NullAudioRenderer::~NullAudioRenderer()
    {
    }

    Error::Enum NullAudioRenderer::init(int8_t outChannels, PlaybackMode::Enum playbackMode, AudioOutputRange::Enum outputRange)
    {
        return Error::OK;
    }
    Error::Enum NullAudioRenderer::reset()
    {
        return Error::OK;
    }

    AudioReturnValue::Enum NullAudioRenderer::renderSamples(size_t samplesToRender, float32_t* samples, size_t& samplesRendered, int32_t timeout)
    {
        return AudioReturnValue::OK;
    }

    AudioReturnValue::Enum NullAudioRenderer::renderSamples(size_t samplesToRender, int16_t* samples, size_t& samplesRendered, int32_t timeout)
    {
        return AudioReturnValue::OK;
    }

    void_t NullAudioRenderer::playbackStarted()
    {

    }

    int32_t NullAudioRenderer::getInputSampleRate() const
    {
        return 48000;
    }
    bool_t NullAudioRenderer::isReady() const
    {
        return true;
    }

    AudioInputBuffer* NullAudioRenderer::getAudioInputBuffer()
    {
        return this;
    }

    Error::Enum NullAudioRenderer::setAudioLatency(int64_t latencyUs)
    {
        return Error::OK;
    }

    void_t NullAudioRenderer::setAudioVolume(float_t volume)
    {

    }
    void_t NullAudioRenderer::setAudioVolumeAuxiliary(float_t volume)
    {

    }

    void_t NullAudioRenderer::setHeadTransform(const Quaternion& headTransform)
    {

    }

    Error::Enum NullAudioRenderer::initializeForEncodedInput(streamid_t nrStreams, uint8_t* codecData, size_t dataSize)
    {
        return Error::NOT_SUPPORTED;
    }

    Error::Enum NullAudioRenderer::initializeForNoAudio()
    {
        return Error::OK;
    }

    void_t NullAudioRenderer::startPlayback()
    {
    }

    void_t NullAudioRenderer::stopPlayback()
    {
    }

    AudioState::Enum NullAudioRenderer::getState()
    {
        return AudioState::IDLE;
    }

    Error::Enum NullAudioRenderer::write(streamid_t streamId, MP4VRMediaPacket* packet)
    {
        return Error::OK;
    }

    AudioReturnValue::Enum NullAudioRenderer::flush()
    {
        return AudioReturnValue::OK;
    }

    bool_t NullAudioRenderer::isInitialized() const
    {
        return false;
    }

    uint64_t NullAudioRenderer::getElapsedTimeUs() const
    {
        return 0;
    }

    void_t NullAudioRenderer::setObserver(AudioInputBufferObserver *observer)
    {
    }
    void_t NullAudioRenderer::removeObserver(AudioInputBufferObserver *observer)
    {
    }

    AudioInputBuffer* NullAudioRenderer::getAuxiliaryAudioInputBuffer()
    {
        return this;
   } 

OMAF_NS_END
