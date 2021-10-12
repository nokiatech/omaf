
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
#include "Graphics/OpenGL/NVRRenderTargetGL.h"

#include "Foundation/NVRLogger.h"

#include "Graphics/NVRDiscardMask.h"
#include "Graphics/OpenGL/NVRGLContext.h"

#include "Graphics/OpenGL/NVRGLCompatibility.h"
#include "Graphics/OpenGL/NVRGLError.h"
#include "Graphics/OpenGL/NVRGLExtensions.h"
#include "Graphics/OpenGL/NVRGLUtilities.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(RenderTargetGL);

RenderTargetGL::RenderTargetGL()
    : mHandle(0)
    , mWidth(0)
    , mHeight(0)
    , mNumAttachments(0)
    , mDestroyAttachments(false)
    , mDiscardMask((uint16_t) DiscardMask::NONE)
    , mNativeHandle(false)
{
    MemoryZero(mAttachments, OMAF_SIZE_OF(mAttachments));
}

RenderTargetGL::~RenderTargetGL()
{
    OMAF_ASSERT(mHandle == 0, "Render target is not destroyed");
}

GLuint RenderTargetGL::getHandle() const
{
    return mHandle;
}

uint16_t RenderTargetGL::getWidth() const
{
    return mWidth;
}

uint16_t RenderTargetGL::getHeight() const
{
    return mHeight;
}

bool_t RenderTargetGL::create(TextureFormat::Enum colorBufferFormat,
                              TextureFormat::Enum depthStencilBufferFormat,
                              uint16_t width,
                              uint16_t height,
                              uint8_t numMultisamples,
                              uint16_t discardMask)
{
    OMAF_UNUSED_VARIABLE(numMultisamples);

    mWidth = width;
    mHeight = height;

    mNumAttachments = 0;

    // Backup previous states
    GLint currentTexture;
    GLint currentRenderbuffer;
    GLint currentFramebuffer;

    OMAF_GL_CHECK(glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture));
    OMAF_GL_CHECK(glGetIntegerv(GL_RENDERBUFFER_BINDING, &currentRenderbuffer));
    OMAF_GL_CHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer));

    // Create frame buffer
    OMAF_GL_CHECK(glGenFramebuffers(1, &mHandle));
    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mHandle));

    // Create texture
    if (colorBufferFormat != TextureFormat::INVALID)
    {
        const TextureFormatGL& formatGL = getTextureFormatGL(colorBufferFormat);

        AttachmentGL& attachment = mAttachments[mNumAttachments];
        attachment.type = AttachmentType::COLOR_ATTACHMENT;

        OMAF_GL_CHECK(glGenTextures(1, &attachment.handle));
        OMAF_GL_CHECK(glBindTexture(GL_TEXTURE_2D, attachment.handle));

        OMAF_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        OMAF_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        OMAF_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        OMAF_GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));

        OMAF_GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, formatGL.internalFormat, width, height, 0, formatGL.format,
                                   formatGL.type, NULL));

        OMAF_GL_CHECK(
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, attachment.handle, 0));

        ++mNumAttachments;
    }

    // Create render buffer
    if (depthStencilBufferFormat != TextureFormat::INVALID)
    {
        const TextureFormatGL& formatGL = getTextureFormatGL(depthStencilBufferFormat);

        bool_t hasDepthAttachment = TextureFormat::hasStencilComponent(depthStencilBufferFormat);
        bool_t hasStencilAttachment = TextureFormat::hasDepthComponent(depthStencilBufferFormat);

        AttachmentGL& attachment = mAttachments[mNumAttachments];

        if (hasDepthAttachment && hasStencilAttachment)
        {
            attachment.type = AttachmentType::DEPTH_STENCIL_ATTACHMENT;
        }
        else if (hasDepthAttachment)
        {
            attachment.type = AttachmentType::DEPTH_ATTACHMENT;
        }
        else if (hasStencilAttachment)
        {
            attachment.type = AttachmentType::STENCIL_ATTACHMENT;
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }

        OMAF_GL_CHECK(glGenRenderbuffers(1, &attachment.handle));
        OMAF_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, attachment.handle));
        OMAF_GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, formatGL.internalFormat, width, height));

        if (hasDepthAttachment)
        {
            OMAF_GL_CHECK(
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, attachment.handle));
        }

        if (hasStencilAttachment)
        {
            OMAF_GL_CHECK(
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, attachment.handle));
        }

        ++mNumAttachments;
    }

    GLuint status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    OMAF_GL_CHECK_ERRORS();

    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        destroy();

        return false;
    }

    mDestroyAttachments = true;
    mDiscardMask = discardMask;

    // Restore previous states
    OMAF_GL_CHECK(glBindTexture(GL_TEXTURE_2D, currentTexture));
    OMAF_GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, currentRenderbuffer));
    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer));

    return true;
}

bool_t RenderTargetGL::create(TextureGL** attachments, uint8_t numAttachments, uint16_t discardMask)
{
    OMAF_ASSERT_NOT_NULL(attachments);
    OMAF_ASSERT(numAttachments > 0, "");

    // Backup previous states
    GLint currentFramebuffer;

    OMAF_GL_CHECK(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFramebuffer));

    // Create frame buffer
    OMAF_GL_CHECK(glGenFramebuffers(1, &mHandle));
    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mHandle));

    GLenum drawBuffers[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];
    uint8_t drawBufferIndex = 0;

    uint16_t width = 0;
    uint16_t height = 0;

    for (uint8_t i = 0; i < numAttachments; ++i)
    {
        TextureGL* texture = attachments[i];
        TextureFormat::Enum format = texture->getFormat();

        if (i == 0)
        {
            width = texture->getWidth();
            height = texture->getHeight();
        }
        else
        {
            OMAF_ASSERT(width == texture->getWidth(), "");
            OMAF_ASSERT(height == texture->getHeight(), "");
        }

        AttachmentGL& attachment = mAttachments[i];
        attachment.handle = texture->getHandle();

        bool_t hasDepthComponent = TextureFormat::hasDepthComponent(format);
        bool_t hasStencilComponent = TextureFormat::hasStencilComponent(format);

        if (hasDepthComponent && hasStencilComponent)
        {
            attachment.type = AttachmentType::DEPTH_STENCIL_ATTACHMENT;

            OMAF_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
                                                 attachment.handle, 0));
        }
        else if (hasDepthComponent)
        {
            attachment.type = AttachmentType::DEPTH_ATTACHMENT;

            OMAF_GL_CHECK(
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachment.handle, 0));
        }
        else if (hasStencilComponent)
        {
            attachment.type = AttachmentType::STENCIL_ATTACHMENT;

            OMAF_GL_CHECK(
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, attachment.handle, 0));
        }
        else if (!hasDepthComponent && !hasStencilComponent)
        {
            attachment.type = AttachmentType::COLOR_ATTACHMENT;

            OMAF_GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + drawBufferIndex, GL_TEXTURE_2D,
                                                 attachment.handle, 0));

            drawBuffers[drawBufferIndex] = GL_COLOR_ATTACHMENT0 + drawBufferIndex;
            ++drawBufferIndex;
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }
    }

    mWidth = width;
    mHeight = height;

    mNumAttachments = numAttachments;
    mDestroyAttachments = false;
    mDiscardMask = discardMask;

    // Restore previous states
    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer));

    return true;
}

void_t RenderTargetGL::bind()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, mHandle));

    if (mNativeHandle)
    {
        return;
    }

    GLenum drawBuffers[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];
    uint8_t drawBufferIndex = 0;

    for (uint8_t i = 0; i < mNumAttachments; ++i)
    {
        AttachmentGL& attachment = mAttachments[i];

        if (attachment.type == AttachmentType::COLOR_ATTACHMENT)
        {
            drawBuffers[drawBufferIndex] = GL_COLOR_ATTACHMENT0 + drawBufferIndex;
            ++drawBufferIndex;
        }
    }

    if (drawBufferIndex == 0)
    {
        GLenum none = GL_NONE;
        OMAF_GL_CHECK(glDrawBuffers(1, &none));
    }
    else
    {
        OMAF_GL_CHECK(glDrawBuffers(drawBufferIndex, drawBuffers));
    }

    if (drawBufferIndex == 0)
    {
        OMAF_GL_CHECK(glReadBuffer(GL_NONE));
    }
}

void_t RenderTargetGL::unbind()
{
    OMAF_ASSERT(mHandle != 0, "");

    OMAF_GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
}

void_t RenderTargetGL::destroy()
{
    OMAF_ASSERT(mHandle != 0, "");

    if (!mNativeHandle)
    {
        OMAF_GL_CHECK(glDeleteFramebuffers(1, &mHandle));
        mHandle = 0;

        for (size_t i = 0; i < mNumAttachments; ++i)
        {
            AttachmentGL& attachment = mAttachments[i];

            if (mDestroyAttachments)
            {
                if (attachment.type == AttachmentType::DEPTH_ATTACHMENT ||
                    attachment.type == AttachmentType::STENCIL_ATTACHMENT ||
                    attachment.type == AttachmentType::DEPTH_STENCIL_ATTACHMENT)
                {
                    OMAF_GL_CHECK(glDeleteRenderbuffers(1, &attachment.handle));
                }
                else if (attachment.type == AttachmentType::COLOR_ATTACHMENT)
                {
                    OMAF_GL_CHECK(glDeleteTextures(1, &attachment.handle));
                }
                else
                {
                    OMAF_ASSERT_UNREACHABLE();
                }
            }

            attachment.handle = 0;
        }
    }

    mHandle = 0;
    mWidth = 0;
    mHeight = 0;

    mNumAttachments = 0;

    mDestroyAttachments = false;
    mDiscardMask = (uint16_t) DiscardMask::NONE;

    mNativeHandle = false;
}

void_t RenderTargetGL::discard()
{
    OMAF_ASSERT(mDiscardMask != (uint16_t) DiscardMask::INVALID, "");

    GLenum attachments[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];
    uint8_t index = 0;

    uint16_t colorAttachmentFlags = (mDiscardMask & DiscardMask::DISCARD_COLOR_ALL);

    if (DiscardMask::NONE != colorAttachmentFlags)
    {
        for (uint8_t i = 0; i < mNumAttachments; ++i)
        {
            if (DiscardMask::NONE != (colorAttachmentFlags & (DiscardMask::DISCARD_COLOR_0 << i)))
            {
                attachments[index++] = GL_COLOR_ATTACHMENT0 + i;
            }
        }
    }

    uint16_t depthStencilAttachmentFlags = mDiscardMask & (DiscardMask::DISCARD_DEPTH | DiscardMask::DISCARD_STENCIL);

    if (DiscardMask::NONE != depthStencilAttachmentFlags)
    {
        if (depthStencilAttachmentFlags == (DiscardMask::DISCARD_DEPTH | DiscardMask::DISCARD_STENCIL))
        {
            attachments[index++] = GL_DEPTH_STENCIL_ATTACHMENT;
        }
        else if (depthStencilAttachmentFlags == DiscardMask::DISCARD_DEPTH)
        {
            attachments[index++] = GL_DEPTH_ATTACHMENT;
        }
        else if (depthStencilAttachmentFlags == DiscardMask::DISCARD_STENCIL)
        {
            attachments[index++] = GL_STENCIL_ATTACHMENT;
        }
    }

    if (index > 0)
    {
#if OMAF_GRAPHICS_API_OPENGL_ES >= 30 || OMAF_GRAPHICS_API_OPENGL >= 43

        OMAF_GL_CHECK(glInvalidateFramebuffer(GL_FRAMEBUFFER, index, attachments));

#elif defined(GL_EXT_discard_framebuffer)

        if (DefaultExtensionRegistry._GL_EXT_discard_framebuffer.supported)
        {
            OMAF_GL_CHECK(glDiscardFramebufferEXT(GL_FRAMEBUFFER, index, attachments));
        }

#endif
    }
}

OMAF_NS_END
