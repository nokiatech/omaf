
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
#include "Renderer/NVREquirectangularMask.h"
#include "Graphics/NVRRenderBackend.h"
#include "Math/OMAFMathConstants.h"
#include "Math/OMAFVector2.h"

namespace
{
    static const int EQUIRECT_MASK_WIDTH = 512;
    static const int EQUIRECT_MASK_HEIGHT = 256;
}  // namespace

OMAF_NS_BEGIN

EquirectangularMask::EquirectangularMask()
    : mTextureHandle(TextureID::Invalid)
    , mVertexBuffer(VertexBufferID::Invalid)
    , mRenderTarget(RenderTargetID::Invalid)
    , mValidMask(false)
{
}

EquirectangularMask::~EquirectangularMask()
{
}

void_t EquirectangularMask::create()
{
    mVertexStream.allocate(1);

    createRenderTarget();
    createShader();
}

void_t EquirectangularMask::createShader()
{
    RendererType::Enum backendType = RenderBackend::getRendererType();

    char_t* vs = OMAF_NULL;
    char_t* ps = OMAF_NULL;

    if (backendType == RendererType::OPENGL || backendType == RendererType::OPENGL_ES)
    {
        static const char_t* vsGL = R"(
                in vec2 aPosition;
                in float aOpacity;

                out float vOpacity;

                void main()
                {
                    vOpacity = aOpacity;
                    gl_Position = vec4(aPosition, 0.0, 1.0);
                }
            )";

        static const char_t* psGL = R"(
                in float vOpacity;

                void main()
                {
                    fragmentColor = vec4(vOpacity, 0.0, 0.0, 0.0);
                }
            )";

        vs = (char_t*) vsGL;
        ps = (char_t*) psGL;
    }
    else if (backendType == RendererType::D3D11 || backendType == RendererType::D3D12)
    {
        static const char_t* dxShader = R"(
                struct VS_INPUT
                {
                    float2 aPosition : POSITION;
                    float aOpacity : OPACITY;
                };

                struct PS_INPUT
                {
                    float4 Position : SV_POSITION;
                    float vOpacity : OPACITY;
                };

                PS_INPUT mainVS(VS_INPUT input)
                {
                    PS_INPUT output;
                    output.Position = float4(input.aPosition, 0.0, 1.0);
                    output.vOpacity = input.aOpacity;
                    return output;
                }

                struct PS_OUTPUT
                {
                    float4 Color : SV_TARGET0;
                };

                PS_OUTPUT mainPS(PS_INPUT input)
                {
                    PS_OUTPUT output;
                    output.Color = float4(input.vOpacity, 0.0, 0.0, 0.0);
                    return output;
                }
            )";

        vs = (char_t*) dxShader;
        ps = vs;
    }

    mShader.create(vs, ps);
}

void_t EquirectangularMask::createRenderTarget()
{
    TextureDesc textureDesc;
    textureDesc.type = TextureType::TEXTURE_2D;
    textureDesc.width = (uint16_t) EQUIRECT_MASK_WIDTH;
    textureDesc.height = (uint16_t) EQUIRECT_MASK_HEIGHT;
    textureDesc.numMipMaps = 1;
    textureDesc.generateMipMaps = false;
    textureDesc.format = TextureFormat::R8;
    textureDesc.samplerState.addressModeU = TextureAddressMode::CLAMP;
    textureDesc.samplerState.addressModeV = TextureAddressMode::CLAMP;
    textureDesc.samplerState.filterMode = TextureFilterMode::POINT;
    textureDesc.data = OMAF_NULL;
    textureDesc.size = EQUIRECT_MASK_WIDTH * EQUIRECT_MASK_HEIGHT;
    textureDesc.renderTarget = true;
    mTextureHandle = RenderBackend::createTexture(textureDesc);

    TextureID attachments[1] = {mTextureHandle};

    RenderTargetTextureDesc renderTargetDesc;
    renderTargetDesc.attachments = attachments;
    renderTargetDesc.numAttachments = 1;
    renderTargetDesc.destroyAttachments = true;
    renderTargetDesc.discardMask = DiscardMask::NONE;

    mRenderTarget = RenderBackend::createRenderTarget(renderTargetDesc);
}

void_t EquirectangularMask::updateClippingAreas(const ClipArea* clippingAreas, const int16_t clipAreaCount)
{
    if (clipAreaCount == 0 || clippingAreas == OMAF_NULL)
    {
        mValidMask = false;
        return;
    }
    mValidMask = true;

    // resize vertex buffer
    size_t size = mVertexStream.getCapacity();
    if ((clipAreaCount * 6) != size)
    {
        if (mVertexBuffer != VertexBufferID::Invalid)
        {
            RenderBackend::destroyVertexBuffer(mVertexBuffer);
            mVertexBuffer = VertexBufferID::Invalid;
        }

        VertexDeclaration vertexDeclaration = VertexDeclaration()
                                                  .begin()
                                                  .addAttribute("aPosition", VertexAttributeFormat::FLOAT32, 2, false)
                                                  .addAttribute("aOpacity", VertexAttributeFormat::FLOAT32, 1, false)
                                                  .end();

        VertexBufferDesc vertexBufferDesc;
        vertexBufferDesc.declaration = vertexDeclaration;
        vertexBufferDesc.access = BufferAccess::DYNAMIC;
        vertexBufferDesc.data = OMAF_NULL;
        ;
        vertexBufferDesc.size = clipAreaCount * 6 * OMAF_SIZE_OF(Vertex);
        mVertexBuffer = RenderBackend::createVertexBuffer(vertexBufferDesc);

        if (mVertexStream.buffer != OMAF_NULL)
            mVertexStream.free();
        mVertexStream.allocate((size_t)(clipAreaCount * 6));
    }

    mVertexStream.clear();

    for (int16_t i = 0; i < clipAreaCount; i++)
    {
        // [-1.0 .. 1.0] range
        float32_t scale = 2.0f;
        float32_t bias = 1.0f;

        ClipArea clippingArea = clippingAreas[i];
        float32_t t = (clippingArea.centerLatitude - clippingArea.spanLatitude * 0.5f + 90.0f) * scale / 180.0f - 1.0f;
        float32_t b = (clippingArea.centerLatitude + clippingArea.spanLatitude * 0.5f + 90.0f) * scale / 180.0f - 1.0f;
        float32_t l =
            (clippingArea.centerLongitude - clippingArea.spanLongitude * 0.5f + 180.0f) * scale / 360.0f - 1.0f;
        float32_t r =
            (clippingArea.centerLongitude + clippingArea.spanLongitude * 0.5f + 180.0f) * scale / 360.0f - 1.0f;
        float32_t opacity = clippingArea.opacity;

        t = clamp(t, -1.0f, 1.0f);
        b = clamp(b, -1.0f, 1.0f);
        l = clamp(l, -1.0f, 1.0f);
        r = clamp(r, -1.0f, 1.0f);
        opacity = clamp(opacity, 0.0f, 1.0f);

        Vertex vertex[6] = {{l, b, opacity}, {r, b, opacity}, {r, t, opacity},
                            {l, b, opacity}, {r, t, opacity}, {l, t, opacity}};
        mVertexStream.add(vertex, 6);
    }

    RenderBackend::bindRenderTarget(mRenderTarget);

    RasterizerState rasterizerState;
    rasterizerState.scissorTestEnabled = false;
    rasterizerState.cullMode = CullMode::NONE;
    RenderBackend::setRasterizerState(rasterizerState);

    DepthStencilState depthState;
    depthState.depthTestEnabled = false;
    depthState.stencilTestEnabled = false;
    RenderBackend::setDepthStencilState(depthState);

    RenderBackend::setViewport(0, 0, EQUIRECT_MASK_WIDTH, EQUIRECT_MASK_HEIGHT);

    RenderBackend::clear(ClearMask::COLOR_BUFFER, 0.0f, 0.0f, 0.0f, 0.0f);

    mShader.bind();

    RenderBackend::bindVertexBuffer(mVertexBuffer);
    RenderBackend::updateVertexBuffer(mVertexBuffer, 0, mVertexStream.buffer);
    RenderBackend::draw(PrimitiveType::TRIANGLE_LIST, 0, (uint32_t) mVertexStream.getCount());
    RenderBackend::bindVertexBuffer(VertexBufferHandle::Invalid);
}

TextureID EquirectangularMask::getMaskTexture()
{
    return mValidMask ? mTextureHandle : TextureID::Invalid;
}

void_t EquirectangularMask::destroy()
{
    mShader.destroy();

    RenderBackend::destroyRenderTarget(mRenderTarget);
    mRenderTarget = RenderTargetID::Invalid;

    RenderBackend::destroyTexture(mTextureHandle);
    mTextureHandle = TextureID::Invalid;

    mVertexStream.free();

    RenderBackend::destroyVertexBuffer(mVertexBuffer);
    mVertexBuffer = VertexBufferID::Invalid;
}

OMAF_NS_END
