
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
#include "Graphics/NVRDiscardMask.h"

#include "Graphics/D3D11/NVRTextureD3D11.h"

OMAF_NS_BEGIN
class RenderTargetD3D11
{
public:
    RenderTargetD3D11();
    ~RenderTargetD3D11();

    uint16_t getWidth() const;
    uint16_t getHeight() const;

    ID3D11RenderTargetView** getRenderTargetViews();
    ID3D11DepthStencilView* getDepthStencilView();

    bool_t create(ID3D11Device* device,
                  ID3D11DeviceContext* deviceContext,
                  TextureD3D11** attachments,
                  uint8_t numAttachments,
                  uint16_t discardMask);

    void_t destroy();

    void_t bind(ID3D11DeviceContext* deviceContext);
    void_t unbind(ID3D11DeviceContext* deviceContext);

private:
    OMAF_NO_ASSIGN(RenderTargetD3D11);
    OMAF_NO_COPY(RenderTargetD3D11);

private:
    struct AttachmentType
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
    };

    struct Attachment
    {
        AttachmentType::Enum type;
        TextureD3D11* texture;
    };

private:
    ID3D11RenderTargetView* mRenderTargetViews[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];
    ID3D11DepthStencilView* mDepthStencilView;

    uint16_t mWidth;
    uint16_t mHeight;

    uint32_t mNumAttachments;
    Attachment mAttachments[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];

    bool_t mNativeHandle;
};
OMAF_NS_END
