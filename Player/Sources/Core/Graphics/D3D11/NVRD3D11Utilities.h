
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
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRDepthStencilState.h"
#include "Graphics/NVRPrimitiveType.h"
#include "Graphics/NVRRasterizerState.h"
#include "Graphics/NVRScissors.h"
#include "Graphics/NVRShaderConstantType.h"
#include "Graphics/NVRTextureAddressMode.h"
#include "Graphics/NVRTextureFilterMode.h"
#include "Graphics/NVRTextureFormat.h"
#include "Graphics/NVRTextureType.h"
#include "Graphics/NVRVertexAttributeFormat.h"
#include "Graphics/NVRViewport.h"

#include "Graphics/D3D11/NVRD3D11Error.h"

OMAF_NS_BEGIN
#define OMAF_DX_RELEASE(ptr, expectedRefCount)                     \
    {                                                              \
        do                                                         \
        {                                                          \
            if (ptr != OMAF_NULL)                                  \
            {                                                      \
                ULONG refCount = ptr->Release();                   \
                /*OMAF_ASSERT(expectedRefCount == refCount, "");*/ \
                ptr = OMAF_NULL;                                   \
            }                                                      \
        } while (false);                                           \
    }

// Convert types to D3D types
D3D11_FILL_MODE getFillModeD3D(FillMode::Enum fillMode);
D3D11_CULL_MODE getCullModeD3D(CullMode::Enum cullMode);

D3D11_BLEND getBlendFunctionD3D(BlendFunction::Enum blendFunction);
D3D11_BLEND_OP getBlendEquationD3D(BlendEquation::Enum blendEquation);
UINT8 getBlendWriteMaskD3D(ColorMask::Enum blendWriteMask);

D3D11_USAGE getBufferUsageD3D(BufferAccess::Enum access);

D3D11_DEPTH_WRITE_MASK getDepthWriteMaskD3D(DepthMask::Enum depthWriteMask);
D3D11_COMPARISON_FUNC getDepthFunctionD3D(DepthFunction::Enum depthFunction);

D3D11_STENCIL_OP getStencilOperationD3D(StencilOperation::Enum stencilOperation);
D3D11_COMPARISON_FUNC getStencilFunctionD3D(StencilFunction::Enum stencilFunction);

D3D11_PRIMITIVE_TOPOLOGY getPrimitiveTypeD3D(PrimitiveType::Enum primitiveType);

D3D11_TEXTURE_ADDRESS_MODE getTextureAddressModeD3D(TextureAddressMode::Enum textureAddressMode);
D3D11_FILTER getTextureFilterModeD3D(TextureFilterMode::Enum textureFilterMode);

// Helpers
struct TextureFormatInfo
{
    DXGI_FORMAT textureFormat;
    DXGI_FORMAT srvFormat;
    DXGI_FORMAT dsvFormat;
};

TextureFormatInfo getTextureFormatInfoD3D(TextureFormat::Enum format);
TextureFormat::Enum getTextureFormat(DXGI_FORMAT format);

bool_t supportsTextureFormat(TextureFormat::Enum format);
bool_t supportsRenderTargetTextureFormat(TextureFormat::Enum format);
OMAF_NS_END
