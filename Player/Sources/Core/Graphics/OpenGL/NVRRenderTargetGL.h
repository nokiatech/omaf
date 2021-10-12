
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

#include "Graphics/NVRConfig.h"
#include "Graphics/NVRDependencies.h"

#include "Graphics/OpenGL/NVRTextureGL.h"

OMAF_NS_BEGIN

namespace AttachmentType
{
    enum Enum
    {
        INVALID,

        COLOR_ATTACHMENT,
        DEPTH_ATTACHMENT,
        STENCIL_ATTACHMENT,
        DEPTH_STENCIL_ATTACHMENT,

        COUNT
    };
}

struct AttachmentGL
{
    AttachmentType::Enum type;
    GLuint handle;
};

class RenderTargetGL
{
public:
    RenderTargetGL();
    ~RenderTargetGL();

    GLuint getHandle() const;

    uint16_t getWidth() const;
    uint16_t getHeight() const;

    bool_t create(TextureFormat::Enum colorBufferFormat,
                  TextureFormat::Enum depthStencilBufferFormat,
                  uint16_t width,
                  uint16_t height,
                  uint8_t numMultisamples,
                  uint16_t discardMask);

    bool_t create(TextureGL** attachments, uint8_t numAttachments, uint16_t discardMask);

    void_t destroy();

    void_t bind();
    void_t unbind();

    void_t discard();

private:
    OMAF_NO_ASSIGN(RenderTargetGL);
    OMAF_NO_COPY(RenderTargetGL);

private:
    GLuint mHandle;

    uint16_t mWidth;
    uint16_t mHeight;

    uint32_t mNumAttachments;
    AttachmentGL mAttachments[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];

    bool_t mDestroyAttachments;

    uint16_t mDiscardMask;

    bool_t mNativeHandle;
};

OMAF_NS_END
