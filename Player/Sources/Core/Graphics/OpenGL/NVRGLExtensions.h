
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

#include "NVREssentials.h"

#include "Graphics/NVRConfig.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRGraphicsAPIDetection.h"

#include "Graphics/OpenGL/NVRGLCompatibility.h"

OMAF_NS_BEGIN

// OpenGL / OpenGL ES function prototype loading
#if OMAF_OPENGL_API_IMPORT

#define OMAF_GL_EXTENSION_FUNC_DECL(prototype, function) extern prototype function
#define OMAF_GL_EXTENSION_FUNC_IMPORT(prototype, function) prototype function

#define OMAF_GL_IMPORT OMAF_GL_EXTENSION_FUNC_DECL
#include "Graphics/OpenGL/NVRGLExtensionList.h"
#undef OMAF_GL_IMPORT

#endif

    // OpenGL / OpenGL ES extensions declaration helper
#if OMAF_PLATFORM_WINDOWS
    #define OMAF_EXTENSION_DECL(name) Extension _##name = Extension( #name, false );
#else
    #define OMAF_EXTENSION_DECL(name) Extension _##name = { #name, false };
#endif
    
    struct Extension
    {
#if OMAF_PLATFORM_WINDOWS
        Extension(const char_t* _name, bool_t _supported)
        {
            name = _name;
            supported = _supported;
        }
#endif
        const char_t* name;
        bool_t supported;
    };
    
    struct ExtensionRegistry
    {
        static void_t load(ExtensionRegistry& registry);
        
#if OMAF_GRAPHICS_API_OPENGL_ES
        OMAF_EXTENSION_DECL(GL_OES_vertex_array_object);
        OMAF_EXTENSION_DECL(GL_EXT_discard_framebuffer);
        OMAF_EXTENSION_DECL(GL_OES_mapbuffer);
        OMAF_EXTENSION_DECL(GL_EXT_map_buffer_range);
        OMAF_EXTENSION_DECL(GL_EXT_multisampled_render_to_texture);
        OMAF_EXTENSION_DECL(GL_IMG_multisampled_render_to_texture);
        OMAF_EXTENSION_DECL(GL_NV_draw_instanced);
        OMAF_EXTENSION_DECL(GL_NV_instanced_arrays);
        OMAF_EXTENSION_DECL(GL_EXT_instanced_arrays);
#endif
        
#if OMAF_GRAPHICS_API_OPENGL
        OMAF_EXTENSION_DECL(GL_EXT_framebuffer_multisample);
        OMAF_EXTENSION_DECL(GL_NVX_gpu_memory_info);
        OMAF_EXTENSION_DECL(WGL_AMD_gpu_association);
        OMAF_EXTENSION_DECL(GL_ARB_draw_instanced);
        OMAF_EXTENSION_DECL(GL_ARB_instanced_arrays);
        OMAF_EXTENSION_DECL(GL_ARB_compute_shader);
#endif

#if OMAF_GRAPHICS_API_OPENGL_ES || OMAF_GRAPHICS_API_OPENGL
        OMAF_EXTENSION_DECL(GL_EXT_debug_label);
        OMAF_EXTENSION_DECL(GL_EXT_debug_marker);
        OMAF_EXTENSION_DECL(GL_KHR_debug);
        OMAF_EXTENSION_DECL(GL_EXT_draw_instanced);
#endif
    };
    
    static ExtensionRegistry DefaultExtensionRegistry;
    
#if OMAF_OPENGL_API_IMPORT
    
    template <typename T>
    void_t loadExtension(T* pointer, const char_t* name, Extension& extension)
    {
        if((extension.supported) && (*pointer == OMAF_NULL))
        {
#if OMAF_PLATFORM_ANDROID
            *pointer = (T)eglGetProcAddress(name);
#elif OMAF_PLATFORM_WINDOWS
            *pointer = (T)wglGetProcAddress(name);
#endif

            if(*pointer == OMAF_NULL)
            {
                extension.supported = false;
            }
        }
    }
    
#endif

OMAF_NS_END
