
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
#include "Graphics/NVRIRenderContext.h"

OMAF_NS_BEGIN

IRenderContext::IRenderContext(RendererType::Enum type, RenderBackend::Parameters& parameters)
    : mRendererType(type)
    , mParameters(parameters)
    , mViewport(0, 0, 0, 0)
    , mScissors(0, 0, 0, 0)
{
    OMAF_UNUSED_VARIABLE(parameters);

    resetDefaults();
}

IRenderContext::~IRenderContext()
{
}

RendererType::Enum IRenderContext::getRendererType() const
{
    return mRendererType;
}

void_t IRenderContext::resetDefaults()
{
    // Initialize blend state defaults
    BlendState blendState;
    MemoryCopy(&mActiveBlendState, &blendState, OMAF_SIZE_OF(blendState));

    // Initialize depth stencil state defaults
    DepthStencilState depthStencilState;
    MemoryCopy(&mActiveDepthStencilState, &depthStencilState, OMAF_SIZE_OF(depthStencilState));

    // Initialize rasterizer state defaults
    RasterizerState rasterizerState;
    MemoryCopy(&mActiveRasterizerState, &rasterizerState, OMAF_SIZE_OF(rasterizerState));

    // Common defaults
    mViewport = Viewport(0, 0, 0, 0);
    mScissors = Scissors(0, 0, 0, 0);

    // Active resource handles
    mActiveVertexBuffer = VertexBufferHandle::Invalid;
    mActiveIndexBuffer = IndexBufferHandle::Invalid;
    mActiveShader = ShaderHandle::Invalid;
    mActiveRenderTarget = RenderTargetHandle::Invalid;

    for (size_t i = 0; i < OMAF_MAX_TEXTURE_UNITS; ++i)
    {
        mActiveTexture[i] = TextureHandle::Invalid;
    }
}

OMAF_NS_END
