
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
#include "Graphics/NVRRenderBackend.h"
#include "Graphics/NVRGraphicsAPIDetection.h"
#include "Graphics/NVRIRenderContext.h"
#include "Graphics/NVRResourceHandleAllocators.h"

#include "Graphics/NVRDebugTextLayer.h"
#include "Graphics/NVRDisplay.h"

#include "Graphics/Null/NVRRenderContextNull.h"

#include "Foundation/NVRLogger.h"

#if OMAF_GRAPHICS_API_OPENGL || OMAF_GRAPHICS_API_OPENGL_ES
#include "Graphics/OpenGL/NVRRenderContextGL.h"
#endif

#if OMAF_GRAPHICS_API_D3D11
#include "Graphics/D3D11/NVRRenderContextD3D11.h"
#endif

#if OMAF_GRAPHICS_API_METAL
#include "Graphics/Metal/NVRRenderContextMTL.h"
#endif

#if OMAF_GRAPHICS_API_VULKAN
#include "Graphics/Vulkan/NVRRenderContextVK.h"
#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(RenderBackend);

const VertexBufferID VertexBufferID::Invalid;
const IndexBufferID IndexBufferID::Invalid;
const ShaderID ShaderID::Invalid;
const ShaderConstantID ShaderConstantID::Invalid;
const TextureID TextureID::Invalid;
const RenderTargetID RenderTargetID::Invalid;

namespace RenderBackend
{
    static MemoryAllocator* mDefaultMemoryAllocator = OMAF_NULL;
    static IRenderContext* mRenderContext = OMAF_NULL;

    static VertexBufferHandleAllocator mVertexBufferHandleAllocator;
    static IndexBufferHandleAllocator mIndexBufferHandleAllocator;
    static ShaderHandleAllocator mShaderHandleAllocator;
    static ShaderConstantHandleAllocator mShaderConstantHandleAllocator;
    static TextureHandleAllocator mTextureHandleAllocator;
    static RenderTargetHandleAllocator mRenderTargetHandleAllocator;

    struct RenderTargetRef
    {
        typedef FixedArray<TextureHandle, OMAF_MAX_RENDER_TARGET_ATTACHMENTS> Attachments;
        Attachments attachmentRefs;
        bool_t destroyAttachments;
    };

    static FixedArray<RenderTargetRef, OMAF_MAX_RENDER_TARGETS> mRenderTargetRefs;

    static FrameStatistics mFrameStatistics;
    static Capabilities mCapabilities;
    static Parameters mParameters;

    static uint16_t mDebugMode;
    static DebugTextLayer mDebugTextLayer;

    void_t getSupportedRenderers(RendererType::Enum* types, uint8_t& numTypes)
    {
        numTypes = 0;

        types[numTypes++] = RendererType::NIL;

#if OMAF_GRAPHICS_API_OPENGL

        types[numTypes++] = RendererType::OPENGL;

#endif

#if OMAF_GRAPHICS_API_OPENGL_ES

        types[numTypes++] = RendererType::OPENGL_ES;

#endif

#if OMAF_GRAPHICS_API_D3D11

        types[numTypes++] = RendererType::D3D11;

#endif

#if OMAF_GRAPHICS_API_D3D12

        types[numTypes++] = RendererType::D3D12;

#endif

#if OMAF_GRAPHICS_API_METAL

        types[numTypes++] = RendererType::METAL;

#endif

#if OMAF_GRAPHICS_API_VULKAN

        types[numTypes++] = RendererType::VULKAN;

#endif
    }

    const char_t* getRendererName(RendererType::Enum type)
    {
        return RendererType::toString(type);
    }

    bool_t onPostCreate()
    {
        if (mRenderContext->getRendererType() != RendererType::NIL)
        {
            float32_t dpiScale = Display::getDisplayDpiScale();

            if (!mDebugTextLayer.create(*mDefaultMemoryAllocator, dpiScale))
            {
                OMAF_LOG_D("Debug text layer cannot be created");

                return false;
            }
        }

        return true;
    }

    void_t onPreDestroy()
    {
        if (mRenderContext->getRendererType() != RendererType::NIL)
        {
            mDebugTextLayer.destroy(*mDefaultMemoryAllocator);
        }
    }

    bool_t create(RendererType::Enum type, Parameters& parameters)
    {
        mParameters = parameters;

        mDefaultMemoryAllocator = MemorySystem::DefaultHeapAllocator();
        mRenderTargetRefs.resize(OMAF_MAX_RENDER_TARGETS);

        // Clear globals
        MemoryZero(&mDebugMode, OMAF_SIZE_OF(mDebugMode));
        MemoryZero(&mCapabilities, OMAF_SIZE_OF(mCapabilities));

        mFrameStatistics.reset();

        mRenderTargetRefs.clear();
        mRenderTargetRefs.resize(OMAF_MAX_RENDER_TARGETS);

        mVertexBufferHandleAllocator.reset();
        mIndexBufferHandleAllocator.reset();
        mShaderHandleAllocator.reset();
        mShaderConstantHandleAllocator.reset();
        mTextureHandleAllocator.reset();
        mRenderTargetHandleAllocator.reset();

        // Create render backend context
        switch (type)
        {
        case RendererType::NIL:
        {
            mRenderContext = (IRenderContext*) OMAF_NEW(*mDefaultMemoryAllocator, RenderContextNull)(type, parameters);
            break;
        }

#if OMAF_GRAPHICS_API_OPENGL_ES
        case RendererType::OPENGL_ES:
        {
            mRenderContext = (IRenderContext*) OMAF_NEW(*mDefaultMemoryAllocator, RenderContextGL)(type, parameters);
            break;
        }
#endif

#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            mRenderContext = (IRenderContext*) OMAF_NEW(*mDefaultMemoryAllocator, RenderContextGL)(type, parameters);
            break;
        }

#endif

#if OMAF_GRAPHICS_API_METAL

        case RendererType::METAL:
        {
            mRenderContext = (IRenderContext*) OMAF_NEW(*mDefaultMemoryAllocator, RenderContextMTL)(type, parameters);
            break;
        }

#endif

#if OMAF_GRAPHICS_API_D3D11

        case RendererType::D3D11:
        {
            mRenderContext = (IRenderContext*) OMAF_NEW(*mDefaultMemoryAllocator, RenderContextD3D11)(type, parameters);
            break;
        }

#endif

        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
        }

        if (!mRenderContext->create())
        {
            return false;
        }

        mRenderContext->getCapabilities(mCapabilities);

        if (!onPostCreate())
        {
            destroy();

            return false;
        }

        return true;
    }

    void_t destroy()
    {
        onPreDestroy();

        // Destroy render backend context
        mRenderContext->destroy();

        OMAF_DELETE(*mDefaultMemoryAllocator, mRenderContext);
        mRenderContext = OMAF_NULL;

        // Clear globals
        MemoryZero(&mDebugMode, OMAF_SIZE_OF(mDebugMode));
        MemoryZero(&mCapabilities, OMAF_SIZE_OF(mCapabilities));
        MemoryZero(&mParameters, OMAF_SIZE_OF(mParameters));

        mFrameStatistics.reset();

        mRenderTargetRefs.clear();
        mRenderTargetRefs.resize(OMAF_MAX_RENDER_TARGETS);

        mVertexBufferHandleAllocator.reset();
        mIndexBufferHandleAllocator.reset();
        mShaderHandleAllocator.reset();
        mShaderConstantHandleAllocator.reset();
        mTextureHandleAllocator.reset();
        mRenderTargetHandleAllocator.reset();

        mDefaultMemoryAllocator = OMAF_NULL;
    }

    RendererType::Enum getRendererType()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mRenderContext->getRendererType();
    }

    const FrameStatistics& getFrameStatistics()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mFrameStatistics;
    }

    const Capabilities& getCapabilities()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mCapabilities;
    }

    const Parameters& getParameters()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mParameters;
    }

    Window getWindow()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mRenderContext->getWindow();
    }

    void_t resetDefaults()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->resetDefaults();
    }

    void_t activate()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->activate();
    }

    void_t deactivate()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->deactivate();
    }

    bool_t supportsTextureFormat(TextureFormat::Enum format)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mCapabilities.textureFormats[format];
    }

    bool_t supportsRenderTargetTextureFormat(TextureFormat::Enum format)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        return mCapabilities.renderTargetTextureFormats[format];
    }

    MemoryBuffer* allocate(uint32_t size)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT_NOT_NULL(mDefaultMemoryAllocator);

        MemoryBuffer* buffer = (MemoryBuffer*) OMAF_ALLOC(*mDefaultMemoryAllocator, OMAF_SIZE_OF(MemoryBuffer) + size);
        buffer->data = (uint8_t*) buffer + OMAF_SIZE_OF(MemoryBuffer);
        buffer->size = 0;
        buffer->offset = 0;
        buffer->capacity = size;

        return buffer;
    }

    void_t free(MemoryBuffer* buffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT_NOT_NULL(mDefaultMemoryAllocator);

        OMAF_FREE(*mDefaultMemoryAllocator, (MemoryBuffer*) (void_t*) buffer);
    }

    // Set states
    void_t setRasterizerState(const RasterizerState& state)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->setRasterizerState(state, false);
    }

    void_t setBlendState(const BlendState& state)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->setBlendState(state, false);
    }

    void_t setDepthStencilState(const DepthStencilState& state)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->setDepthStencilState(state, false);
    }

    void_t setScissors(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->setScissors(Scissors(x, y, width, height), false);
    }

    void_t setScissors(const Scissors& scissors)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->setScissors(scissors, false);
    }

    void_t setViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mDebugTextLayer.setWindow(width, height);

        mRenderContext->setViewport(Viewport(x, y, width, height), false);
    }

    void_t setViewport(const Viewport& viewport)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mDebugTextLayer.setWindow(viewport.width, viewport.height);

        mRenderContext->setViewport(viewport, false);
    }

    void_t clearColor(uint32_t color)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clearColor(color);
    }

    void_t clearColor(float32_t r, float32_t g, float32_t b, float32_t a)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clearColor(packColor(r, g, b, a));
    }

    void_t clearDepth(float32_t value)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clearDepth(value);
    }

    void_t clearStencil(int32_t value)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clearStencil(value);
    }

    void_t clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clear(clearMask, color, depth, stencil);
    }

    void_t
    clear(uint16_t clearMask, float32_t r, float32_t g, float32_t b, float32_t a, float32_t depth, int32_t stencil)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mRenderContext->clear(clearMask, packColor(r, g, b, a), depth, stencil);
    }

    void_t bindVertexBuffer(VertexBufferID vertexBuffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (vertexBuffer != VertexBufferID::Invalid)
        {
            OMAF_ASSERT(mVertexBufferHandleAllocator.isValid(vertexBuffer._handle), "");
            mRenderContext->bindVertexBuffer(vertexBuffer._handle);
        }
        else
        {
            mRenderContext->bindVertexBuffer(VertexBufferID::Invalid._handle);
        }
    }

    void_t bindIndexBuffer(IndexBufferID indexBuffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (indexBuffer != IndexBufferID::Invalid)
        {
            OMAF_ASSERT(mIndexBufferHandleAllocator.isValid(indexBuffer._handle), "");
            mRenderContext->bindIndexBuffer(indexBuffer._handle);
        }
        else
        {
            mRenderContext->bindIndexBuffer(IndexBufferID::Invalid._handle);
        }
    }

    void_t bindShader(ShaderID shader)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (shader != ShaderID::Invalid)
        {
            OMAF_ASSERT(mShaderHandleAllocator.isValid(shader._handle), "");
            mRenderContext->bindShader(shader._handle);
        }
        else
        {
            mRenderContext->bindShader(ShaderID::Invalid._handle);
        }
    }

    void_t bindTexture(TextureID texture, uint16_t textureUnit)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(textureUnit < mCapabilities.numTextureUnits, "");

        if (texture != TextureID::Invalid)
        {
            OMAF_ASSERT(mTextureHandleAllocator.isValid(texture._handle), "");
            mRenderContext->bindTexture(texture._handle, textureUnit);
        }
        else
        {
            mRenderContext->bindTexture(TextureID::Invalid._handle, textureUnit);
        }
    }

    void_t bindTexture(RenderTargetID renderTarget, uint16_t textureAttachment, uint16_t textureUnit)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(textureUnit < mCapabilities.numTextureUnits, "");

        if (renderTarget == RenderTargetID::Invalid)
        {
            mRenderContext->bindTexture(TextureHandle::Invalid, textureUnit);
        }
        else
        {
            OMAF_ASSERT(mRenderTargetHandleAllocator.isValid(renderTarget._handle), "");
            OMAF_ASSERT(textureAttachment < mRenderTargetRefs[renderTarget._handle.index].attachmentRefs.getSize(),
                        "Render target cannot be bound as texture if it doesn't have explicit texture attachments");

            RenderTargetRef& renderTargetRef = mRenderTargetRefs[renderTarget._handle.index];
            TextureHandle& textureHandle = renderTargetRef.attachmentRefs[textureAttachment];

            mRenderContext->bindTexture(textureHandle, textureUnit);
        }
    }

    void_t bindRenderTarget(RenderTargetID renderTarget)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        // Note: Invalid renderTarget handle binds default render target
        if (renderTarget != RenderTargetID::Invalid)
        {
            OMAF_ASSERT(mRenderTargetHandleAllocator.isValid(renderTarget._handle), "");
            mRenderContext->bindRenderTarget(renderTarget._handle);
        }
        else
        {
            mRenderContext->bindRenderTarget(RenderTargetID::Invalid._handle);
        }
    }

    void_t setShaderConstant(ShaderConstantID constant, const void_t* values, uint32_t numValues)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(constant != ShaderConstantID::Invalid, "");
        OMAF_ASSERT_NOT_NULL(values);
        OMAF_ASSERT(numValues != 0, "");
        OMAF_ASSERT(mShaderConstantHandleAllocator.isValid(constant._handle), "");

        mRenderContext->setShaderConstant(constant._handle, values, numValues);
    }

    void_t setShaderConstant(ShaderID shader, ShaderConstantID constant, const void_t* values, uint32_t numValues)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(shader != ShaderID::Invalid, "");
        OMAF_ASSERT(constant != ShaderConstantID::Invalid, "");
        OMAF_ASSERT_NOT_NULL(values);
        OMAF_ASSERT(numValues != 0, "");
        OMAF_ASSERT(mShaderConstantHandleAllocator.isValid(constant._handle), "");

        mRenderContext->setShaderConstant(shader._handle, constant._handle, values, numValues);
    }

    void_t setSamplerState(TextureID texture, const SamplerState& state)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(mTextureHandleAllocator.isValid(texture._handle), "");

        mRenderContext->setSamplerState(texture._handle, state);
    }

    void_t draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");

        mRenderContext->draw(primitiveType, offset, count);

        ++mFrameStatistics.calls.numDrawCalls;
    }

    void_t drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
        OMAF_ASSERT(count != 0, "");

        mRenderContext->drawIndexed(primitiveType, offset, count);

        ++mFrameStatistics.calls.numDrawIndexedCalls;
    }

    void_t
    drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");

        OMAF_ASSERT(mCapabilities.instancedSupport, "Instancing support is not available");

        if (!mCapabilities.instancedSupport)
        {
            return;
        }

        mRenderContext->drawInstanced(primitiveType, offset, vertexCount, instanceCount);

        ++mFrameStatistics.calls.numDrawInstancedCalls;
    }

    void_t drawIndexedInstanced(PrimitiveType::Enum primitiveType,
                                uint32_t offset,
                                uint32_t indexCount,
                                uint32_t instanceCount)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");

        OMAF_ASSERT(mCapabilities.instancedSupport, "Instancing support is not available");

        if (!mCapabilities.instancedSupport)
        {
            return;
        }

        mRenderContext->drawIndexedInstanced(primitiveType, offset, indexCount, instanceCount);

        ++mFrameStatistics.calls.numDrawIndexedInstancedCalls;
    }

    void_t bindComputeImage(uint16_t stage, TextureID texture, ComputeBufferAccess::Enum access)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(stage < OMAF_MAX_COMPUTE_BINDINGS, "");
        OMAF_ASSERT(access > ComputeBufferAccess::INVALID && access < ComputeBufferAccess::COUNT, "");
        OMAF_ASSERT(access != ComputeBufferAccess::NONE, "");

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return;
        }

        if (texture != TextureID::Invalid)
        {
            OMAF_ASSERT(mTextureHandleAllocator.isValid(texture._handle), "");

            mRenderContext->bindComputeImage(stage, texture._handle, access);
        }
        else
        {
            mRenderContext->bindComputeImage(stage, TextureHandle::Invalid, access);
        }
    }

    void_t
    bindComputeImage(uint16_t stage, RenderTargetID renderTarget, uint16_t attachment, ComputeBufferAccess::Enum access)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(stage < OMAF_MAX_COMPUTE_BINDINGS, "");
        OMAF_ASSERT(access > ComputeBufferAccess::INVALID && access < ComputeBufferAccess::COUNT, "");
        OMAF_ASSERT(access != ComputeBufferAccess::NONE, "");

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return;
        }

        if (renderTarget == RenderTargetID::Invalid)
        {
            mRenderContext->bindComputeImage(stage, TextureHandle::Invalid, access);
        }
        else
        {
            OMAF_ASSERT(mRenderTargetHandleAllocator.isValid(renderTarget._handle), "");
            OMAF_ASSERT(attachment < mRenderTargetRefs[renderTarget._handle.index].attachmentRefs.getSize(),
                        "Render target cannot be bound as texture if it doesn't have explicit texture attachments");

            RenderTargetRef& renderTargetRef = mRenderTargetRefs[renderTarget._handle.index];
            TextureHandle& textureHandle = renderTargetRef.attachmentRefs[attachment];

            mRenderContext->bindComputeImage(stage, textureHandle, access);
        }
    }

    void_t bindComputeBuffer(uint16_t stage, VertexBufferID vertexBuffer, ComputeBufferAccess::Enum access)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(stage < OMAF_MAX_COMPUTE_BINDINGS, "");
        OMAF_ASSERT(access > ComputeBufferAccess::INVALID && access < ComputeBufferAccess::COUNT, "");
        OMAF_ASSERT(access != ComputeBufferAccess::NONE, "");

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return;
        }

        if (vertexBuffer != VertexBufferID::Invalid)
        {
            OMAF_ASSERT(mVertexBufferHandleAllocator.isValid(vertexBuffer._handle), "");

            mRenderContext->bindComputeVertexBuffer(stage, vertexBuffer._handle, access);
        }
        else
        {
            mRenderContext->bindComputeVertexBuffer(stage, VertexBufferID::Invalid._handle, access);
        }
    }

    void_t bindComputeBuffer(uint16_t stage, IndexBufferID indexBuffer, ComputeBufferAccess::Enum access)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(stage < OMAF_MAX_COMPUTE_BINDINGS, "");
        OMAF_ASSERT(access > ComputeBufferAccess::INVALID && access < ComputeBufferAccess::COUNT, "");
        OMAF_ASSERT(access != ComputeBufferAccess::NONE, "");

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return;
        }

        if (indexBuffer != IndexBufferID::Invalid)
        {
            OMAF_ASSERT(mIndexBufferHandleAllocator.isValid(indexBuffer._handle), "");

            mRenderContext->bindComputeIndexBuffer(stage, indexBuffer._handle, access);
        }
        else
        {
            mRenderContext->bindComputeIndexBuffer(stage, IndexBufferID::Invalid._handle, access);
        }
    }

    void_t dispatchCompute(ShaderID shader, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return;
        }

        if (shader != ShaderID::Invalid)
        {
            OMAF_ASSERT(mShaderHandleAllocator.isValid(shader._handle), "");

            mRenderContext->dispatchCompute(shader._handle, numGroupsX, numGroupsY, numGroupsZ);

            ++mFrameStatistics.calls.numComputeDispatchCalls;
        }
    }

    void_t submitFrame()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mFrameStatistics.submitFrame();

        mRenderContext->submitFrame();
    }

    VertexBufferID createVertexBuffer(const VertexBufferDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT((desc.access > BufferAccess::INVALID) && (desc.access < BufferAccess::COUNT), "");

        VertexBufferHandle handle = mVertexBufferHandleAllocator.allocate();

        if (mVertexBufferHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createVertexBuffer(handle, desc);

            if (!result)
            {
                mVertexBufferHandleAllocator.release(handle);

                return VertexBufferID::Invalid;
            }

            return handle;
        }

        return VertexBufferID::Invalid;
    }

    IndexBufferID createIndexBuffer(const IndexBufferDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT((desc.access > BufferAccess::INVALID) && (desc.access < BufferAccess::COUNT), "");
        OMAF_ASSERT((desc.format > IndexBufferFormat::INVALID) && (desc.format < IndexBufferFormat::COUNT), "");

        IndexBufferHandle handle = mIndexBufferHandleAllocator.allocate();

        if (mIndexBufferHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createIndexBuffer(handle, desc);

            if (!result)
            {
                mIndexBufferHandleAllocator.release(handle);

                return IndexBufferID::Invalid;
            }

            return handle;
        }

        return IndexBufferID::Invalid;
    }

    ShaderID createShader(const char_t* vertexShader, const char_t* fragmentShader)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT_NOT_NULL(vertexShader);
        OMAF_ASSERT_NOT_NULL(fragmentShader);

        ShaderHandle handle = mShaderHandleAllocator.allocate();

        if (mShaderHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createShader(handle, vertexShader, fragmentShader);

            if (!result)
            {
                mShaderHandleAllocator.release(handle);

                return ShaderID::Invalid;
            }

            return handle;
        }

        return ShaderID::Invalid;
    }

    ShaderID createShader(const char_t* computeShader)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT_NOT_NULL(computeShader);

        OMAF_ASSERT(mCapabilities.computeSupport, "Compute support is not available");

        if (!mCapabilities.computeSupport)
        {
            return ShaderID::Invalid;
        }

        ShaderHandle handle = mShaderHandleAllocator.allocate();

        if (mShaderHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createShader(handle, computeShader);

            if (!result)
            {
                mShaderHandleAllocator.release(handle);

                return ShaderID::Invalid;
            }

            return handle;
        }

        return ShaderID::Invalid;
    }

    ShaderConstantID createShaderConstant(ShaderID shader, const char_t* name, ShaderConstantType::Enum type)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(mShaderHandleAllocator.isValid(shader._handle), "");
        OMAF_ASSERT_NOT_NULL(name);
        OMAF_ASSERT(type != ShaderConstantType::INVALID, "");

        ShaderConstantHandle constant = mShaderConstantHandleAllocator.allocate();

        if (mShaderConstantHandleAllocator.isValid(constant))
        {
            bool_t result = mRenderContext->createShaderConstant(shader._handle, constant, name, type);

            if (!result)
            {
                mShaderConstantHandleAllocator.release(constant);

                return ShaderConstantID::Invalid;
            }

            return constant;
        }

        return ShaderConstantID::Invalid;
    }

    TextureID createTexture(const TextureDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        TextureHandle handle = mTextureHandleAllocator.allocate();

        if (mTextureHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createTexture(handle, desc);

            if (!result)
            {
                mTextureHandleAllocator.release(handle);

                return TextureID::Invalid;
            }

            return handle;
        }

        return TextureID::Invalid;
    }

    TextureID createNativeTexture(const NativeTextureDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        TextureHandle handle = mTextureHandleAllocator.allocate();

        if (mTextureHandleAllocator.isValid(handle))
        {
            bool_t result = mRenderContext->createNativeTexture(handle, desc);

            if (!result)
            {
                mTextureHandleAllocator.release(handle);

                return TextureID::Invalid;
            }

            return handle;
        }

        return TextureID::Invalid;
    }

    RenderTargetID createRenderTarget(const RenderTargetDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        OMAF_ASSERT(
            (desc.colorBufferFormat > TextureFormat::INVALID) && (desc.colorBufferFormat < TextureFormat::COUNT), "");
        OMAF_ASSERT((desc.depthStencilBufferFormat > TextureFormat::INVALID) &&
                        (desc.depthStencilBufferFormat < TextureFormat::COUNT),
                    "");

        TextureID renderTargetTextures[2] = {TextureID::Invalid};

        if (desc.colorBufferFormat != TextureFormat::INVALID)
        {
            TextureDesc colorTargetDesc;
            colorTargetDesc.type = TextureType::TEXTURE_2D;
            colorTargetDesc.width = desc.width;
            colorTargetDesc.height = desc.height;
            colorTargetDesc.numMipMaps = 1;
            colorTargetDesc.format = desc.colorBufferFormat;
            colorTargetDesc.renderTarget = true;

            TextureID colorTargetHandle = createTexture(colorTargetDesc);

            if (colorTargetHandle == TextureID::Invalid)
            {
                mTextureHandleAllocator.release(colorTargetHandle._handle);

                return RenderTargetID::Invalid;
            }

            renderTargetTextures[0] = colorTargetHandle;
        }

        if (desc.depthStencilBufferFormat != TextureFormat::INVALID)
        {
            TextureDesc depthStencilTargetDesc;
            depthStencilTargetDesc.type = TextureType::TEXTURE_2D;
            depthStencilTargetDesc.width = desc.width;
            depthStencilTargetDesc.height = desc.height;
            depthStencilTargetDesc.numMipMaps = 1;
            depthStencilTargetDesc.format = desc.depthStencilBufferFormat;
            depthStencilTargetDesc.renderTarget = true;

            TextureID depthStencilTargetHandle = createTexture(depthStencilTargetDesc);

            if (depthStencilTargetHandle == TextureID::Invalid)
            {
                TextureID colorTargetHandle = renderTargetTextures[0];

                if (colorTargetHandle == TextureID::Invalid)
                {
                    mTextureHandleAllocator.release(colorTargetHandle._handle);
                }

                mTextureHandleAllocator.release(depthStencilTargetHandle._handle);

                return RenderTargetID::Invalid;
            }

            renderTargetTextures[1] = depthStencilTargetHandle;
        }

        RenderTargetTextureDesc rttDesc;
        rttDesc.attachments = renderTargetTextures;
        rttDesc.numAttachments = 2;
        rttDesc.destroyAttachments = true;
        rttDesc.discardMask = desc.discardMask;

        return createRenderTarget(rttDesc);
    }

    RenderTargetID createRenderTarget(const RenderTargetTextureDesc& desc)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT_NOT_NULL(desc.attachments);
        OMAF_ASSERT(desc.numAttachments > 0, "");

        RenderTargetHandle handle = mRenderTargetHandleAllocator.allocate();

        if (mRenderTargetHandleAllocator.isValid(handle))
        {
            RenderTargetRef& renderTargetRef = mRenderTargetRefs[handle.index];
            renderTargetRef.destroyAttachments = desc.destroyAttachments;

            for (uint8_t i = 0; i < desc.numAttachments; ++i)
            {
                TextureHandle handle = ((TextureHandle*) desc.attachments)[i];
                renderTargetRef.attachmentRefs.add(handle);
            }

            bool_t result = mRenderContext->createRenderTarget(handle, (TextureHandle*) desc.attachments,
                                                               desc.numAttachments, desc.discardMask);

            if (!result)
            {
                mRenderTargetHandleAllocator.release(handle);

                return RenderTargetID::Invalid;
            }

            return handle;
        }

        return RenderTargetID::Invalid;
    }

    void_t updateVertexBuffer(VertexBufferID vertexBuffer, uint32_t offset, const MemoryBuffer* buffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(mVertexBufferHandleAllocator.isValid(vertexBuffer._handle), "");

        if (mVertexBufferHandleAllocator.isValid(vertexBuffer._handle))
        {
            mRenderContext->updateVertexBuffer(vertexBuffer._handle, offset, buffer);
        }
    }

    void_t updateIndexBuffer(VertexBufferID vertexBuffer, uint32_t offset, const MemoryBuffer* buffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT(mVertexBufferHandleAllocator.isValid(vertexBuffer._handle), "");

        if (mVertexBufferHandleAllocator.isValid(vertexBuffer._handle))
        {
            mRenderContext->updateIndexBuffer(vertexBuffer._handle, offset, buffer);
        }
    }

    void_t destroyVertexBuffer(VertexBufferID vertexBuffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mVertexBufferHandleAllocator.isValid(vertexBuffer._handle))
        {
            mRenderContext->destroyVertexBuffer(vertexBuffer._handle);
            mVertexBufferHandleAllocator.release(vertexBuffer._handle);
        }
    }

    void_t destroyIndexBuffer(IndexBufferID indexBuffer)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mIndexBufferHandleAllocator.isValid(indexBuffer._handle))
        {
            mRenderContext->destroyIndexBuffer(indexBuffer._handle);
            mIndexBufferHandleAllocator.release(indexBuffer._handle);
        }
    }

    void_t destroyShader(ShaderID shader)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mShaderHandleAllocator.isValid(shader._handle))
        {
            mRenderContext->destroyShader(shader._handle);
            mShaderHandleAllocator.release(shader._handle);
        }
    }

    void_t destroyShaderConstant(ShaderConstantID shaderConstant)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mShaderConstantHandleAllocator.isValid(shaderConstant._handle))
        {
            mRenderContext->destroyShaderConstant(shaderConstant._handle);
            mShaderConstantHandleAllocator.release(shaderConstant._handle);
        }
    }

    void_t destroyTexture(TextureID texture)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mTextureHandleAllocator.isValid(texture._handle))
        {
            mRenderContext->destroyTexture(texture._handle);
            mTextureHandleAllocator.release(texture._handle);
        }
    }

    void_t destroyRenderTarget(RenderTargetID renderTarget)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mRenderTargetHandleAllocator.isValid(renderTarget._handle))
        {
            RenderTargetRef& renderTargetRef = mRenderTargetRefs[renderTarget._handle.index];
            RenderTargetRef::Attachments& attachmentRefs = renderTargetRef.attachmentRefs;

            if (renderTargetRef.destroyAttachments)
            {
                for (size_t a = attachmentRefs.getSize(); a > 0; --a)
                {
                    size_t index = a - 1;

                    TextureHandle& handle = attachmentRefs[index];

                    mRenderContext->destroyTexture(handle);
                    mTextureHandleAllocator.release(handle);

                    attachmentRefs.removeAt(index);
                }
            }

            attachmentRefs.clear();

            mRenderContext->destroyRenderTarget(renderTarget._handle);
            mRenderTargetHandleAllocator.release(renderTarget._handle);
        }
    }

    void_t pushDebugMarker(const char_t* name)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

#if OMAF_DEBUG_BUILD

        mRenderContext->pushDebugMarker(name);

#endif
    }

    void_t popDebugMarker()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

#if OMAF_DEBUG_BUILD

        mRenderContext->popDebugMarker();

#endif
    }

    void_t setDebugMode(uint16_t mode)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        mDebugMode = mode;
    }

    void_t debugPrintClear()
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mRenderContext->getRendererType() == RendererType::NIL)
        {
            return;
        }

        if (mDebugMode & DebugMode::DEBUG_TEXT)
        {
            mDebugTextLayer.clear();
        }
    }

    void_t
    debugPrintFormat(uint16_t x, uint16_t y, uint32_t textColor, uint32_t backgroundColor, const char_t* format, ...)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);
        OMAF_ASSERT_NOT_NULL(format);

        if (mRenderContext->getRendererType() == RendererType::NIL)
        {
            return;
        }

        if (mDebugMode & DebugMode::DEBUG_TEXT)
        {
            va_list args;
            va_start(args, format);

            mDebugTextLayer.debugPrintFormatVar(x, y, textColor, backgroundColor, format, args);

            va_end(args);
        }
    }

    void_t debugPrintFormatVar(uint16_t x,
                               uint16_t y,
                               uint32_t textColor,
                               uint32_t backgroundColor,
                               const utf8_t* format,
                               va_list args)
    {
        OMAF_ASSERT_NOT_NULL(mRenderContext);

        if (mRenderContext->getRendererType() == RendererType::NIL)
        {
            return;
        }

        if (mDebugMode & DebugMode::DEBUG_TEXT)
        {
            va_list copy;
            va_copy(copy, args);

            mDebugTextLayer.debugPrintFormatVar(x, y, textColor, backgroundColor, format, copy);

            va_end(copy);
        }
    }
}  // namespace RenderBackend

OMAF_NS_END
