
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
#include "Graphics/NVRTextureFormat.h"
#include "Math/OMAFMathFunctions.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

namespace TextureFormat
{
    const char_t* toString(Enum value)
    {
        OMAF_ASSERT(value < COUNT, "");
        
        struct Name
        {
            const char_t* name;
        };
        
        static Name names[] =
        {
            { "BC1" },
            { "BC1A1" },
            { "BC2" },
            { "BC3" },
            
            { "BC4" },
            { "BC5" },
            
            { "ETC1" },
            
            { "ETC2" },
            { "ETC2EAC" },
            { "ETC2A1" },
            
            { "PVRTCIRGB2" },
            { "PVRTCIRGBA2" },
            { "PVRTCIRGB4" },
            { "PVRTCIRGBA4" },
            
            { "PVRTCIIRGBA2" },
            { "PVRTCIIRGBA4" },
            
            { "A8" },
            
            { "R8" },
            { "R8I" },
            { "R8U" },
            { "R8S" },
            
            { "R16" },
            { "R16I" },
            { "R16U" },
            { "R16F" },
            { "R16S" },
            
            { "R32I" },
            { "R32U" },
            { "R32F" },
            
            { "RG8" },
            { "RG8I" },
            { "RG8U" },
            { "RG8S" },
            
            { "RG16" },
            { "RG16I" },
            { "RG16U" },
            { "RG16F" },
            { "RG16S" },
            
            { "RG32I" },
            { "RG32U" },
            { "RG32F" },
            
            { "RGB8" },
            { "RGB9E5F" },
            
            { "BGRA8" },
            
            { "RGBA8" },
            { "RGBA8I" },
            { "RGBA8U" },
            { "RGBA8S" },
            
            { "RGBA16" },
            { "RGBA16I" },
            { "RGBA16U" },
            { "RGBA16F" },
            { "RGBA16S" },
            
            { "RGBA32I" },
            { "RGBA32U" },
            { "RGBA32F" },
            
            { "R5G6B5" },
            { "RGBA4" },
            { "RGB5A1" },
            
            { "RGB10A2" },
            { "R11G11B10F" },
            
            { "D16" },
            { "D24" },
            { "D32" },
            
            { "D16F" },
            { "D24F" },
            { "D32F" },
            
            { "D24S8" },
            { "D32FS8X24" },
            
            { "S8" },
            { "S16" },
            
            { "RGBA8_TYPELESS" },
            { "BGRA8_TYPELESS" }
        };
        
        if (value == TextureFormat::INVALID)
        {
            return "INVALID";
        }

        OMAF_COMPILE_ASSERT(OMAF_ARRAY_SIZE(names) == TextureFormat::COUNT);
        
        return names[value].name;
    }
    
    size_t calculateImageSize(const CompressionDescription& description, uint16_t& width, uint16_t& height)
    {
        const uint8_t bitsPerPixel = description.bitsPerPixel;
        
        const uint16_t blockWidth = description.blockWidth;
        const uint16_t blockHeight = description.blockHeight;
        
        const uint16_t minBlockX = description.minBlockX;
        const uint16_t minBlockY = description.minBlockY;
        
        width = max(blockWidth * minBlockX, ((width + blockWidth - 1) / blockWidth) * blockWidth);
        height = min(blockHeight * minBlockY, ((height + blockHeight - 1) / blockHeight) * blockHeight);
        
        size_t size = ((width * height) * bitsPerPixel) / 8;
        
        return size;
    }
    
    const CompressionDescription& getCompressionDescription(TextureFormat::Enum textureFormat)
    {
        OMAF_ASSERT(textureFormat != TextureFormat::INVALID, "");
        OMAF_ASSERT(textureFormat < TextureFormat::COUNT, "");
        OMAF_ASSERT(TextureFormat::isCompressed(textureFormat), "");

        static CompressionDescription descriptions[] =
        {
            /* BC1 */           { BC1, 4, 4, 4,  8, 1, 1 },
            /* BC1A1 */         { BC1A1, 4, 4, 4,  8, 1, 1 },
            /* BC2 */           { BC2, 8, 4, 4, 16, 1, 1 },
            /* BC3 */           { BC3, 8, 4, 4, 16, 1, 1 },
            
            /* BC4 */           { BC4, 4, 4, 4,  8, 1, 1 },
            /* BC5 */           { BC5, 8, 4, 4, 16, 1, 1 },
            
            /* ETC1 */          { ETC1, 4, 4, 4,  8, 1, 1 },
            
            /* ETC2 */          { ETC2, 4, 4, 4,  8, 1, 1 },
            
            /* ETC2EAC */       { ETC2EAC, 8, 4, 4, 16, 1, 1 },
            /* ETC2A1 */        { ETC2A1, 4, 4, 4,  8, 1, 1 },
            
            /* PVRTCIRGB2 */    { PVRTCIRGB2, 2, 8, 4,  8, 2, 2 },
            /* PVRTCIRGBA2 */   { PVRTCIRGBA2, 2, 8, 4,  8, 2, 2 },
            
            /* PVRTCIRGB4 */    { PVRTCIRGB4, 4, 4, 4,  8, 2, 2 },
            /* PVRTCIRGBA4 */   { PVRTCIRGBA4, 4, 4, 4,  8, 2, 2 },
            
            /* PVRTCIIRGBA2 */  { PVRTCIIRGBA2, 2, 8, 4,  8, 2, 2 },
            /* PVRTCIIRGBA4 */  { PVRTCIIRGBA4, 4, 4, 4,  8, 2, 2 },
        };
        
        return descriptions[textureFormat];
    }
    
    const Description& getDescription(TextureFormat::Enum textureFormat)
    {
        OMAF_ASSERT(textureFormat != TextureFormat::INVALID, "");
        OMAF_ASSERT(textureFormat < TextureFormat::COUNT, "");
        
        static Description descriptions[] =
        {
            { BC1,          3,  true,   false,  ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 24 },
            { BC1A1,        4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            { BC2,          4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            { BC3,          4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            
            { BC4,          1,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 8 },
            { BC5,          2,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 16 },
            
            { ETC1,         3,  true,   false,  ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 24 },
            
            { ETC2,         3,  true,   false,  ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 24 },
            { ETC2EAC,      4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            { ETC2A1,       4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            
            { PVRTCIRGB2,   3,  true,   false,  ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 24 },
            { PVRTCIRGBA2,  4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            { PVRTCIRGB4,   3,  true,   false,  ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 24 },
            { PVRTCIRGBA4,  4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            
            { PVRTCIIRGBA2, 4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            { PVRTCIIRGBA4, 4,  true,   true,   ColorSpace::RGB,    0, 0, 0, 0, 0, 0, 32 },
            
            { A8,           1,  false,  false,  ColorSpace::NONE,   8, 0, 0, 0, 0, 0, 8 },
            
            { R8,           1,  false,  false,  ColorSpace::RGB,   0, 8, 0, 0, 0, 0, 8 },
            { R8I,          1,  false,  false,  ColorSpace::RGB,   0, 8, 0, 0, 0, 0, 8 },
            { R8U,          1,  false,  false,  ColorSpace::RGB,   0, 8, 0, 0, 0, 0, 8 },
            { R8S,          1,  false,  false,  ColorSpace::RGB,   0, 8, 0, 0, 0, 0, 8 },
            
            { R16,          1,  false,  false,  ColorSpace::RGB,   0, 16, 0, 0, 0, 0, 16 },
            { R16I,         1,  false,  false,  ColorSpace::RGB,   0, 16, 0, 0, 0, 0, 16 },
            { R16U,         1,  false,  false,  ColorSpace::RGB,   0, 16, 0, 0, 0, 0, 16 },
            { R16F,         1,  false,  false,  ColorSpace::RGB,   0, 16, 0, 0, 0, 0, 16 },
            { R16S,         1,  false,  false,  ColorSpace::RGB,   0, 16, 0, 0, 0, 0, 16 },
            
            { R32I,         1,  false,  false,  ColorSpace::RGB,   0, 32, 0, 0, 0, 0, 32 },
            { R32U,         1,  false,  false,  ColorSpace::RGB,   0, 32, 0, 0, 0, 0, 32 },
            { R32F,         1,  false,  false,  ColorSpace::RGB,   0, 32, 0, 0, 0, 0, 32 },
            
            { RG8,          2,  false,  false,  ColorSpace::RGB,   0, 8, 8, 0, 0, 0, 16 },
            { RG8I,         2,  false,  false,  ColorSpace::RGB,   0, 8, 8, 0, 0, 0, 16 },
            { RG8U,         2,  false,  false,  ColorSpace::RGB,   0, 8, 8, 0, 0, 0, 16 },
            { RG8S,         2,  false,  false,  ColorSpace::RGB,   0, 8, 8, 0, 0, 0, 16 },
            
            { RG16,         2,  false,  false,  ColorSpace::RGB,   0, 16, 16, 0, 0, 0, 32 },
            { RG16I,        2,  false,  false,  ColorSpace::RGB,   0, 16, 16, 0, 0, 0, 32 },
            { RG16U,        2,  false,  false,  ColorSpace::RGB,   0, 16, 16, 0, 0, 0, 32 },
            { RG16F,        2,  false,  false,  ColorSpace::RGB,   0, 16, 16, 0, 0, 0, 32 },
            { RG16S,        2,  false,  false,  ColorSpace::RGB,   0, 16, 16, 0, 0, 0, 32 },
            
            { RG32I,        2,  false,  false,  ColorSpace::RGB,   0, 32, 32, 0, 0, 0, 64 },
            { RG32U,        2,  false,  false,  ColorSpace::RGB,   0, 32, 32, 0, 0, 0, 64 },
            { RG32F,        2,  false,  false,  ColorSpace::RGB,   0, 32, 32, 0, 0, 0, 64 },
            
            { RGB8,         3,  false,  false,  ColorSpace::RGB,   0, 8, 0, 0, 0, 0, 24 },
            { RGB9E5F,      3,  false,  false,  ColorSpace::RGB,   0, 14, 14, 14, 0, 0, 32 },
            
            { BGRA8,        4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            
            { RGBA8,        4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            { RGBA8I,       4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            { RGBA8U,       4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            { RGBA8S,       4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            
            { RGBA16,       4,  false,  false,  ColorSpace::RGB,   16, 16, 16, 16, 0, 0, 64 },
            { RGBA16I,      4,  false,  false,  ColorSpace::RGB,   16, 16, 16, 16, 0, 0, 64 },
            { RGBA16U,      4,  false,  false,  ColorSpace::RGB,   16, 16, 16, 16, 0, 0, 64 },
            { RGBA16F,      4,  false,  false,  ColorSpace::RGB,   16, 16, 16, 16, 0, 0, 64 },
            { RGBA16S,      4,  false,  false,  ColorSpace::RGB,   16, 16, 16, 16, 0, 0, 64 },
            
            { RGBA32I,      4,  false,  false,  ColorSpace::RGB,   32, 32, 32, 32, 0, 0, 128 },
            { RGBA32U,      4,  false,  false,  ColorSpace::RGB,   32, 32, 32, 32, 0, 0, 128 },
            { RGBA32F,      4,  false,  false,  ColorSpace::RGB,   32, 32, 32, 32, 0, 0, 128 },
            
            { R5G6B5,       3,  false,  false,  ColorSpace::RGB,   0, 5, 6, 5, 0, 0, 16 },
            { RGBA4,        3,  false,  false,  ColorSpace::RGB,   0, 4, 4, 4, 0, 0, 16 },
            { RGB5A1,       4,  false,  false,  ColorSpace::RGB,   1, 5, 5, 5, 0, 0, 16 },
            
            { RGB10A2,      4,  false,  false,  ColorSpace::RGB,   2, 10, 10, 10, 0, 0, 32 },
            { R11G11B10F,   3,  false,  false,  ColorSpace::RGB,   0, 11, 11, 10, 0, 0, 32 },

            { D16,          1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 16, 0, 16 },
            { D24,          1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 24, 0, 24 },
            { D32,          1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 32, 0, 32 },
            
            { D16F,         1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 16, 0, 16 },
            { D24F,         1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 24, 0, 24 },
            { D32F,         1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 32, 0, 32 },
            
            { D24S8,        2,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 24, 8, 32 },
            { D32FS8X24,    2,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 32, 8, 64 },
            
            { S8,           1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 0, 8, 8 },
            { S16,          1,  false,  false,  ColorSpace::NONE,   0, 0, 0, 0, 0, 16, 16 },

            { RGBA8_TYPELESS,4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
            { BGRA8_TYPELESS,4,  false,  false,  ColorSpace::RGB,   8, 8, 8, 8, 0, 0, 32 },
        };
        
        OMAF_COMPILE_ASSERT(OMAF_ARRAY_SIZE(descriptions) == TextureFormat::COUNT);
        
        return descriptions[textureFormat];
    }

    bool_t hasAlphaComponent(TextureFormat::Enum textureFormat)
    {
        const Description& description = getDescription(textureFormat);
        
        return description.hasAlphaComponent;
    }

    bool_t isCompressed(TextureFormat::Enum textureFormat)
    {
        const Description& description = getDescription(textureFormat);
        
        return description.compressed;
    }
    
    bool_t hasDepthComponent(TextureFormat::Enum textureFormat)
    {
        const Description& description = getDescription(textureFormat);
        
        return (description.depthBits > 0);
    }
    
    bool_t hasStencilComponent(TextureFormat::Enum textureFormat)
    {
        const Description& description = getDescription(textureFormat);
        
        return (description.stencilBits > 0);
    }
}

OMAF_NS_END
