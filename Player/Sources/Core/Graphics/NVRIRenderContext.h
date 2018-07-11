
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRFixedString.h"

#include "Graphics/NVRConfig.h"
#include "Graphics/NVRRendererType.h"
#include "Graphics/NVRRasterizerState.h"
#include "Graphics/NVRBlendState.h"
#include "Graphics/NVRDepthStencilState.h"
#include "Graphics/NVRSamplerState.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRPrimitiveType.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRColor.h"
#include "Graphics/NVRViewport.h"
#include "Graphics/NVRScissors.h"
#include "Graphics/NVRVertexDeclaration.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Graphics/NVRColorMask.h"
#include "Graphics/NVRResourceHandleAllocators.h"
#include "Graphics/NVRDiscardMask.h"
#include "Graphics/NVRResourceDescriptors.h"
#include "Graphics/NVRClearMask.h"
#include "Graphics/NVRGraphicsAPIDetection.h"

OMAF_NS_BEGIN

namespace RenderBackend
{
    struct Window
    {
        uint32_t width;
        uint32_t height;

        float32_t scale;
    };

    struct Parameters
    {
        #if OMAF_PLATFORM_WINDOWS
        
            void_t* hWnd;
        
        
        #endif

        #if OMAF_GRAPHICS_API_D3D11

            void_t* d3dDevice;

        #endif

        Window window;
    };

    struct Capabilities
    {
        RendererType::Enum rendererType;
        
        uint16_t numTextureUnits;
        
        uint16_t maxTexture2DSize;
        uint16_t maxTexture3DSize;
        uint16_t maxTextureCubeSize;
        
        uint16_t maxRenderTargetAttachments;
        
        bool_t textureFormats[TextureFormat::COUNT];
        bool_t renderTargetTextureFormats[TextureFormat::COUNT];
        
        bool_t instancedSupport;

        bool_t computeSupport;
        int32_t maxComputeWorkGroupCount[3];
        int32_t maxComputeWorkGroupSize[3];
        int32_t maxComputeWorkGroupInvocations;
        
        FixedString128 apiVersionStr;
        FixedString128 apiShaderVersionStr;
        
        FixedString128 vendorStr;
        FixedString128 deviceStr;

        FixedString128 memoryInfo;
        
        Capabilities()
        : rendererType(RendererType::INVALID)
        , numTextureUnits(0)
        , maxTexture2DSize(0)
        , maxTexture3DSize(0)
        , maxTextureCubeSize(0)
        , maxRenderTargetAttachments(0)
        , instancedSupport(false)
        , computeSupport(false)
        , maxComputeWorkGroupInvocations(0)
        , apiVersionStr(OMAF_NULL)
        , apiShaderVersionStr(OMAF_NULL)
        , vendorStr(OMAF_NULL)
        , deviceStr(OMAF_NULL)
        {
            MemorySet(textureFormats, false, OMAF_SIZE_OF(textureFormats));
            MemorySet(renderTargetTextureFormats, false, OMAF_SIZE_OF(renderTargetTextureFormats));

            MemoryZero(&maxComputeWorkGroupCount, OMAF_SIZE_OF(maxComputeWorkGroupCount));
            MemoryZero(&maxComputeWorkGroupSize, OMAF_SIZE_OF(maxComputeWorkGroupSize));
        }
    };
    
    struct FrameStatistics
    {
        uint64_t numFrames;
        
        struct Calls
        {
            uint32_t numDrawCalls;
            uint32_t numDrawIndexedCalls;
            
            uint32_t numDrawInstancedCalls;
            uint32_t numDrawIndexedInstancedCalls;
            
            uint32_t numComputeDispatchCalls;
        };
        
        Calls calls;
        
        FrameStatistics()
        : numFrames(0)
        {
            MemoryZero(&calls, OMAF_SIZE_OF(calls));
        }
        
        void_t reset()
        {
            MemoryZero(this, OMAF_SIZE_OF(FrameStatistics));
        }

        void_t submitFrame()
        {
            MemoryZero(&calls, OMAF_SIZE_OF(calls));
            
            numFrames++;
        }
    };
}

class IRenderContext
{
    public:
    
        IRenderContext(RendererType::Enum type, RenderBackend::Parameters& parameters);
        virtual ~IRenderContext();
    
        virtual bool_t create() = 0;
        virtual void_t destroy() = 0;
    
        virtual void_t activate() = 0;
        virtual void_t deactivate() = 0;

        RendererType::Enum getRendererType() const;
       
        virtual RenderBackend::Window getWindow() const = 0;
        virtual void_t getCapabilities(RenderBackend::Capabilities& capabitilies) const = 0;

        virtual void_t resetDefaults();
    
        virtual bool_t supportsTextureFormat(TextureFormat::Enum format) = 0;
        virtual bool_t supportsRenderTargetFormat(TextureFormat::Enum format) = 0;
    
        // Set states
        virtual void_t setRasterizerState(const RasterizerState& rasterizerState, bool_t forced) = 0;
        virtual void_t setBlendState(const BlendState& blendState, bool_t forced) = 0;
        virtual void_t setDepthStencilState(const DepthStencilState& depthStencilState, bool_t forced) = 0;
    
        virtual void_t setScissors(const Scissors& scissors, bool_t forced) = 0;
        virtual void_t setViewport(const Viewport& viewport, bool_t forced) = 0;
        
        // Clear buffers
        virtual void_t clearColor(uint32_t color) = 0;
        virtual void_t clearDepth(float32_t value) = 0;
        virtual void_t clearStencil(int32_t value) = 0;
        virtual void_t clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil) = 0;
    
        // Bind resources
        virtual void_t bindVertexBuffer(VertexBufferHandle handle) = 0;
        virtual void_t bindIndexBuffer(IndexBufferHandle handle) = 0;
        virtual void_t bindShader(ShaderHandle handle) = 0;
        virtual void_t bindTexture(TextureHandle handle, uint16_t textureUnit) = 0;
        virtual void_t bindRenderTarget(RenderTargetHandle handle) = 0;
    
        // Set shader constants
        virtual void_t setShaderConstant(ShaderConstantHandle handle, const void_t* values, uint32_t numValues) = 0;
        virtual void_t setShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const void_t* values, uint32_t numValues) = 0;

        // Set texture properties
        virtual void_t setSamplerState(TextureHandle handle, const SamplerState& samplerState) = 0;
    
        // Draw calls
        virtual void_t draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count) = 0;
        virtual void_t drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count) = 0;
    
        virtual void_t drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount) = 0;
        virtual void_t drawIndexedInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount) = 0;

        // Compute calls
        virtual void_t bindComputeImage(uint16_t stage, TextureHandle handle, ComputeBufferAccess::Enum access) = 0;

        virtual void_t bindComputeVertexBuffer(uint16_t stage, VertexBufferHandle handle, ComputeBufferAccess::Enum access) = 0;
        virtual void_t bindComputeIndexBuffer(uint16_t stage, IndexBufferHandle handle, ComputeBufferAccess::Enum access) = 0;

        virtual void_t dispatchCompute(ShaderHandle handle, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ) = 0;

        // Frame calls
        virtual void_t submitFrame() = 0;

        // Create resources
        virtual bool_t createVertexBuffer(VertexBufferHandle handle, const VertexBufferDesc& desc) = 0;
        virtual bool_t createIndexBuffer(IndexBufferHandle handle, const IndexBufferDesc& desc) = 0;
        
        virtual bool_t createShader(ShaderHandle handle, const char_t* vertexShader, const char_t* fragmentShader) = 0;
        virtual bool_t createShader(ShaderHandle handle, const char_t* computeShader) = 0;
    
        virtual bool_t createShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const char_t* name, ShaderConstantType::Enum type) = 0;
        
        virtual bool_t createTexture(TextureHandle handle, const TextureDesc& desc) = 0;
        virtual bool_t createNativeTexture(TextureHandle handle, const NativeTextureDesc& desc) = 0;

        virtual bool_t createRenderTarget(RenderTargetHandle handle, TextureHandle* attachments, uint8_t numAttachments, int16_t discardMask) = 0;

        // Update resources
        virtual void_t updateVertexBuffer(VertexBufferHandle vertexBuffer, uint32_t offset, const MemoryBuffer* buffer) = 0;
        virtual void_t updateIndexBuffer(IndexBufferHandle vertexBuffer, uint32_t offset, const MemoryBuffer* buffer) = 0;

        // Destroy resources
        virtual void_t destroyVertexBuffer(VertexBufferHandle handle) = 0;
        virtual void_t destroyIndexBuffer(IndexBufferHandle handle) = 0;
    
        virtual void_t destroyShader(ShaderHandle handle) = 0;
        virtual void_t destroyShaderConstant(ShaderConstantHandle handle) = 0;
    
        virtual void_t destroyTexture(TextureHandle handle) = 0;
    
        virtual void_t destroyRenderTarget(RenderTargetHandle handle) = 0;
    
        // Debugging
        virtual void_t pushDebugMarker(const char_t* name) = 0;
        virtual void_t popDebugMarker() = 0;
    
    private:
        
        OMAF_NO_DEFAULT(IRenderContext);
        OMAF_NO_COPY(IRenderContext);
        OMAF_NO_ASSIGN(IRenderContext);
    
    protected:
    
        RendererType::Enum mRendererType;

        RenderBackend::Parameters& mParameters;

        BlendState mActiveBlendState;
        DepthStencilState mActiveDepthStencilState;
        RasterizerState mActiveRasterizerState;
    
        Viewport mViewport;
        Scissors mScissors;

        VertexBufferHandle mActiveVertexBuffer;
        IndexBufferHandle mActiveIndexBuffer;
        ShaderHandle mActiveShader;
        TextureHandle mActiveTexture[OMAF_MAX_TEXTURE_UNITS];
        RenderTargetHandle mActiveRenderTarget;

        struct ComputeResourceType
        {
            enum Enum
            {
                INVALID = -1,

                IMAGE_BUFFER,
                VERTEX_BUFFER,
                INDEX_BUFFER,

                COUNT
            };
        };

        struct ComputeResourceBinding
        {
            uint32_t handle;
            ComputeResourceType::Enum type;

            ComputeResourceBinding()
            : handle(0)
            , type(ComputeResourceType::INVALID)
            {
            }
        };

        FixedArray<ComputeResourceBinding, OMAF_MAX_COMPUTE_BINDINGS> mActiveComputeResources;
};

OMAF_NS_END
