
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
#include "Graphics/OpenGL/NVRGLExtensions.h"

#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"

#include "Foundation/NVRLogger.h"

#if OMAF_OPENGL_API_IMPORT

OMAF_NS_BEGIN

#define OMAF_GL_IMPORT OMAF_GL_EXTENSION_FUNC_IMPORT
#include "Graphics/OpenGL/NVRGLExtensionList.h"
#undef OMAF_GL_IMPORT

OMAF_NS_END

#if OMAF_PLATFORM_WINDOWS

extern "C"
{
    WINGDIAPI PROC WINAPI wglGetProcAddress(LPCSTR);
}

#endif

#endif

OMAF_NS_BEGIN

#ifndef OMAF_ENABLE_GL_EXTENSION_LOGGING
#define OMAF_ENABLE_GL_EXTENSION_LOGGING 0
#endif

OMAF_LOG_ZONE(GLExtensions);

void_t ExtensionRegistry::load(ExtensionRegistry& registry)
{
// Query available extensions
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 30

    GLint numExtensions = 0;
    OMAF_GL_CHECK(glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions));

#if OMAF_ENABLE_GL_EXTENSION_LOGGING

    OMAF_LOG_D("-------------------- OpenGL / OpenGL ES - Extensions --------------------");

#endif

    for (GLint i = 0; i < numExtensions; i++)
    {
        const char_t* extensionStr = getStringGL(GL_EXTENSIONS, i);

#if OMAF_ENABLE_GL_EXTENSION_LOGGING

        OMAF_LOG_D("\t\t%s", extensionStr);

#endif

        Extension* ptr = (Extension*) &registry;
        size_t numEntries = OMAF_SIZE_OF(registry) / OMAF_SIZE_OF(Extension);

        for (size_t e = 0; e < numEntries; ++e)
        {
            if (StringFindFirst(extensionStr, ptr->name) != Npos)
            {
                ptr->supported = true;

                break;
            }

            ptr++;
        }
    }

#if OMAF_ENABLE_GL_EXTENSION_LOGGING

    OMAF_LOG_D("------------------------------------------------------------------------------");

#endif

#else

    const char_t* extensionsStr = getStringGL(GL_EXTENSIONS);

#if OMAF_ENABLE_GL_EXTENSION_LOGGING

    OMAF_LOG_D("-------------------- OpenGL / OpenGL ES - Extensions --------------------");
    OMAF_LOG_D("%s", extensionsStr);
    OMAF_LOG_D("------------------------------------------------------------------------------");

#endif

    Extension* ptr = (Extension*) &registry;
    size_t numExtensions = OMAF_SIZE_OF(registry) / OMAF_SIZE_OF(Extension);

    for (size_t e = 0; e < numExtensions; ++e)
    {
        if (StringFindFirst(extensionsStr, ptr->name) != Npos)
        {
            ptr->supported = true;
        }
        else
        {
            ptr->supported = false;
        }

        ptr++;
    }

#endif

// Load extensions function ptrs
#if OMAF_OPENGL_API_IMPORT

#if OMAF_GRAPHICS_API_OPENGL_ES

    if (registry._GL_OES_vertex_array_object.supported)
    {
        loadExtension<PFNGLGENVERTEXARRAYSOESPROC>(&glGenVertexArraysOES, "glGenVertexArraysOES",
                                                   registry._GL_OES_vertex_array_object);
        loadExtension<PFNGLBINDVERTEXARRAYOESPROC>(&glBindVertexArrayOES, "glBindVertexArrayOES",
                                                   registry._GL_OES_vertex_array_object);
        loadExtension<PFNGLDELETEVERTEXARRAYSOESPROC>(&glDeleteVertexArraysOES, "glDeleteVertexArraysOES",
                                                      registry._GL_OES_vertex_array_object);
        loadExtension<PFNGLISVERTEXARRAYOESPROC>(&glIsVertexArrayOES, "glIsVertexArrayOES",
                                                 registry._GL_OES_vertex_array_object);
    }

    if (registry._GL_EXT_discard_framebuffer.supported)
    {
        loadExtension<PFNGLDISCARDFRAMEBUFFEREXTPROC>(&glDiscardFramebufferEXT, "glDiscardFramebufferEXT",
                                                      registry._GL_EXT_discard_framebuffer);
    }

    if (registry._GL_OES_mapbuffer.supported)
    {
        loadExtension<PFNGLGETBUFFERPOINTERVOESPROC>(&glGetBufferPointervOES, "glGetBufferPointervOES",
                                                     registry._GL_OES_mapbuffer);
        loadExtension<PFNGLMAPBUFFEROESPROC>(&glMapBufferOES, "glMapBufferOES", registry._GL_OES_mapbuffer);
        loadExtension<PFNGLUNMAPBUFFEROESPROC>(&glUnmapBufferOES, "glUnmapBufferOES", registry._GL_OES_mapbuffer);
    }

    if (registry._GL_EXT_map_buffer_range.supported)
    {
        loadExtension<PFNGLMAPBUFFERRANGEEXTPROC>(&glMapBufferRangeEXT, "glMapBufferRangeEXT",
                                                  registry._GL_EXT_map_buffer_range);
        loadExtension<PFNGLFLUSHMAPPEDBUFFERRANGEEXTPROC>(&glFlushMappedBufferRangeEXT, "glFlushMappedBufferRangeEXT",
                                                          registry._GL_EXT_map_buffer_range);
    }

    if (registry._GL_EXT_multisampled_render_to_texture.supported)
    {
        loadExtension<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>(&glRenderbufferStorageMultisampleEXT,
                                                                  "glRenderbufferStorageMultisampleEXT",
                                                                  registry._GL_EXT_multisampled_render_to_texture);
        loadExtension<PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC>(&glFramebufferTexture2DMultisampleEXT,
                                                                   "glFramebufferTexture2DMultisampleEXT",
                                                                   registry._GL_EXT_multisampled_render_to_texture);
    }

    if (registry._GL_IMG_multisampled_render_to_texture.supported)
    {
        loadExtension<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>(&glRenderbufferStorageMultisampleEXT,
                                                                  "glRenderbufferStorageMultisampleEXT",
                                                                  registry._GL_IMG_multisampled_render_to_texture);
        loadExtension<PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC>(&glFramebufferTexture2DMultisampleEXT,
                                                                   "glFramebufferTexture2DMultisampleEXT",
                                                                   registry._GL_IMG_multisampled_render_to_texture);
    }

    if (registry._GL_NV_draw_instanced.supported)
    {
        loadExtension<PFNGLDRAWARRAYSINSTANCEDNVPROC>(&glDrawArraysInstancedNV, "glDrawArraysInstancedNV",
                                                      registry._GL_NV_draw_instanced);
        loadExtension<PFNGLDRAWELEMENTSINSTANCEDNVPROC>(&glDrawElementsInstancedNV, "glDrawElementsInstancedNV",
                                                        registry._GL_NV_draw_instanced);
    }

    if (registry._GL_NV_instanced_arrays.supported)
    {
        loadExtension<PFNGLVERTEXATTRIBDIVISORNVPROC>(&glVertexAttribDivisorNV, "glVertexAttribDivisorNV",
                                                      registry._GL_NV_instanced_arrays);
    }

    if (registry._GL_EXT_instanced_arrays.supported)
    {
        loadExtension<PFNGLVERTEXATTRIBDIVISOREXTPROC>(&glVertexAttribDivisorEXT, "glVertexAttribDivisorEXT",
                                                       registry._GL_EXT_instanced_arrays);
        loadExtension<PFNGLDRAWARRAYSINSTANCEDEXTPROC>(&glDrawArraysInstancedEXT, "glDrawArraysInstancedEXT",
                                                       registry._GL_EXT_instanced_arrays);
        loadExtension<PFNGLDRAWELEMENTSINSTANCEDEXTPROC>(&glDrawElementsInstancedEXT, "glDrawElementsInstancedEXT",
                                                         registry._GL_EXT_instanced_arrays);
    }

#endif

#if OMAF_GRAPHICS_API_OPENGL

    if (registry._GL_EXT_framebuffer_multisample.supported)
    {
        loadExtension<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>(&glRenderbufferStorageMultisampleEXT,
                                                                  "glRenderbufferStorageMultisampleEXT",
                                                                  registry._GL_EXT_framebuffer_multisample);
    }

    if (registry._GL_NVX_gpu_memory_info.supported)
    {
        // No functions to load
    }

    if (registry._WGL_AMD_gpu_association.supported)
    {
        loadExtension<PFNWGLGETGPUIDSAMDPROC>(&wglGetGPUIDsAMD, "wglGetGPUIDsAMD", registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLGETGPUINFOAMDPROC>(&wglGetGPUInfoAMD, "wglGetGPUInfoAMD",
                                               registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLGETCONTEXTGPUIDAMDPROC>(&wglGetContextGPUIDAMD, "wglGetContextGPUIDAMD",
                                                    registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLCREATEASSOCIATEDCONTEXTAMDPROC>(
            &wglCreateAssociatedContextAMD, "wglCreateAssociatedContextAMD", registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLCREATEASSOCIATEDCONTEXTATTRIBSAMDPROC>(&wglCreateAssociatedContextAttribsAMD,
                                                                   "wglCreateAssociatedContextAttribsAMD",
                                                                   registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLDELETEASSOCIATEDCONTEXTAMDPROC>(
            &wglDeleteAssociatedContextAMD, "wglDeleteAssociatedContextAMD", registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLMAKEASSOCIATEDCONTEXTCURRENTAMDPROC>(&wglMakeAssociatedContextCurrentAMD,
                                                                 "wglMakeAssociatedContextCurrentAMD",
                                                                 registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLGETCURRENTASSOCIATEDCONTEXTAMDPROC>(
            &wglGetCurrentAssociatedContextAMD, "wglGetCurrentAssociatedContextAMD", registry._WGL_AMD_gpu_association);
        loadExtension<PFNWGLBLITCONTEXTFRAMEBUFFERAMDPROC>(
            &wglBlitContextFramebufferAMD, "wglBlitContextFramebufferAMD", registry._WGL_AMD_gpu_association);
    }

    if (registry._GL_ARB_draw_instanced.supported)
    {
        loadExtension<PFNGLDRAWARRAYSINSTANCEDARBPROC>(&glDrawArraysInstancedARB, "glDrawArraysInstancedARB",
                                                       registry._GL_ARB_draw_instanced);
        loadExtension<PFNGLDRAWELEMENTSINSTANCEDARBPROC>(&glDrawElementsInstancedARB, "glDrawElementsInstancedARB",
                                                         registry._GL_ARB_draw_instanced);
    }

    if (registry._GL_ARB_instanced_arrays.supported)
    {
        loadExtension<PFNGLVERTEXATTRIBDIVISORARBPROC>(&glVertexAttribDivisorARB, "glVertexAttribDivisorARB",
                                                       registry._GL_ARB_instanced_arrays);
    }

    if (registry._GL_ARB_compute_shader.supported)
    {
        loadExtension<PFNGLDISPATCHCOMPUTEARBPROC>(&glDispatchComputeARB, "glDispatchComputeARB",
                                                   registry._GL_ARB_compute_shader);
        loadExtension<PFNGLDISPATCHCOMPUTEINDIRECTARBPROC>(
            &glDispatchComputeIndirectARB, "glDispatchComputeIndirectARB", registry._GL_ARB_compute_shader);
    }

#endif

#if OMAF_GRAPHICS_API_OPENGL_ES || OMAF_GRAPHICS_API_OPENGL

    if (registry._GL_EXT_debug_label.supported)
    {
        loadExtension<PFNGLLABELOBJECTEXTPROC>(&glLabelObjectEXT, "glLabelObjectEXT", registry._GL_EXT_debug_label);
        loadExtension<PFNGLGETOBJECTLABELEXTPROC>(&glGetObjectLabelEXT, "glGetObjectLabelEXT",
                                                  registry._GL_EXT_debug_label);
    }

    if (registry._GL_EXT_debug_marker.supported)
    {
        loadExtension<PFNGLINSERTEVENTMARKEREXTPROC>(&glInsertEventMarkerEXT, "glInsertEventMarkerEXT",
                                                     registry._GL_EXT_debug_marker);
        loadExtension<PFNGLPUSHGROUPMARKEREXTPROC>(&glPushGroupMarkerEXT, "glPushGroupMarkerEXT",
                                                   registry._GL_EXT_debug_marker);
        loadExtension<PFNGLPOPGROUPMARKEREXTPROC>(&glPopGroupMarkerEXT, "glPopGroupMarkerEXT",
                                                  registry._GL_EXT_debug_marker);
    }

    if (registry._GL_KHR_debug.supported)
    {
        loadExtension<PFNGLDEBUGMESSAGECONTROLKHRPROC>(&glDebugMessageControlKHR, "glDebugMessageControlKHR",
                                                       registry._GL_KHR_debug);
        loadExtension<PFNGLDEBUGMESSAGEINSERTKHRPROC>(&glDebugMessageInsertKHR, "glDebugMessageInsertKHR",
                                                      registry._GL_KHR_debug);
        loadExtension<PFNGLDEBUGMESSAGECALLBACKKHRPROC>(&glDebugMessageCallbackKHR, "glDebugMessageCallbackKHR",
                                                        registry._GL_KHR_debug);
        loadExtension<PFNGLGETDEBUGMESSAGELOGKHRPROC>(&glGetDebugMessageLogKHR, "glGetDebugMessageLogKHR",
                                                      registry._GL_KHR_debug);
        loadExtension<PFNGLPUSHDEBUGGROUPKHRPROC>(&glPushDebugGroupKHR, "glPushDebugGroupKHR", registry._GL_KHR_debug);
        loadExtension<PFNGLPOPDEBUGGROUPKHRPROC>(&glPopDebugGroupKHR, "glPopDebugGroupKHR", registry._GL_KHR_debug);
        loadExtension<PFNGLOBJECTLABELKHRPROC>(&glObjectLabelKHR, "glObjectLabelKHR", registry._GL_KHR_debug);
        loadExtension<PFNGLGETOBJECTLABELKHRPROC>(&glGetObjectLabelKHR, "glGetObjectLabelKHR", registry._GL_KHR_debug);
        loadExtension<PFNGLOBJECTPTRLABELKHRPROC>(&glObjectPtrLabelKHR, "glObjectPtrLabelKHR", registry._GL_KHR_debug);
        loadExtension<PFNGLGETOBJECTPTRLABELKHRPROC>(&glGetObjectPtrLabelKHR, "glGetObjectPtrLabelKHR",
                                                     registry._GL_KHR_debug);
        loadExtension<PFNGLGETPOINTERVKHRPROC>(&glGetPointervKHR, "glGetPointervKHR", registry._GL_KHR_debug);
    }

    if (registry._GL_EXT_draw_instanced.supported)
    {
        loadExtension<PFNGLDRAWARRAYSINSTANCEDEXTPROC>(&glDrawArraysInstancedEXT, "glDrawArraysInstancedEXT",
                                                       registry._GL_EXT_draw_instanced);
        loadExtension<PFNGLDRAWELEMENTSINSTANCEDEXTPROC>(&glDrawElementsInstancedEXT, "glDrawElementsInstancedEXT",
                                                         registry._GL_EXT_draw_instanced);
    }

#endif

#endif
}

OMAF_NS_END
