
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
#include "Graphics/OpenGL/NVRGLError.h"

#include "Graphics/OpenGL/NVRGLUtilities.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"

OMAF_NS_BEGIN
    const char_t* errorStringGL(GLenum error)
    {
        switch(error)
        {
            case GL_NO_ERROR:               return "GL_NO_ERROR";
            case GL_INVALID_ENUM:           return "GL_INVALID_ENUM";
            case GL_INVALID_VALUE:          return "GL_INVALID_VALUE";
            case GL_INVALID_OPERATION:      return "GL_INVALID_OPERATION";
                
#if defined __gl_h_
                
            case GL_STACK_OVERFLOW:         return "GL_STACK_OVERFLOW";
            case GL_STACK_UNDERFLOW:        return "GL_STACK_UNDERFLOW";
                
            case GL_CONTEXT_LOST:           return "GL_CONTEXT_LOST";
            case GL_TABLE_TOO_LARGE:        return "GL_TABLE_TOO_LARGE";
            
#endif
                
#if defined __gl_h_ || defined __gl3_h_
                
            case GL_OUT_OF_MEMORY:                      return "GL_OUT_OF_MEMORY";
            case GL_INVALID_FRAMEBUFFER_OPERATION:      return "GL_INVALID_FRAMEBUFFER_OPERATION";
                
#endif
        }
        
        return "GL_UNKNOWN_ERROR";
    }
    
#if OMAF_PLATFORM_ANDROID
    
    const char_t* errorStringEGL(EGLint error)
    {
        switch (error)
        {
            case EGL_SUCCESS:               return "EGL_SUCCESS";
            case EGL_NOT_INITIALIZED:       return "EGL_NOT_INITIALIZED";
            case EGL_BAD_ACCESS:            return "EGL_BAD_ACCESS";
            case EGL_BAD_ALLOC:             return "EGL_BAD_ALLOC";
            case EGL_BAD_ATTRIBUTE:         return "EGL_BAD_ATTRIBUTE";
            case EGL_BAD_CONTEXT:           return "EGL_BAD_CONTEXT";
            case EGL_BAD_CONFIG:            return "EGL_BAD_CONFIG";
            case EGL_BAD_CURRENT_SURFACE:   return "EGL_BAD_CURRENT_SURFACE";
            case EGL_BAD_DISPLAY:           return "EGL_BAD_DISPLAY";
            case EGL_BAD_SURFACE:           return "EGL_BAD_SURFACE";
            case EGL_BAD_MATCH:             return "EGL_BAD_MATCH";
            case EGL_BAD_PARAMETER:         return "EGL_BAD_PARAMETER";
            case EGL_BAD_NATIVE_PIXMAP:     return "EGL_BAD_NATIVE_PIXMAP";
            case EGL_BAD_NATIVE_WINDOW:     return "EGL_BAD_NATIVE_WINDOW";
            case EGL_CONTEXT_LOST:          return "EGL_CONTEXT_LOST";
                
            default:                        return "EGL_UNKNOWN_ERROR";
        }
    }
    
#endif
OMAF_NS_END
