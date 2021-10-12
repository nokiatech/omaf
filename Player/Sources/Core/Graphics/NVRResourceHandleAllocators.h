
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

#include "Foundation/NVRHandleAllocator.h"
#include "Graphics/NVRConfig.h"

OMAF_NS_BEGIN

#define OMAF_NUM_HANDLE_INDEX_BITS 16
#define OMAF_NUM_HANDLE_GENERATION_BITS 16

typedef HandleAllocator<uint32_t, OMAF_NUM_HANDLE_INDEX_BITS, OMAF_NUM_HANDLE_GENERATION_BITS, OMAF_MAX_VERTEX_BUFFERS>
    VertexBufferHandleAllocator;
typedef HandleAllocator<uint32_t, OMAF_NUM_HANDLE_INDEX_BITS, OMAF_NUM_HANDLE_GENERATION_BITS, OMAF_MAX_INDEX_BUFFERS>
    IndexBufferHandleAllocator;
typedef HandleAllocator<uint32_t, OMAF_NUM_HANDLE_INDEX_BITS, OMAF_NUM_HANDLE_GENERATION_BITS, OMAF_MAX_SHADERS>
    ShaderHandleAllocator;
typedef HandleAllocator<uint32_t,
                        OMAF_NUM_HANDLE_INDEX_BITS,
                        OMAF_NUM_HANDLE_GENERATION_BITS,
                        OMAF_MAX_SHADER_CONSTANTS>
    ShaderConstantHandleAllocator;
typedef HandleAllocator<uint32_t, OMAF_NUM_HANDLE_INDEX_BITS, OMAF_NUM_HANDLE_GENERATION_BITS, OMAF_MAX_TEXTURES>
    TextureHandleAllocator;
typedef HandleAllocator<uint32_t, OMAF_NUM_HANDLE_INDEX_BITS, OMAF_NUM_HANDLE_GENERATION_BITS, OMAF_MAX_RENDER_TARGETS>
    RenderTargetHandleAllocator;

typedef VertexBufferHandleAllocator::HandleType VertexBufferHandle;
typedef IndexBufferHandleAllocator::HandleType IndexBufferHandle;
typedef TextureHandleAllocator::HandleType TextureHandle;
typedef ShaderHandleAllocator::HandleType ShaderHandle;
typedef RenderTargetHandleAllocator::HandleType RenderTargetHandle;
typedef ShaderConstantHandleAllocator::HandleType ShaderConstantHandle;

OMAF_NS_END
