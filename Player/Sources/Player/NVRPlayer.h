
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

#include "Graphics/NVRRendererType.h"
#include "Renderer/NVRRenderingManager.h"

#include "Provider/NVRVideoProvider.h"
#include "Provider/NVRCoreProvider.h"

#include "Audio/NVRAudioInputBuffer.h"
#include "Audio/NVRAudioRendererAPI.h"
#include "Audio/NVRAudioTypes.h"
#include "Audio/NVRAudioBackend.h"

namespace OMAF
{
    struct PlatformParameters;
}

OMAF_NS_BEGIN
    Error::Enum initialize(RendererType::Enum rendererType, OMAF::PlatformParameters* platformParameters = OMAF_NULL, MemoryAllocator* clientHeapAllocator = OMAF_NULL);
    void_t deinitialize();
        
    RenderingManager* createRenderingManager();
    void_t destroyRenderingManager(RenderingManager* presenceRenderer);
    
    AudioRendererAPI* createAudioRenderer(AudioRendererObserver* observer);
    void_t destroyAudioRenderer(AudioRendererAPI* audioRenderer);
    
    VideoProvider* createVideoProvider();
    void_t destroyVideoProvider(VideoProvider* localVideoProvider);

    AudioBackend* createAudioBackend();
    void_t destroyAudioBackend(AudioBackend* audioBackend);
OMAF_NS_END
