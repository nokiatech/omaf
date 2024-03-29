
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
#pragma once

#include "NVRNamespace.h"

#include "Audio/NVRAudioRendererObserver.h"

OMAF_NS_BEGIN

class AudioRendererAPI;

class AudioBackend : public AudioRendererObserver
{
public:
    virtual ~AudioBackend()
    {
    }

    virtual void_t init(AudioRendererAPI* renderer, bool_t allowExclusiveMode, const wchar_t* audioDevice) = 0;

    virtual void_t shutdown() = 0;

public:  // AudioRendererObserver
    virtual void_t onRendererReady() = 0;

    virtual void_t onRendererPlaying() = 0;

    virtual void_t onRendererPaused() = 0;

    virtual void_t onFlush() = 0;

    virtual int64_t onGetPlayedTimeUs() = 0;

    virtual void_t onErrorOccurred(Error::Enum error) = 0;
};

OMAF_NS_END
