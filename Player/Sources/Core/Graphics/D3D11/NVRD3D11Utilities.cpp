
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

#include "Graphics/D3D11/NVRD3D11Utilities.h"

#include "Graphics/D3D11/NVRD3D11Error.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    D3D11_FILL_MODE getFillModeD3D(FillMode::Enum fillMode)
    {
        switch(fillMode)
        {
            case FillMode::SOLID:       return D3D11_FILL_SOLID;
            case FillMode::WIREFRAME:   return D3D11_FILL_WIREFRAME;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_FILL_MODE)0;
    }

    D3D11_CULL_MODE getCullModeD3D(CullMode::Enum cullMode)
    {
        switch(cullMode)
        {
            case CullMode::NONE:   return D3D11_CULL_NONE;
            case CullMode::FRONT:  return D3D11_CULL_FRONT;
            case CullMode::BACK:   return D3D11_CULL_BACK;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_CULL_MODE)0;
    }

    D3D11_BLEND getBlendFunctionD3D(BlendFunction::Enum blendFunction)
    {
        switch(blendFunction)
        {
            case BlendFunction::ZERO:                   return D3D11_BLEND_ZERO;
            case BlendFunction::ONE:                    return D3D11_BLEND_ONE;
            case BlendFunction::SRC_COLOR:              return D3D11_BLEND_SRC_COLOR;
            case BlendFunction::ONE_MINUS_SRC_COLOR:    return D3D11_BLEND_INV_SRC_COLOR;
            case BlendFunction::DST_COLOR:              return D3D11_BLEND_DEST_COLOR;
            case BlendFunction::ONE_MINUS_DST_COLOR:    return D3D11_BLEND_DEST_COLOR;
            case BlendFunction::SRC_ALPHA:              return D3D11_BLEND_SRC_ALPHA;
            case BlendFunction::ONE_MINUS_SRC_ALPHA:    return D3D11_BLEND_INV_SRC_ALPHA;
            case BlendFunction::DST_ALPHA:              return D3D11_BLEND_DEST_ALPHA;
            case BlendFunction::ONE_MINUS_DST_ALPHA:    return D3D11_BLEND_INV_DEST_ALPHA;
            case BlendFunction::SRC_ALPHA_SATURATE:     return D3D11_BLEND_SRC_ALPHA_SAT;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_BLEND)0;
    }

    D3D11_BLEND_OP getBlendEquationD3D(BlendEquation::Enum blendEquation)
    {
        switch(blendEquation)
        {
            case BlendEquation::ADD:                return D3D11_BLEND_OP_ADD;
            case BlendEquation::SUBTRACT:           return D3D11_BLEND_OP_SUBTRACT;
            case BlendEquation::REVERSE_SUBTRACT:   return D3D11_BLEND_OP_REV_SUBTRACT;
            case BlendEquation::MINIMUM:            return D3D11_BLEND_OP_MIN;
            case BlendEquation::MAXIMUM:            return D3D11_BLEND_OP_MAX;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_BLEND_OP)0;
    }

    UINT8 getBlendWriteMaskD3D(ColorMask::Enum blendWriteMask)
    {
        UINT8 writeMask = 0;
        writeMask |= (blendWriteMask & ColorMask::RED) ? D3D11_COLOR_WRITE_ENABLE_RED : 0;
        writeMask |= (blendWriteMask & ColorMask::GREEN) ? D3D11_COLOR_WRITE_ENABLE_GREEN : 0;
        writeMask |= (blendWriteMask & ColorMask::BLUE) ? D3D11_COLOR_WRITE_ENABLE_BLUE : 0;
        writeMask |= (blendWriteMask & ColorMask::ALPHA) ? D3D11_COLOR_WRITE_ENABLE_ALPHA : 0;

        return writeMask;
    }

    D3D11_USAGE getBufferUsageD3D(BufferAccess::Enum access)
    {
        switch (access)
        {
            case BufferAccess::STATIC:  return D3D11_USAGE_IMMUTABLE;
            case BufferAccess::DYNAMIC: return D3D11_USAGE_DYNAMIC;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_USAGE)0;
    }

    D3D11_DEPTH_WRITE_MASK getDepthWriteMaskD3D(DepthMask::Enum depthWriteMask)
    {
        switch (depthWriteMask)
        {
            case DepthMask::ZERO:   return D3D11_DEPTH_WRITE_MASK_ZERO;
            case DepthMask::ALL:    return D3D11_DEPTH_WRITE_MASK_ALL;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_DEPTH_WRITE_MASK)0;
    }

    D3D11_COMPARISON_FUNC getDepthFunctionD3D(DepthFunction::Enum depthFunction)
    {
        switch(depthFunction)
        {
            case DepthFunction::NEVER:          return D3D11_COMPARISON_NEVER;
            case DepthFunction::ALWAYS:         return D3D11_COMPARISON_ALWAYS;
            case DepthFunction::EQUAL:          return D3D11_COMPARISON_EQUAL;
            case DepthFunction::NOT_EQUAL:      return D3D11_COMPARISON_NOT_EQUAL;
            case DepthFunction::LESS:           return D3D11_COMPARISON_LESS;
            case DepthFunction::LESS_EQUAL:     return D3D11_COMPARISON_LESS_EQUAL;
            case DepthFunction::GREATER:        return D3D11_COMPARISON_GREATER;
            case DepthFunction::GREATER_EQUAL:  return D3D11_COMPARISON_GREATER_EQUAL;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_COMPARISON_FUNC)0;
    }

    D3D11_STENCIL_OP getStencilOperationD3D(StencilOperation::Enum stencilOperation)
    {
        switch(stencilOperation)
        {
            case StencilOperation::KEEP:            return D3D11_STENCIL_OP_KEEP;
            case StencilOperation::ZERO:            return D3D11_STENCIL_OP_ZERO;
            case StencilOperation::REPLACE:         return D3D11_STENCIL_OP_REPLACE;
            case StencilOperation::INCREMENT:       return D3D11_STENCIL_OP_INCR_SAT;
            case StencilOperation::INCREMENT_WRAP:  return D3D11_STENCIL_OP_INCR;
            case StencilOperation::DECREMENT:       return D3D11_STENCIL_OP_DECR;
            case StencilOperation::DECREMENT_WRAP:  return D3D11_STENCIL_OP_DECR_SAT;
            case StencilOperation::INVERT:          return D3D11_STENCIL_OP_INVERT;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_STENCIL_OP)0;
    }

    D3D11_COMPARISON_FUNC getStencilFunctionD3D(StencilFunction::Enum stencilFunction)
    {
        switch(stencilFunction)
        {
            case StencilFunction::NEVER:        return D3D11_COMPARISON_NEVER;
            case StencilFunction::ALWAYS:           return D3D11_COMPARISON_ALWAYS;
            case StencilFunction::EQUAL:            return D3D11_COMPARISON_EQUAL;
            case StencilFunction::NOT_EQUAL:        return D3D11_COMPARISON_NOT_EQUAL;
            case StencilFunction::LESS:             return D3D11_COMPARISON_LESS;
            case StencilFunction::LESS_EQUAL:       return D3D11_COMPARISON_LESS_EQUAL;
            case StencilFunction::GREATER:          return D3D11_COMPARISON_GREATER;
            case StencilFunction::GREATER_EQUAL:    return D3D11_COMPARISON_GREATER_EQUAL;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_COMPARISON_FUNC)0;
    }

    D3D11_PRIMITIVE_TOPOLOGY getPrimitiveTypeD3D(PrimitiveType::Enum primitiveType)
    {
        switch(primitiveType)
        {
            case PrimitiveType::POINT_LIST:     return D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
            case PrimitiveType::LINE_LIST:      return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
            case PrimitiveType::LINE_STRIP:     return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
            case PrimitiveType::TRIANGLE_LIST:  return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
            case PrimitiveType::TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_PRIMITIVE_TOPOLOGY)0;
    }

    D3D11_TEXTURE_ADDRESS_MODE getTextureAddressModeD3D(TextureAddressMode::Enum textureAddressMode)
    {
        switch(textureAddressMode)
        {
            case TextureAddressMode::REPEAT:    return D3D11_TEXTURE_ADDRESS_WRAP;
            case TextureAddressMode::MIRROR:    return D3D11_TEXTURE_ADDRESS_MIRROR;
            case TextureAddressMode::CLAMP:     return D3D11_TEXTURE_ADDRESS_CLAMP;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_TEXTURE_ADDRESS_MODE)0;
    }

    D3D11_FILTER getTextureFilterModeD3D(TextureFilterMode::Enum textureFilterMode)
    {
        switch(textureFilterMode)
        {
            case TextureFilterMode::POINT:          return D3D11_FILTER_MIN_MAG_MIP_POINT;
            case TextureFilterMode::BILINEAR:       return D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            case TextureFilterMode::TRILINEAR:      return D3D11_FILTER_MIN_MAG_MIP_LINEAR;

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return (D3D11_FILTER)0;
    }

    TextureFormatInfo getTextureFormatInfoD3D(TextureFormat::Enum format)
    {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/bb173059(v=vs.85).aspx
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ff471325(v=vs.85).aspx

        static TextureFormatInfo textureFormats[TextureFormat::COUNT] =
        {
            /* BC1 */           { DXGI_FORMAT_BC1_UNORM,            DXGI_FORMAT_BC1_UNORM,              DXGI_FORMAT_UNKNOWN },
            /* BC1A1 */         { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* BC2 */           { DXGI_FORMAT_BC2_UNORM,            DXGI_FORMAT_BC2_UNORM,              DXGI_FORMAT_UNKNOWN },
            /* BC3 */           { DXGI_FORMAT_BC3_UNORM,            DXGI_FORMAT_BC3_UNORM,              DXGI_FORMAT_UNKNOWN },

            /* BC4 */           { DXGI_FORMAT_BC4_UNORM,            DXGI_FORMAT_BC4_UNORM,              DXGI_FORMAT_UNKNOWN },
            /* BC5 */           { DXGI_FORMAT_BC5_UNORM,            DXGI_FORMAT_BC5_UNORM,              DXGI_FORMAT_UNKNOWN },

            /* ETC1 */          { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* ETC2 */          { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* ETC2EAC */       { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* ETC2A1 */        { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* PVRTCIRGB2 */    { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* PVRTCIRGBA2 */   { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* PVRTCIRGB4 */    { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* PVRTCIRGBA4 */   { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* PVRTCIIRGBA2 */  { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* PVRTCIIRGBA4 */  { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* A8 */            { DXGI_FORMAT_A8_UNORM,             DXGI_FORMAT_A8_UNORM,               DXGI_FORMAT_UNKNOWN },

            /* R8 */            { DXGI_FORMAT_R8_UNORM,             DXGI_FORMAT_R8_UNORM,               DXGI_FORMAT_UNKNOWN },
            /* R8I */           { DXGI_FORMAT_R8_SINT,              DXGI_FORMAT_R8_SINT,                DXGI_FORMAT_UNKNOWN },
            /* R8U */           { DXGI_FORMAT_R8_UINT,              DXGI_FORMAT_R8_UINT,                DXGI_FORMAT_UNKNOWN },
            /* R8S */           { DXGI_FORMAT_R8_SNORM,             DXGI_FORMAT_R8_SNORM,               DXGI_FORMAT_UNKNOWN },

            /* R16 */           { DXGI_FORMAT_R16_UNORM,            DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_UNKNOWN },
            /* R16I */          { DXGI_FORMAT_R16_SINT,             DXGI_FORMAT_R16_SINT,               DXGI_FORMAT_UNKNOWN },
            /* R16U */          { DXGI_FORMAT_R16_UINT,             DXGI_FORMAT_R16_UINT,               DXGI_FORMAT_UNKNOWN },
            /* R16F */          { DXGI_FORMAT_R16_FLOAT,            DXGI_FORMAT_R16_FLOAT,              DXGI_FORMAT_UNKNOWN },
            /* R16S */          { DXGI_FORMAT_R16_SNORM,            DXGI_FORMAT_R16_SNORM,              DXGI_FORMAT_UNKNOWN },

            /* R32I */          { DXGI_FORMAT_R32_SINT,             DXGI_FORMAT_R32_SINT,               DXGI_FORMAT_UNKNOWN },
            /* R32U */          { DXGI_FORMAT_R32_UINT,             DXGI_FORMAT_R32_UINT,               DXGI_FORMAT_UNKNOWN },
            /* R32F */          { DXGI_FORMAT_R32_FLOAT,            DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_UNKNOWN },

            /* RG8 */           { DXGI_FORMAT_R8G8_UNORM,           DXGI_FORMAT_R8G8_UNORM,             DXGI_FORMAT_UNKNOWN },
            /* RG8I */          { DXGI_FORMAT_R8G8_SINT,            DXGI_FORMAT_R8G8_SINT,              DXGI_FORMAT_UNKNOWN },
            /* RG8U */          { DXGI_FORMAT_R8G8_UINT,            DXGI_FORMAT_R8G8_UINT,              DXGI_FORMAT_UNKNOWN },
            /* RG8S */          { DXGI_FORMAT_R8G8_SNORM,           DXGI_FORMAT_R8G8_SNORM,             DXGI_FORMAT_UNKNOWN },

            /* RG16 */          { DXGI_FORMAT_R16G16_UNORM,         DXGI_FORMAT_R16G16_UNORM,           DXGI_FORMAT_UNKNOWN },
            /* RG16I */         { DXGI_FORMAT_R16G16_SINT,          DXGI_FORMAT_R16G16_SINT,            DXGI_FORMAT_UNKNOWN },
            /* RG16U */         { DXGI_FORMAT_R16G16_UINT,          DXGI_FORMAT_R16G16_UINT,            DXGI_FORMAT_UNKNOWN },
            /* RG16F */         { DXGI_FORMAT_R16G16_FLOAT,         DXGI_FORMAT_R16G16_FLOAT,           DXGI_FORMAT_UNKNOWN },
            /* RG16S */         { DXGI_FORMAT_R16G16_SNORM,         DXGI_FORMAT_R16G16_SNORM,           DXGI_FORMAT_UNKNOWN },

            /* RG32I */         { DXGI_FORMAT_R32G32_SINT,          DXGI_FORMAT_R32G32_SINT,            DXGI_FORMAT_UNKNOWN },
            /* RG32U */         { DXGI_FORMAT_R32G32_UINT,          DXGI_FORMAT_R32G32_UINT,            DXGI_FORMAT_UNKNOWN },
            /* RG32F */         { DXGI_FORMAT_R32G32_FLOAT,         DXGI_FORMAT_R32G32_FLOAT,           DXGI_FORMAT_UNKNOWN },

            /* RGB8 */          { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* RGB9E5F */       { DXGI_FORMAT_R9G9B9E5_SHAREDEXP,   DXGI_FORMAT_R9G9B9E5_SHAREDEXP,     DXGI_FORMAT_UNKNOWN },

            /* BGRA8 */         { DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM,         DXGI_FORMAT_UNKNOWN },

            /* RGBA8 */         { DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,         DXGI_FORMAT_UNKNOWN },
            /* RGBA8I */        { DXGI_FORMAT_R8G8B8A8_SINT,        DXGI_FORMAT_R8G8B8A8_SINT,          DXGI_FORMAT_UNKNOWN },
            /* RGBA8U */        { DXGI_FORMAT_R8G8B8A8_UINT,        DXGI_FORMAT_R8G8B8A8_UINT,          DXGI_FORMAT_UNKNOWN },
            /* RGBA8S */        { DXGI_FORMAT_R8G8B8A8_SNORM,       DXGI_FORMAT_R8G8B8A8_SNORM,         DXGI_FORMAT_UNKNOWN },

            /* RGBA16 */        { DXGI_FORMAT_R16G16B16A16_UNORM,   DXGI_FORMAT_R16G16B16A16_UNORM,     DXGI_FORMAT_UNKNOWN },
            /* RGBA16I */       { DXGI_FORMAT_R16G16B16A16_SINT,    DXGI_FORMAT_R16G16B16A16_SINT,      DXGI_FORMAT_UNKNOWN },
            /* RGBA16U */       { DXGI_FORMAT_R16G16B16A16_UINT,    DXGI_FORMAT_R16G16B16A16_UINT,      DXGI_FORMAT_UNKNOWN },
            /* RGBA16F */       { DXGI_FORMAT_R16G16B16A16_FLOAT,   DXGI_FORMAT_R16G16B16A16_FLOAT,     DXGI_FORMAT_UNKNOWN },
            /* RGBA16S */       { DXGI_FORMAT_R16G16B16A16_SNORM,   DXGI_FORMAT_R16G16B16A16_SNORM,     DXGI_FORMAT_UNKNOWN },

            /* RGBA32I */       { DXGI_FORMAT_R32G32B32A32_SINT,    DXGI_FORMAT_R32G32B32A32_SINT,      DXGI_FORMAT_UNKNOWN },
            /* RGBA32U */       { DXGI_FORMAT_R32G32B32A32_UINT,    DXGI_FORMAT_R32G32B32A32_UINT,      DXGI_FORMAT_UNKNOWN },
            /* RGBA32F */       { DXGI_FORMAT_R32G32B32A32_FLOAT,   DXGI_FORMAT_R32G32B32A32_FLOAT,     DXGI_FORMAT_UNKNOWN },

            /* R5G6B5 */        { DXGI_FORMAT_B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           DXGI_FORMAT_UNKNOWN },
            /* RGBA4 */         { DXGI_FORMAT_B4G4R4A4_UNORM,       DXGI_FORMAT_B4G4R4A4_UNORM,         DXGI_FORMAT_UNKNOWN },
            /* RGB5A1 */        { DXGI_FORMAT_B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         DXGI_FORMAT_UNKNOWN },

            /* RGB10A2 */       { DXGI_FORMAT_R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_UNORM,      DXGI_FORMAT_UNKNOWN },
            /* R11G11B10F */    { DXGI_FORMAT_R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        DXGI_FORMAT_UNKNOWN },

            /* D16 */           { DXGI_FORMAT_R16_TYPELESS,         DXGI_FORMAT_R16_UNORM,              DXGI_FORMAT_D16_UNORM },
            /* D24 */           { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* D32 */           { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* D16F */          { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* D24F */          { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* D32F */          { DXGI_FORMAT_R32_TYPELESS,         DXGI_FORMAT_R32_FLOAT,              DXGI_FORMAT_D32_FLOAT },

            /* D24S8 */         { DXGI_FORMAT_R24G8_TYPELESS,       DXGI_FORMAT_R24_UNORM_X8_TYPELESS,      DXGI_FORMAT_D24_UNORM_S8_UINT },
            /* D32FS8X24 */      { DXGI_FORMAT_R32G8X24_TYPELESS,    DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS,   DXGI_FORMAT_D32_FLOAT_S8X24_UINT },

            /* S8 */            { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },
            /* S16 */           { DXGI_FORMAT_UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN },

            /* RGBA8_TYPELESS */{ DXGI_FORMAT_R8G8B8A8_UNORM,       DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_UNKNOWN },
            /* BGRA8_TYPELESS */{ DXGI_FORMAT_B8G8R8A8_UNORM,       DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_UNKNOWN },
        };

        OMAF_COMPILE_ASSERT(OMAF_ARRAY_SIZE(textureFormats) == TextureFormat::COUNT);

        return textureFormats[format];
    }

    TextureFormat::Enum getTextureFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
            case DXGI_FORMAT_BC1_UNORM:             return TextureFormat::BC1;
            case DXGI_FORMAT_BC2_UNORM:             return TextureFormat::BC2;
            case DXGI_FORMAT_BC3_UNORM:             return TextureFormat::BC3;

            case DXGI_FORMAT_BC4_UNORM:             return TextureFormat::BC4;
            case DXGI_FORMAT_BC5_UNORM:             return TextureFormat::BC5;

            case DXGI_FORMAT_A8_UNORM:              return TextureFormat::A8;

            case DXGI_FORMAT_R8_UNORM:              return TextureFormat::R8;
            case DXGI_FORMAT_R8_SINT:               return TextureFormat::R8I;
            case DXGI_FORMAT_R8_UINT:               return TextureFormat::R8U;
            case DXGI_FORMAT_R8_SNORM:              return TextureFormat::R8S;

            case DXGI_FORMAT_R16_UNORM:             return TextureFormat::R16;
            case DXGI_FORMAT_R16_SINT:              return TextureFormat::R16I;
            case DXGI_FORMAT_R16_UINT:              return TextureFormat::R16U;
            case DXGI_FORMAT_R16_FLOAT:             return TextureFormat::R16F;
            case DXGI_FORMAT_R16_SNORM:             return TextureFormat::R16S;

            case DXGI_FORMAT_R32_SINT:              return TextureFormat::R32I;
            case DXGI_FORMAT_R32_UINT:              return TextureFormat::R32U;
            case DXGI_FORMAT_R32_FLOAT:             return TextureFormat::R32F;

            case DXGI_FORMAT_R8G8_UNORM:            return TextureFormat::RG8;
            case DXGI_FORMAT_R8G8_SINT:             return TextureFormat::RG8I;
            case DXGI_FORMAT_R8G8_UINT:             return TextureFormat::RG8U;
            case DXGI_FORMAT_R8G8_SNORM:            return TextureFormat::RG8S;

            case DXGI_FORMAT_R16G16_UNORM:          return TextureFormat::RG16;
            case DXGI_FORMAT_R16G16_SINT:           return TextureFormat::RG16I;
            case DXGI_FORMAT_R16G16_UINT:           return TextureFormat::RG16U;
            case DXGI_FORMAT_R16G16_FLOAT:          return TextureFormat::RG16F;
            case DXGI_FORMAT_R16G16_SNORM:          return TextureFormat::RG16S;

            case DXGI_FORMAT_R32G32_SINT:           return TextureFormat::RG32I;
            case DXGI_FORMAT_R32G32_UINT:           return TextureFormat::RG32U;
            case DXGI_FORMAT_R32G32_FLOAT:          return TextureFormat::RG32F;

            case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:    return TextureFormat::RGB9E5F;

            case DXGI_FORMAT_B8G8R8A8_UNORM:        return TextureFormat::BGRA8;

            case DXGI_FORMAT_R8G8B8A8_UNORM:        return TextureFormat::RGBA8;
            case DXGI_FORMAT_R8G8B8A8_SINT:         return TextureFormat::RGBA8I;
            case DXGI_FORMAT_R8G8B8A8_UINT:         return TextureFormat::RGBA8U;
            case DXGI_FORMAT_R8G8B8A8_SNORM:        return TextureFormat::RGBA8S;

            case DXGI_FORMAT_R16G16B16A16_UNORM:    return TextureFormat::RGBA16;
            case DXGI_FORMAT_R16G16B16A16_SINT:     return TextureFormat::RGBA16I;
            case DXGI_FORMAT_R16G16B16A16_UINT:     return TextureFormat::RGBA16U;
            case DXGI_FORMAT_R16G16B16A16_FLOAT:    return TextureFormat::RGBA16F;
            case DXGI_FORMAT_R16G16B16A16_SNORM:    return TextureFormat::RGBA16S;

            case DXGI_FORMAT_R32G32B32A32_SINT:     return TextureFormat::RGBA32I;
            case DXGI_FORMAT_R32G32B32A32_UINT:     return TextureFormat::RGBA32U;
            case DXGI_FORMAT_R32G32B32A32_FLOAT:    return TextureFormat::RGBA32F;

            case DXGI_FORMAT_B5G6R5_UNORM:          return TextureFormat::R5G6B5;
            case DXGI_FORMAT_B4G4R4A4_UNORM:        return TextureFormat::RGBA4;
            case DXGI_FORMAT_B5G5R5A1_UNORM:        return TextureFormat::RGB5A1;

            case DXGI_FORMAT_R10G10B10A2_UNORM:     return TextureFormat::RGB10A2;
            case DXGI_FORMAT_R11G11B10_FLOAT:       return TextureFormat::R11G11B10F;

            case DXGI_FORMAT_R16_TYPELESS:          return TextureFormat::D16;

            case DXGI_FORMAT_R32_TYPELESS:          return TextureFormat::D32F;

            case DXGI_FORMAT_D24_UNORM_S8_UINT:     return TextureFormat::D24S8;

            case DXGI_FORMAT_R24G8_TYPELESS:        return TextureFormat::D24S8;
            case DXGI_FORMAT_R32G8X24_TYPELESS:     return TextureFormat::D32FS8X24;

            case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return TextureFormat::RGBA8_TYPELESS;
            case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return TextureFormat::BGRA8_TYPELESS;
            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        };

        return TextureFormat::INVALID;
    }

    bool_t supportsTextureFormatD3D(TextureFormat::Enum format)
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();

        return false;
    }

    bool_t supportsRenderTargetTextureFormatD3D(TextureFormat::Enum format)
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();

        return false;
    }
OMAF_NS_END
