
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

#include "Graphics/NVRResourceHandleAllocators.h"

OMAF_NS_BEGIN

#define OMAF_DECLARE_HANDLE(name, type)                      \
struct name                                                 \
{                                                           \
    typedef type InternalType;                              \
                                                            \
    InternalType _handle;                                   \
    static const name Invalid;                              \
                                                            \
    name()                                                  \
    : _handle(InternalType::Invalid)                        \
    {                                                       \
    }                                                       \
                                                            \
    name(const InternalType& handle)                        \
    : _handle(handle)                                       \
    {                                                       \
    }                                                       \
                                                            \
    name(const name& handle)                                \
    : _handle(handle._handle)                               \
    {                                                       \
    }                                                       \
                                                            \
    bool_t isValid() const                                  \
    {                                                       \
        return _handle != InternalType::Invalid;            \
    }                                                       \
                                                            \
    OMAF_INLINE bool_t operator == (const name& other) const \
    {                                                       \
        return _handle == other._handle;                    \
    }                                                       \
                                                            \
    OMAF_INLINE bool_t operator != (const name& other) const \
    {                                                       \
        return _handle != other._handle;                    \
    }                                                       \
}

OMAF_DECLARE_HANDLE(VertexBufferID, VertexBufferHandleAllocator::HandleType);
OMAF_DECLARE_HANDLE(IndexBufferID, IndexBufferHandleAllocator::HandleType);
OMAF_DECLARE_HANDLE(ShaderID, ShaderHandleAllocator::HandleType);
OMAF_DECLARE_HANDLE(ShaderConstantID, ShaderConstantHandleAllocator::HandleType);
OMAF_DECLARE_HANDLE(TextureID, TextureHandleAllocator::HandleType);
OMAF_DECLARE_HANDLE(RenderTargetID, RenderTargetHandleAllocator::HandleType);

OMAF_NS_END
