
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

#include "Graphics/NVRBlendState.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRClearMask.h"
#include "Graphics/NVRColor.h"
#include "Graphics/NVRDebugMode.h"
#include "Graphics/NVRDepthStencilState.h"
#include "Graphics/NVRDiscardMask.h"
#include "Graphics/NVRHandles.h"
#include "Graphics/NVRIRenderContext.h"
#include "Graphics/NVRPrimitiveType.h"
#include "Graphics/NVRRasterizerState.h"
#include "Graphics/NVRRendererType.h"
#include "Graphics/NVRResourceDescriptors.h"
#include "Graphics/NVRResourceHandleAllocators.h"
#include "Graphics/NVRSamplerState.h"
#include "Graphics/NVRScissors.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRVertexDeclaration.h"
#include "Graphics/NVRViewport.h"

OMAF_NS_BEGIN

namespace RenderBackend
{
    // Common
    void_t getSupportedRenderers(RendererType::Enum* types, uint8_t& numTypes);
    const char_t* getRendererName(RendererType::Enum type);

    bool_t create(RendererType::Enum type, Parameters& parameters);
    void_t destroy();

    const FrameStatistics& getFrameStatistics();
    const Capabilities& getCapabilities();
    const Parameters& getParameters();

    Window getWindow();

    RendererType::Enum getRendererType();

    void_t activate();
    void_t deactivate();

    void_t resetDefaults();

    bool_t supportsTextureFormat(TextureFormat::Enum format);
    bool_t supportsRenderTargetTextureFormat(TextureFormat::Enum format);

    // Memory management
    MemoryBuffer* allocate(uint32_t size);
    void_t free(MemoryBuffer* buffer);

    // Set states
    void_t setRasterizerState(const RasterizerState& state);
    void_t setBlendState(const BlendState& state);
    void_t setDepthStencilState(const DepthStencilState& state);

    void_t setScissors(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void_t setScissors(const Scissors& scissors);

    void_t setViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
    void_t setViewport(const Viewport& viewport);

    // Clear buffers
    void_t clearColor(uint32_t color);
    void_t clearColor(float32_t r, float32_t g, float32_t b, float32_t a);

    void_t clearDepth(float32_t value);
    void_t clearStencil(int32_t value);

    void_t clear(uint16_t clearMask, uint32_t color, float32_t depth = 1.0f, int32_t stencil = 0);
    void_t clear(uint16_t clearMask,
                 float32_t r,
                 float32_t g,
                 float32_t b,
                 float32_t a,
                 float32_t depth = 1.0f,
                 int32_t stencil = 0);

    // Bind resources
    void_t bindVertexBuffer(VertexBufferID vertexBuffer);
    void_t bindIndexBuffer(IndexBufferID indexBuffer);
    void_t bindShader(ShaderID shader);
    void_t bindTexture(TextureID texture, uint16_t textureUnit = 0);
    void_t bindTexture(RenderTargetID renderTarget, uint16_t textureAttachment, uint16_t textureUnit = 0);
    void_t bindRenderTarget(RenderTargetID renderTarget);

    // Set shader constants
    void_t setShaderConstant(ShaderConstantID constant, const void_t* values, uint32_t numValues = 1);
    void_t setShaderConstant(ShaderID shader, ShaderConstantID constant, const void_t* values, uint32_t numValues = 1);

    // Set texture properties
    void_t setSamplerState(TextureID texture, const SamplerState& state);

    // Draw calls
    void_t draw(PrimitiveType::Enum primitiveType, uint32_t offset = 0, uint32_t count = OMAF_UINT32_MAX);
    void_t drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset = 0, uint32_t count = OMAF_UINT32_MAX);

    void_t
    drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount);
    void_t drawIndexedInstanced(PrimitiveType::Enum primitiveType,
                                uint32_t offset,
                                uint32_t indexCount,
                                uint32_t instanceCount);

    // Compute calls
    void_t bindComputeImage(uint16_t stage, TextureID texture, ComputeBufferAccess::Enum access);
    void_t bindComputeImage(uint16_t stage,
                            RenderTargetID renderTarget,
                            uint16_t attachment,
                            ComputeBufferAccess::Enum access);

    void_t bindComputeBuffer(uint16_t stage, VertexBufferID vertexBuffer, ComputeBufferAccess::Enum access);
    void_t bindComputeBuffer(uint16_t stage, IndexBufferID indexBuffer, ComputeBufferAccess::Enum access);

    void_t dispatchCompute(ShaderID shader, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ);

    // Frame calls
    void_t submitFrame();

    // Create resources
    VertexBufferID createVertexBuffer(const VertexBufferDesc& desc);
    IndexBufferID createIndexBuffer(const IndexBufferDesc& desc);

    ShaderID createShader(const char_t* vertexShader, const char_t* fragmentShader);
    ShaderID createShader(const char_t* computeShader);

    ShaderConstantID createShaderConstant(ShaderID shader, const char_t* name, ShaderConstantType::Enum type);

    TextureID createTexture(const TextureDesc& desc);
    TextureID createNativeTexture(const NativeTextureDesc& desc);

    RenderTargetID createRenderTarget(const RenderTargetDesc& desc);
    RenderTargetID createRenderTarget(const RenderTargetTextureDesc& desc);

    // Update resources
    void_t updateVertexBuffer(VertexBufferID vertexBuffer, uint32_t offset, const MemoryBuffer* buffer);
    void_t updateIndexBuffer(VertexBufferID vertexBuffer, uint32_t offset, const MemoryBuffer* buffer);

    // Destroy resources
    void_t destroyVertexBuffer(VertexBufferID vertexBuffer);
    void_t destroyIndexBuffer(IndexBufferID indexBuffer);

    void_t destroyShader(ShaderID shader);
    void_t destroyShaderConstant(ShaderConstantID shaderConstant);

    void_t destroyTexture(TextureID texture);

    void_t destroyRenderTarget(RenderTargetID renderTarget);

    // Debugging
    void_t pushDebugMarker(const char_t* name);
    void_t popDebugMarker();

    void_t setDebugMode(uint16_t mode);

    void_t debugPrintClear();

    void_t
    debugPrintFormat(uint16_t x, uint16_t y, uint32_t textColor, uint32_t backgroundColor, const char_t* format, ...);
    void_t debugPrintFormatVar(uint16_t x,
                               uint16_t y,
                               uint32_t textColor,
                               uint32_t backgroundColor,
                               const char_t* format,
                               va_list args);
}  // namespace RenderBackend

OMAF_NS_END
