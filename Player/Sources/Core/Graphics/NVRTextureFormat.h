
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

#include "Graphics/NVRColorSpace.h"

OMAF_NS_BEGIN

namespace TextureFormat
{
    enum Enum
    {
        INVALID = -1,
        
        // Compressed formats
        BC1, // DXT1, RGB
        BC1A1, // DXT1, RGBA
        BC2, // DXT3, RGBA
        BC3, // DXT5, RGBA
        
        BC4, // LATC1 / ATI1 / RGTC1 / ATI_3DC_X
        BC5, // LATC2 / ATI2 / RGTC2 / ATI_3DC_XY
        
        ETC1, // ETC1, RGB 888
        
        ETC2, // ETC2, RGB 888
        ETC2EAC, // ETC2, RGBA 8888
        ETC2A1, // ETC2, RGBA 8881
        
        PVRTCIRGB2, // PVRTC 1, RGB 2BPP
        PVRTCIRGBA2, // PVRTC 1, RGBA 2BPP
        PVRTCIRGB4, // PVRTC 1, RGB 4BPP
        PVRTCIRGBA4, // PVRTC 1, RGBA 4BPP
        
        PVRTCIIRGBA2, // PVRTC 2, RGBA 2BPP
        PVRTCIIRGBA4, // PVRTC 2, RGBA 4BPP
        
        // Color formats
        A8,
        
        R8,
        R8I,
        R8U,
        R8S,
        
        R16,
        R16I,
        R16U,
        R16F,
        R16S,
        
        R32I,
        R32U,
        R32F,
        
        RG8,
        RG8I,
        RG8U,
        RG8S,
        
        RG16,
        RG16I,
        RG16U,
        RG16F,
        RG16S,
        
        RG32I,
        RG32U,
        RG32F,
        
        RGB8,
        RGB9E5F,
        
        BGRA8,
        
        RGBA8,
        RGBA8I,
        RGBA8U,
        RGBA8S,
        
        RGBA16,
        RGBA16I,
        RGBA16U,
        RGBA16F,
        RGBA16S,
        
        RGBA32I,
        RGBA32U,
        RGBA32F,
        
        R5G6B5,
        RGBA4,
        RGB5A1,
        
        RGB10A2,
        R11G11B10F,
        
        // Depth stencil formats
        D16,
        D24,
        D32,
        
        D16F,
        D24F,
        D32F,
        
        D24S8,
        D32FS8X24,
        
        S8,
        S16,
        
        RGBA8_TYPELESS,
        BGRA8_TYPELESS,
        COUNT
    };
    
    const char_t* toString(Enum value);
}

namespace TextureFormat
{
    struct Description
    {
        TextureFormat::Enum format;
        uint8_t numChannels;
        
        bool_t compressed;
        bool_t hasAlphaComponent;
        
        ColorSpace::Enum colorSpace;
        
        uint8_t alphaBits;
        uint8_t redBits;
        uint8_t greenBits;
        uint8_t blueBits;
        
        uint8_t depthBits;
        uint8_t stencilBits;
        
        uint8_t bitsPerPixel;
    };
    
    struct CompressionDescription
    {
        TextureFormat::Enum format;
        
        uint8_t bitsPerPixel;
        
        uint8_t blockWidth;
        uint8_t blockHeight;
        
        uint8_t blockSize;
        
        uint8_t minBlockX;
        uint8_t minBlockY;
    };
    
    const Description& getDescription(TextureFormat::Enum textureFormat);
    const CompressionDescription& getCompressionDescription(TextureFormat::Enum textureFormat);
    
    size_t calculateImageSize(const CompressionDescription& description, uint16_t& width, uint16_t& height);
    
    bool_t hasAlphaComponent(TextureFormat::Enum textureFormat);
    bool_t isCompressed(TextureFormat::Enum textureFormat);
    
    bool_t hasDepthComponent(TextureFormat::Enum textureFormat);
    bool_t hasStencilComponent(TextureFormat::Enum textureFormat);
}

OMAF_NS_END
