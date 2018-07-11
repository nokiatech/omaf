
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
#include <NVRNamespace.h>
#include "Platform/OMAFPlatformDetection.h"
#include <NVRErrorCodes.h>
#include "NVRNullAudioBackend.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(NullBackend)

NullBackend::NullBackend()
{
}

NullBackend::~NullBackend()
{
}

void NullBackend::init(AudioRendererAPI *renderer, bool_t allowExclusiveMode, const wchar_t* audioDevice)
{
}

void NullBackend::shutdown()
{
}

void NullBackend::onErrorOccurred(OMAF_NS::Error::Enum err)
{
}

int64_t NullBackend::onGetPlayedTimeUs()
{
    return 0;
}

void NullBackend::onFlush()
{
}

void_t NullBackend::onRendererPaused()
{
}

void NullBackend::onRendererReady()
{
}

void NullBackend::onRendererPlaying()
{
}

OMAF_NS_END
