
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

#include "Graphics/NVRDependencies.h"

OMAF_NS_BEGIN

#define OMAF_ENABLE_GL_DEBUGGING 0
        
#if OMAF_ENABLE_GL_DEBUGGING && OMAF_ENABLE_LOG
        
    #define OMAF_GL_CHECK_ERRORS()                                                       \
    {                                                                                   \
        do                                                                              \
        {                                                                               \
            GLenum error = glGetError();                                                \
            const char_t* str = errorStringGL(error);                                   \
                                                                                        \
            OMAF_ASSERT(error == GL_NO_ERROR, "OpenGL error:%d %s", int(error), str);    \
        }                                                                               \
        while(false);                                                                   \
    }
            
    #define OMAF_GL_CHECK(call)                                                          \
    {                                                                                   \
        do                                                                              \
        {                                                                               \
            call;                                                                       \
                                                                                        \
            GLenum error = glGetError();                                                \
            const char_t* str = errorStringGL(error);                                   \
                                                                                        \
            OMAF_ASSERT(error == GL_NO_ERROR, "OpenGL error:%d %s", int(error), str);    \
        }                                                                               \
        while(false);                                                                   \
    }
        
#else
        
    #define OMAF_GL_CHECK_ERRORS()
    #define OMAF_GL_CHECK(call) call
        
#endif
    
const char_t* errorStringGL(GLenum error);
    
#if OMAF_PLATFORM_ANDROID
    
    const char_t* errorStringEGL(EGLint error);
    
#endif

OMAF_NS_END
