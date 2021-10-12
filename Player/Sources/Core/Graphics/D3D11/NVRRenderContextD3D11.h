
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

#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRIRenderContext.h"

#include "Graphics/D3D11/NVRIndexBufferD3D11.h"
#include "Graphics/D3D11/NVRRenderTargetD3D11.h"
#include "Graphics/D3D11/NVRShaderConstantD3D11.h"
#include "Graphics/D3D11/NVRShaderD3D11.h"
#include "Graphics/D3D11/NVRTextureD3D11.h"
#include "Graphics/D3D11/NVRVertexBufferD3D11.h"
#if OMAF_PLATFORM_UWP
#define WAIT_FOR_FRAME 1
#else
#define WAIT_FOR_FRAME 0
#endif

OMAF_NS_BEGIN
class RenderContextD3D11 : public IRenderContext
{
public:
    RenderContextD3D11(RendererType::Enum type, RenderBackend::Parameters& parameters);
    virtual ~RenderContextD3D11();

    virtual bool_t create();
    virtual void_t destroy();

    virtual void_t activate();
    virtual void_t deactivate();

    virtual RenderBackend::Window getWindow() const;
    virtual void_t getCapabilities(RenderBackend::Capabilities& capabitilies) const;

    virtual void_t resetDefaults();

    virtual bool_t supportsTextureFormat(TextureFormat::Enum format);
    virtual bool_t supportsRenderTargetFormat(TextureFormat::Enum format);

    // Set states
    virtual void_t setRasterizerState(const RasterizerState& rasterizerState, bool_t forced);
    virtual void_t setBlendState(const BlendState& blendState, bool_t forced);
    virtual void_t setDepthStencilState(const DepthStencilState& depthStencilState, bool_t forced);

    virtual void_t setScissors(const Scissors& scissors, bool_t forced);
    virtual void_t setViewport(const Viewport& viewport, bool_t forced);

    // Clear buffers
    virtual void_t clearColor(uint32_t color);
    virtual void_t clearDepth(float32_t value);
    virtual void_t clearStencil(int32_t value);
    virtual void_t clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil);

    // Bind resources
    virtual void_t bindVertexBuffer(VertexBufferHandle handle);
    virtual void_t bindIndexBuffer(IndexBufferHandle handle);
    virtual void_t bindShader(ShaderHandle handle);
    virtual void_t bindTexture(TextureHandle handle, uint16_t textureUnit);
    virtual void_t bindRenderTarget(RenderTargetHandle handle);

    // Set shader constants
    virtual void_t setShaderConstant(ShaderConstantHandle handle, const void_t* values, uint32_t numValues);
    virtual void_t setShaderConstant(ShaderHandle shaderHandle,
                                     ShaderConstantHandle constantHandle,
                                     const void_t* values,
                                     uint32_t numValues);

    // Set texture properties
    virtual void_t setSamplerState(TextureHandle handle, const SamplerState& samplerState);

    // Draw calls
    virtual void_t draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count);
    virtual void_t drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count);

    virtual void_t
    drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount);
    virtual void_t drawIndexedInstanced(PrimitiveType::Enum primitiveType,
                                        uint32_t offset,
                                        uint32_t vertexCount,
                                        uint32_t instanceCount);

    // Compute calls
    virtual void_t bindComputeImage(uint16_t stage, TextureHandle handle, ComputeBufferAccess::Enum access);

    virtual void_t bindComputeVertexBuffer(uint16_t stage, VertexBufferHandle handle, ComputeBufferAccess::Enum access);
    virtual void_t bindComputeIndexBuffer(uint16_t stage, IndexBufferHandle handle, ComputeBufferAccess::Enum access);

    virtual void_t dispatchCompute(ShaderHandle handle, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ);

    // Frame calls
    virtual void_t submitFrame();

    // Create resources
    virtual bool_t createVertexBuffer(VertexBufferHandle handle, const VertexBufferDesc& desc);
    virtual bool_t createIndexBuffer(IndexBufferHandle handle, const IndexBufferDesc& desc);

    virtual bool_t createShader(ShaderHandle handle, const char_t* vertexShader, const char_t* fragmentShader);
    virtual bool_t createShader(ShaderHandle handle, const char_t* computeShader);

    virtual bool_t createShaderConstant(ShaderHandle shaderHandle,
                                        ShaderConstantHandle constantHandle,
                                        const char_t* name,
                                        ShaderConstantType::Enum type);

    virtual bool_t createTexture(TextureHandle handle, const TextureDesc& desc);
    virtual bool_t createNativeTexture(TextureHandle handle, const NativeTextureDesc& desc);

    virtual bool_t createRenderTarget(RenderTargetHandle handle,
                                      TextureHandle* attachments,
                                      uint8_t numAttachments,
                                      int16_t discardMask);

    // Update resources
    virtual void_t updateVertexBuffer(VertexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer);
    virtual void_t updateIndexBuffer(IndexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer);

    // Destroy resources
    virtual void_t destroyVertexBuffer(VertexBufferHandle handle);
    virtual void_t destroyIndexBuffer(IndexBufferHandle handle);

    virtual void_t destroyShader(ShaderHandle handle);
    virtual void_t destroyShaderConstant(ShaderConstantHandle handle);

    virtual void_t destroyTexture(TextureHandle handle);

    virtual void_t destroyRenderTarget(RenderTargetHandle handle);

    // Debugging
    virtual void_t pushDebugMarker(const char_t* name);
    virtual void_t popDebugMarker();

private:
    UINT getMSAASampleCount(DXGI_FORMAT format, UINT sampleCount);

private:
    RenderBackend::Capabilities mCapabilities;

    FixedArray<VertexBufferD3D11, OMAF_MAX_VERTEX_BUFFERS> mVertexBuffers;
    FixedArray<IndexBufferD3D11, OMAF_MAX_INDEX_BUFFERS> mIndexBuffers;
    FixedArray<ShaderD3D11, OMAF_MAX_SHADERS> mShaders;
    FixedArray<ShaderConstantD3D11, OMAF_MAX_SHADER_CONSTANTS> mShaderConstants;
    FixedArray<TextureD3D11, OMAF_MAX_TEXTURES> mTextures;
    FixedArray<RenderTargetD3D11, OMAF_MAX_RENDER_TARGETS> mRenderTargets;

    ID3D11Device* mDevice;
    ID3D11DeviceContext* mDeviceContext;

#if OMAF_DEBUG_BUILD
    ID3DUserDefinedAnnotation* mAnnotation;
#endif
    ID3D11RenderTargetView* mCurrentColor;
    ID3D11DepthStencilView* mCurrentDepthStencil;

    ID3D11RasterizerState* mRasterizerState;
    D3D11_RASTERIZER_DESC mRasterizerDesc;

    ID3D11BlendState* mBlendState;
    FLOAT mBlendFactor[4];
    D3D11_BLEND_DESC mBlendDesc;

    ID3D11DepthStencilState* mDepthStencilState;
    D3D11_DEPTH_STENCIL_DESC mDepthStencilDesc;

    bool_t mMSAAEnabled;

    // We need custom "quad" rendering to clear selected parts of the rendertarget (D3D only supports full clears!)
    struct QuadState
    {
        QuadState()
            : mNullState(OMAF_NULL)
            , mColorState(OMAF_NULL)
            , mKeepState(OMAF_NULL)
            , mDepthState(OMAF_NULL)
            , mStencilState(OMAF_NULL)
            , mDepthStencilState(OMAF_NULL)
            , mScissorEnabled(OMAF_NULL)
            , mScissorDisabled(OMAF_NULL)
        {
        }
        ID3D11BlendState* mNullState;                 // Dont modify color
        ID3D11BlendState* mColorState;                // Modify color
        ID3D11DepthStencilState* mKeepState;          // Dont modify depth or stencil
        ID3D11DepthStencilState* mDepthState;         // Modify only depth
        ID3D11DepthStencilState* mStencilState;       // Modify only stencil
        ID3D11DepthStencilState* mDepthStencilState;  // Modify both depth and stencil
        ID3D11RasterizerState* mScissorEnabled;
        ID3D11RasterizerState* mScissorDisabled;
        ShaderConstantD3D11 mClearColor, mClearDepth;
        ShaderD3D11 mShader;
    } mQuadState;
    void_t createClearStates();
    void_t clippedClear(uint16_t clearMask, float clearColor[], float clearDepth, uint32_t stencil);
    void_t destroyClearStates();
#if WAIT_FOR_FRAME
    ID3D11Query* mQuery;
#endif
};
OMAF_NS_END
