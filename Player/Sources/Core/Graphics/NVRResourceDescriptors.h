
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "Graphics/NVRHandles.h"
#include "Graphics/NVRBufferAccess.h"
#include "Graphics/NVRComputeBufferAccess.h"
#include "Graphics/NVRIndexBufferFormat.h"
#include "Graphics/NVRVertexDeclaration.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRSamplerState.h"
#include "Graphics/NVRDiscardMask.h"

OMAF_NS_BEGIN

struct VertexBufferDesc
{
    VertexBufferDesc()
    : access(BufferAccess::INVALID)
    , computeAccess(ComputeBufferAccess::NONE)
    , data(OMAF_NULL)
    , size(0)
    {
    }
        
    VertexDeclaration declaration;
    BufferAccess::Enum access;
    ComputeBufferAccess::Enum computeAccess;
    const void_t* data;
    size_t size;
};
    
struct IndexBufferDesc
{
    IndexBufferDesc()
    : access(BufferAccess::INVALID)
    , computeAccess(ComputeBufferAccess::NONE)
    , format(IndexBufferFormat::INVALID)
    , data(OMAF_NULL)
    , size(0)
    {
    }
    
    BufferAccess::Enum access;
    ComputeBufferAccess::Enum computeAccess;
    IndexBufferFormat::Enum format;
    const void_t* data;
    size_t size;
};

struct TextureDesc
{
    TextureDesc()
    : type(TextureType::INVALID)
    , computeAccess(ComputeBufferAccess::NONE)
    , width(0)
    , height(0)
    , numMipMaps(0)
    , generateMipMaps(false)
    , format(TextureFormat::INVALID)
    , renderTarget(false)
    , writeOnly(false)
    , data(OMAF_NULL)
    , size(0)
    {
        samplerState.addressModeU = TextureAddressMode::INVALID;
        samplerState.addressModeV = TextureAddressMode::INVALID;
        samplerState.addressModeW = TextureAddressMode::INVALID;
        samplerState.filterMode = TextureFilterMode::INVALID;
    }
    
    TextureType::Enum type;
    ComputeBufferAccess::Enum computeAccess;
    uint16_t width;
    uint16_t height;
    uint8_t numMipMaps;
    bool_t generateMipMaps;
    TextureFormat::Enum format;
    SamplerState samplerState;
    bool_t renderTarget;
    bool_t writeOnly;
    const void_t* data;
    size_t size;
};
    
// Only used to register native graphics API textures resources to render backend
struct NativeTextureDesc
{
    NativeTextureDesc()
    : type(TextureType::INVALID)
    , width(0)
    , height(0)
    , numMipMaps(0)
    , arrayIndex(0)
    , format(TextureFormat::INVALID)
    , nativeHandle(OMAF_NULL)
    {
    }
        
    TextureType::Enum type;
    uint16_t width;
    uint16_t height;
    uint8_t numMipMaps;
    uint8_t arrayIndex;
    TextureFormat::Enum format;
    const void_t* nativeHandle;
};
    
struct RenderTargetDesc
{
    RenderTargetDesc()
    : colorBufferFormat(TextureFormat::INVALID)
    , depthStencilBufferFormat(TextureFormat::INVALID)
    , width(0)
    , height(0)
    , numMultisamples(0)
    , discardMask(DiscardMask::NONE)
    {
    }
        
    TextureFormat::Enum colorBufferFormat;
    TextureFormat::Enum depthStencilBufferFormat;
    uint16_t width;
    uint16_t height;
    uint8_t numMultisamples;
    int16_t discardMask;
};
    
struct RenderTargetTextureDesc
{
    RenderTargetTextureDesc()
    : attachments(OMAF_NULL)
    , numAttachments(0)
    , destroyAttachments(false)
    , discardMask(DiscardMask::NONE)
    {
    }
        
    const TextureID* attachments;
    uint8_t numAttachments;
    bool_t destroyAttachments;
    int16_t discardMask;
};

struct MemoryBuffer
{
    uint8_t* data;
    uint32_t size;
    uint32_t offset;
    uint32_t capacity;

    MemoryBuffer()
    : data(OMAF_NULL)
    , size(0)
    , offset(0)
    , capacity(0)
    {
    }
};

OMAF_NS_END
