
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

// Platform specific
#if OMAF_PLATFORM_ANDROID

    #include <EGL/egl.h>
    #include <EGL/eglext.h>

    // https://www.khronos.org/registry/OpenGL/index_es.php

    #if OMAF_GRAPHICS_API_OPENGL_ES >= 32

        #include <GLES3/gl32.h>
        #include <GLES2/gl2ext.h>

    #elif OMAF_GRAPHICS_API_OPENGL_ES >= 31

        #include <GLES3/gl31.h>
        #include <GLES2/gl2ext.h>

    #elif OMAF_GRAPHICS_API_OPENGL_ES >= 30

        #include <GLES3/gl3.h>
        #include <GLES2/gl2ext.h>

    #elif OMAF_GRAPHICS_API_OPENGL_ES >= 20

        #include <GLES2/gl2.h>
        #include <GLES2/gl2ext.h>

    #else

        #error Invalid OpenGL ES version

    #endif

#elif OMAF_PLATFORM_UWP

#define D3D11_NO_HELPERS

    #include "Foundation\NVRDependencies.h"
    #include <d3d11_1.h>
    #include <d3dcompiler.h>
    #include <d3d11shader.h>

    #include <comdef.h>

#elif OMAF_PLATFORM_WINDOWS
#define NOMINMAX

#include <windows.h>
    #if OMAF_GRAPHICS_API_OPENGL
        #include <gl/gl.h>
        #include <gl/glext.h>
        #include <wgl/wglext.h>

    #endif

    #if OMAF_GRAPHICS_API_D3D11

        #include <sal.h>

        #define D3D11_NO_HELPERS

        #include <d3d11_1.h>

        #include <d3dcompiler.h>
        #include <d3d11shader.h>

        #include <comdef.h>

    #endif

    #if OMAF_GRAPHICS_API_VULKAN

        #include <vulkan/vulkan.h>

    #endif

#else

    #error Unsupported platform

#endif
