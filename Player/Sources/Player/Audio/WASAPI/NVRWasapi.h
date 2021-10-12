
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

#include "Audio/NVRAudioBackend.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
namespace WASAPIImpl
{
    class Context;
}

class AudioRenderer;
class MemoryAllocator;

class WASAPIBackend : public AudioBackend
{
public:
    WASAPIBackend();
    virtual ~WASAPIBackend();

public:  // AudioBackend
    virtual void_t init(AudioRendererAPI* renderer, bool_t allowExclusiveMode, const wchar_t* audioDevice);

    virtual void_t shutdown();

public:  // AudioRendererObserver
    virtual void_t onRendererReady();

    virtual void_t onRendererPlaying();

    virtual void_t onRendererPaused();

    virtual void_t onFlush();

    virtual int64_t onGetPlayedTimeUs();

    virtual void_t onErrorOccurred(Error::Enum error);

private:
    WASAPIImpl::Context* mContext;
    MemoryAllocator& mAllocator;
};
OMAF_NS_END
