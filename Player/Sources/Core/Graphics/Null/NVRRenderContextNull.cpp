
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
#include "Graphics/Null/NVRRenderContextNull.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(RenderContextNull);

RenderContextNull::RenderContextNull(RendererType::Enum type, RenderBackend::Parameters& parameters)
    : IRenderContext(type, parameters)
{
    resetDefaults();
}

RenderContextNull::~RenderContextNull()
{
}

bool_t RenderContextNull::create()
{
    return true;
}

void_t RenderContextNull::destroy()
{
}

void_t RenderContextNull::activate()
{
}

void_t RenderContextNull::deactivate()
{
}

RenderBackend::Window RenderContextNull::getWindow() const
{
    RenderBackend::Window window;
    window.width = 0;
    window.height = 0;

    return window;
}

void_t RenderContextNull::getCapabilities(RenderBackend::Capabilities& capabitilies) const
{
}

void_t RenderContextNull::resetDefaults()
{
    IRenderContext::resetDefaults();
}

bool_t RenderContextNull::supportsTextureFormat(TextureFormat::Enum format)
{
    return true;
}

bool_t RenderContextNull::supportsRenderTargetFormat(TextureFormat::Enum format)
{
    return true;
}

void_t RenderContextNull::setRasterizerState(const RasterizerState& rasterizerState, bool_t forced)
{
    OMAF_UNUSED_VARIABLE(rasterizerState);
    OMAF_UNUSED_VARIABLE(forced);
}

void_t RenderContextNull::setBlendState(const BlendState& blendState, bool_t forced)
{
    OMAF_UNUSED_VARIABLE(blendState);
    OMAF_UNUSED_VARIABLE(forced);
}

void_t RenderContextNull::setDepthStencilState(const DepthStencilState& depthStencilState, bool_t forced)
{
    OMAF_UNUSED_VARIABLE(depthStencilState);
    OMAF_UNUSED_VARIABLE(forced);
}

void_t RenderContextNull::setScissors(const Scissors& scissors, bool_t forced)
{
    OMAF_UNUSED_VARIABLE(scissors);
    OMAF_UNUSED_VARIABLE(forced);
}

void_t RenderContextNull::setViewport(const Viewport& viewport, bool_t forced)
{
    OMAF_UNUSED_VARIABLE(viewport);
    OMAF_UNUSED_VARIABLE(forced);
}

void_t RenderContextNull::clearColor(uint32_t color)
{
    OMAF_UNUSED_VARIABLE(color);
}

void_t RenderContextNull::clearDepth(float32_t value)
{
    OMAF_UNUSED_VARIABLE(value);
}

void_t RenderContextNull::clearStencil(int32_t value)
{
    OMAF_UNUSED_VARIABLE(value);
}

void_t RenderContextNull::clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil)
{
    OMAF_UNUSED_VARIABLE(clearMask);
    OMAF_UNUSED_VARIABLE(color);
    OMAF_UNUSED_VARIABLE(depth);
    OMAF_UNUSED_VARIABLE(stencil);
}

void_t RenderContextNull::bindVertexBuffer(VertexBufferHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::bindIndexBuffer(IndexBufferHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::bindShader(ShaderHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::bindTexture(TextureHandle handle, uint16_t textureUnit)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(textureUnit);
}

void_t RenderContextNull::bindRenderTarget(RenderTargetHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::setShaderConstant(ShaderConstantHandle handle, const void_t* values, uint32_t numValues)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(values);
    OMAF_UNUSED_VARIABLE(numValues);
}

void_t RenderContextNull::setShaderConstant(ShaderHandle shaderHandle,
                                            ShaderConstantHandle constantHandle,
                                            const void_t* values,
                                            uint32_t numValues)
{
    OMAF_UNUSED_VARIABLE(shaderHandle);
    OMAF_UNUSED_VARIABLE(constantHandle);
    OMAF_UNUSED_VARIABLE(values);
    OMAF_UNUSED_VARIABLE(numValues);
}

void_t RenderContextNull::setSamplerState(TextureHandle handle, const SamplerState& samplerState)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(samplerState);
}

void_t RenderContextNull::draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
{
    OMAF_UNUSED_VARIABLE(primitiveType);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(count);
}

void_t RenderContextNull::drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
{
    OMAF_UNUSED_VARIABLE(primitiveType);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(count);
}

void_t RenderContextNull::drawInstanced(PrimitiveType::Enum primitiveType,
                                        uint32_t offset,
                                        uint32_t vertexCount,
                                        uint32_t instanceCount)
{
    OMAF_UNUSED_VARIABLE(primitiveType);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(vertexCount);
    OMAF_UNUSED_VARIABLE(instanceCount);
}

void_t RenderContextNull::drawIndexedInstanced(PrimitiveType::Enum primitiveType,
                                               uint32_t offset,
                                               uint32_t vertexCount,
                                               uint32_t instanceCount)
{
    OMAF_UNUSED_VARIABLE(primitiveType);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(vertexCount);
    OMAF_UNUSED_VARIABLE(instanceCount);
}

void_t RenderContextNull::bindComputeImage(uint16_t stage, TextureHandle handle, ComputeBufferAccess::Enum access)
{
    OMAF_UNUSED_VARIABLE(stage);
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(access);
}

void_t RenderContextNull::bindComputeVertexBuffer(uint16_t stage,
                                                  VertexBufferHandle handle,
                                                  ComputeBufferAccess::Enum access)
{
    OMAF_UNUSED_VARIABLE(stage);
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(access);
}

void_t RenderContextNull::bindComputeIndexBuffer(uint16_t stage,
                                                 IndexBufferHandle handle,
                                                 ComputeBufferAccess::Enum access)
{
    OMAF_UNUSED_VARIABLE(stage);
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(access);
}

void_t
RenderContextNull::dispatchCompute(ShaderHandle handle, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(numGroupsX);
    OMAF_UNUSED_VARIABLE(numGroupsY);
    OMAF_UNUSED_VARIABLE(numGroupsZ);
}

void_t RenderContextNull::submitFrame()
{
}

bool_t RenderContextNull::createVertexBuffer(VertexBufferHandle handle, const VertexBufferDesc& desc)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(desc);

    return true;
}

bool_t RenderContextNull::createIndexBuffer(IndexBufferHandle handle, const IndexBufferDesc& desc)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(desc);

    return true;
}

bool_t RenderContextNull::createShader(ShaderHandle handle, const char_t* vertexShader, const char_t* fragmentShader)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(vertexShader);
    OMAF_UNUSED_VARIABLE(fragmentShader);

    return true;
}

bool_t RenderContextNull::createShader(ShaderHandle handle, const char_t* computeShader)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(computeShader);

    return true;
}

bool_t RenderContextNull::createShaderConstant(ShaderHandle shaderHandle,
                                               ShaderConstantHandle constantHandle,
                                               const char_t* name,
                                               ShaderConstantType::Enum type)
{
    OMAF_UNUSED_VARIABLE(shaderHandle);
    OMAF_UNUSED_VARIABLE(constantHandle);
    OMAF_UNUSED_VARIABLE(name);
    OMAF_UNUSED_VARIABLE(type);

    return true;
}

bool_t RenderContextNull::createTexture(TextureHandle handle, const TextureDesc& desc)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(desc);

    return true;
}

bool_t RenderContextNull::createNativeTexture(TextureHandle handle, const NativeTextureDesc& desc)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(desc);

    return true;
}

bool_t RenderContextNull::createRenderTarget(RenderTargetHandle handle,
                                             TextureHandle* attachments,
                                             uint8_t numAttachments,
                                             int16_t discardMask)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(attachments);
    OMAF_UNUSED_VARIABLE(numAttachments);
    OMAF_UNUSED_VARIABLE(discardMask);

    return true;
}

void_t RenderContextNull::updateVertexBuffer(VertexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(buffer);
}

void_t RenderContextNull::updateIndexBuffer(IndexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
{
    OMAF_UNUSED_VARIABLE(handle);
    OMAF_UNUSED_VARIABLE(offset);
    OMAF_UNUSED_VARIABLE(buffer);
}

void_t RenderContextNull::destroyVertexBuffer(VertexBufferHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::destroyIndexBuffer(IndexBufferHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::destroyShader(ShaderHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::destroyShaderConstant(ShaderConstantHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::destroyTexture(TextureHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::destroyRenderTarget(RenderTargetHandle handle)
{
    OMAF_UNUSED_VARIABLE(handle);
}

void_t RenderContextNull::pushDebugMarker(const char_t* name)
{
    OMAF_UNUSED_VARIABLE(name);
}

void_t RenderContextNull::popDebugMarker()
{
}

OMAF_NS_END
