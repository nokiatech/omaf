
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
#include "Graphics/D3D11/NVRRenderTargetD3D11.h"

#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(RenderTargetD3D11)

RenderTargetD3D11::RenderTargetD3D11()
    : mDepthStencilView(NULL)
    , mWidth(0)
    , mHeight(0)
    , mNumAttachments(0)
    , mNativeHandle(false)
{
    MemoryZero(mRenderTargetViews, OMAF_SIZE_OF(mRenderTargetViews));
}

RenderTargetD3D11::~RenderTargetD3D11()
{
    for (uint8_t i = 0; i < OMAF_MAX_RENDER_TARGET_ATTACHMENTS; ++i)
    {
        OMAF_ASSERT(mRenderTargetViews[i] == NULL, "Render target is not destroyed");
    }

    OMAF_ASSERT(mDepthStencilView == NULL, "Render target is not destroyed");
}

uint16_t RenderTargetD3D11::getWidth() const
{
    return mWidth;
}

uint16_t RenderTargetD3D11::getHeight() const
{
    return mHeight;
}

ID3D11RenderTargetView** RenderTargetD3D11::getRenderTargetViews()
{
    return mRenderTargetViews;
}

ID3D11DepthStencilView* RenderTargetD3D11::getDepthStencilView()
{
    return mDepthStencilView;
}

bool_t RenderTargetD3D11::create(ID3D11Device* device,
                                 ID3D11DeviceContext* deviceContext,
                                 TextureD3D11** attachments,
                                 uint8_t numAttachments,
                                 uint16_t discardMask)
{
    OMAF_ASSERT_NOT_NULL(attachments);
    OMAF_ASSERT(numAttachments > 0, "");

    uint16_t width = 0;
    uint16_t height = 0;

    for (uint8_t i = 0; i < numAttachments; ++i)
    {
        D3D11_TEXTURE2D_DESC textureDesc;
        TextureD3D11* texture = attachments[i];

        ID3D11Texture2D* dxtexture = (ID3D11Texture2D*) texture->getHandle();
        dxtexture->GetDesc(&textureDesc);

        TextureFormat::Enum format = texture->getFormat();

        // Check that all attachments are created with same dimensions
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

        Attachment& attachment = mAttachments[i];
        attachment.texture = texture;

        TextureFormatInfo textureFormatInfo = getTextureFormatInfoD3D(format);

        bool_t hasDepthComponent = TextureFormat::hasDepthComponent(format);
        bool_t hasStencilComponent = TextureFormat::hasStencilComponent(format);

        if (hasDepthComponent || hasStencilComponent)
        {
            if (hasDepthComponent && hasStencilComponent)
            {
                attachment.type = AttachmentType::DEPTH_STENCIL_ATTACHMENT;
            }
            else if (hasDepthComponent)
            {
                attachment.type = AttachmentType::DEPTH_ATTACHMENT;
            }
            else if (hasStencilComponent)
            {
                attachment.type = AttachmentType::STENCIL_ATTACHMENT;
            }
            else
            {
                OMAF_ASSERT_UNREACHABLE();
            }

            D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
            depthStencilViewDesc.Format = textureFormatInfo.dsvFormat;
            switch (texture->getNativeDesc().type)
            {
            case TextureType::TEXTURE_CUBE:
            case TextureType::TEXTURE_ARRAY:
            {
                depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
                depthStencilViewDesc.Flags = 0;
                depthStencilViewDesc.Texture2DArray.ArraySize = 1;
                depthStencilViewDesc.Texture2DArray.FirstArraySlice = texture->getNativeDesc().arrayIndex;
                depthStencilViewDesc.Texture2DArray.MipSlice = 0;
                break;
            }
            case TextureType::TEXTURE_2D:  // normal native texture.
            case TextureType::INVALID:     // not a "native" texture
            default:
            {
                depthStencilViewDesc.Flags = 0;
                if (textureDesc.SampleDesc.Count > 1)
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    depthStencilViewDesc.Texture2D.MipSlice = 0;
                }
            }
            }
            OMAF_DX_CHECK(
                device->CreateDepthStencilView(texture->getHandle(), &depthStencilViewDesc, &mDepthStencilView));
        }
        else
        {
            attachment.type = AttachmentType::COLOR_ATTACHMENT;

            D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
            renderTargetViewDesc.Format = textureFormatInfo.textureFormat;

            switch (texture->getNativeDesc().type)
            {
            case TextureType::TEXTURE_CUBE:
            case TextureType::TEXTURE_ARRAY:
            {
                renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                renderTargetViewDesc.Texture2DArray.ArraySize = 1;
                renderTargetViewDesc.Texture2DArray.FirstArraySlice = texture->getNativeDesc().arrayIndex;
                renderTargetViewDesc.Texture2DArray.MipSlice = 0;
                break;
            }
            case TextureType::TEXTURE_2D:  // normal native texture.
            case TextureType::INVALID:     // not a "native" texture
            default:
            {
                if (textureDesc.SampleDesc.Count > 1)
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
                }
                else
                {
                    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
                    renderTargetViewDesc.Texture2D.MipSlice = 0;
                }
                break;
            }
            }


            OMAF_DX_CHECK(
                device->CreateRenderTargetView(texture->getHandle(), &renderTargetViewDesc, &mRenderTargetViews[i]));
        }
    }

    mWidth = width;
    mHeight = height;
    mNumAttachments = numAttachments;
    mNativeHandle = false;

    return true;
}

void_t RenderTargetD3D11::bind(ID3D11DeviceContext* deviceContext)
{
    // Collect all render target views
    if (mDepthStencilView != NULL)
    {
        deviceContext->OMSetRenderTargets(mNumAttachments - 1, mRenderTargetViews, mDepthStencilView);
    }
    else
    {
        deviceContext->OMSetRenderTargets(mNumAttachments, mRenderTargetViews, NULL);
    }
}

void_t RenderTargetD3D11::unbind(ID3D11DeviceContext* deviceContext)
{
    deviceContext->OMSetRenderTargets(0, NULL, NULL);
}

void_t RenderTargetD3D11::destroy()
{
    if (!mNativeHandle)
    {
        for (uint8_t i = 0; i < OMAF_MAX_RENDER_TARGET_ATTACHMENTS; ++i)
        {
            OMAF_DX_RELEASE(mRenderTargetViews[i], 0);
        }

        OMAF_DX_RELEASE(mDepthStencilView, 0);
    }

    mWidth = 0;
    mHeight = 0;
    mNumAttachments = 0;
    mNativeHandle = false;
}
OMAF_NS_END
