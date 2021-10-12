
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

#include "NVREssentials.h"

#if OMAF_PLATFORM_WINDOWS

#define OMAF_GL_CONTEXT_WGL 1

#elif OMAF_PLATFORM_ANDROID

#define OMAF_GL_CONTEXT_EGL 1

#endif

OMAF_NS_BEGIN

namespace GLContext
{
    struct Framebuffer
    {
        uint32_t fbo;
        uint32_t colorRbo;
        uint32_t depthStencilRbo;

        uint32_t width;
        uint32_t height;

        // FBO can be created internally by window / application frameworks.
        // Eg. GLKit for iOS / Mac OS creates FBO for you, so you have query it manually and pass forward.
        bool_t isInternal;
    };

    void_t create(uint32_t width, uint32_t height);
    void_t createInternal();

    void_t destroy();

    bool_t isValid();

    const Framebuffer* getDefaultFramebuffer();
}  // namespace GLContext

OMAF_NS_END
