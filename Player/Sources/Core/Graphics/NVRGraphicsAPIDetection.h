
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

#include "NVREssentials.h"

OMAF_NS_BEGIN

// Supported graphics APIs
#if OMAF_PLATFORM_ANDROID

    #define OMAF_SUPPORTS_GRAPHICS_API_OPENGL_ES 1

    #define OMAF_SUPPORTS_GRAPHICS_API_VULKAN 1

#elif OMAF_PLATFORM_WINDOWS

    #define OMAF_SUPPORTS_GRAPHICS_API_OPENGL 1

    // E.g. simulated via PowerVR, Mali SDK
    #define OMAF_SUPPORTS_GRAPHICS_API_OPENGL_ES 1

    #define OMAF_SUPPORTS_GRAPHICS_API_VULKAN 1

    #define OMAF_SUPPORTS_GRAPHICS_API_D3D11 1

    #define OMAF_SUPPORTS_GRAPHICS_API_D3D12 1

#elif OMAF_PLATFORM_UWP

    #define OMAF_SUPPORTS_GRAPHICS_API_OPENGL 0
    #define OMAF_SUPPORTS_GRAPHICS_API_OPENGL_ES 0
    #define OMAF_SUPPORTS_GRAPHICS_API_VULKAN 0
    #define OMAF_SUPPORTS_GRAPHICS_API_D3D11 1
    #define OMAF_SUPPORTS_GRAPHICS_API_D3D12 0

#endif

// Define emptys
#ifndef OMAF_GRAPHICS_API_OPENGL_ES

    #define OMAF_GRAPHICS_API_OPENGL_ES 0

#endif

#ifndef OMAF_GRAPHICS_API_OPENGL

    #define OMAF_GRAPHICS_API_OPENGL 0

#endif

#ifndef OMAF_GRAPHICS_API_METAL

    #define OMAF_GRAPHICS_API_METAL 0

#endif

#ifndef OMAF_GRAPHICS_API_VULKAN

    #define OMAF_GRAPHICS_API_VULKAN 0

#endif

#ifndef OMAF_GRAPHICS_API_D3D11

    #define OMAF_GRAPHICS_API_D3D11 0

#endif

#ifndef OMAF_GRAPHICS_API_D3D12

    #define OMAF_GRAPHICS_API_D3D12 0

#endif

OMAF_NS_END
