
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
#include "Graphics/OpenGL/NVRGLContext.h"

#include "Graphics/NVRDependencies.h"

#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLCompatibility.h"

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(GLContext);
    
    namespace GLContext
    {
        static Framebuffer mFramebuffer;
        static bool_t mInitialized = false;
        
        void_t create(uint32_t width, uint32_t height)
        {
            OMAF_UNUSED_VARIABLE(width);
            OMAF_UNUSED_VARIABLE(height);

            mInitialized = true;
            
            OMAF_ASSERT_NOT_IMPLEMENTED();
        }
        
        void_t createInternal()
        {
            MemoryZero(&mFramebuffer, OMAF_SIZE_OF(mFramebuffer));
            
            OMAF_GL_CHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&mFramebuffer.fbo));
            OMAF_GL_CHECK(glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&mFramebuffer.colorRbo));
            OMAF_GL_CHECK(glGetIntegerv(GL_RENDERBUFFER_BINDING, (GLint*)&mFramebuffer.depthStencilRbo));
            
            // Hackish way to get default FBO dimensions
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            
            mFramebuffer.width = viewport[2];
            mFramebuffer.height = viewport[3];
            
            mFramebuffer.isInternal = true;
            
            mInitialized = true;
        }
        
        void_t destroy()
        {
            mInitialized = false;
            
            if (!mFramebuffer.isInternal)
            {
                OMAF_ASSERT_NOT_IMPLEMENTED();
            }
            
            MemoryZero(&mFramebuffer, OMAF_SIZE_OF(mFramebuffer));
        }
        
        bool_t isValid()
        {
            return mInitialized;
        }
        
        const Framebuffer* getDefaultFramebuffer()
        {
            if (isValid())
            {
                return &mFramebuffer;
            }
            
            return OMAF_NULL;
        }
    }
OMAF_NS_END
