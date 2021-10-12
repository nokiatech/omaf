
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
#include "Graphics/D3D11/NVRVertexBufferD3D11.h"

#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(VertexBufferD3D11)

VertexBufferD3D11::VertexBufferD3D11()
    : mHandle(OMAF_NULL)
    , mBytes(0)
    , mBufferAccess(BufferAccess::INVALID)
{
}

VertexBufferD3D11::~VertexBufferD3D11()
{
    OMAF_ASSERT(mHandle == OMAF_NULL, "Vertex buffer is not destroyed");
}

const ID3D11Buffer* VertexBufferD3D11::getHandle() const
{
    return mHandle;
}

bool_t VertexBufferD3D11::create(ID3D11Device* device,
                                 const VertexDeclaration& declaration,
                                 BufferAccess::Enum access,
                                 const void_t* data,
                                 size_t bytes)
{
    OMAF_ASSERT_NOT_NULL(device);

    OMAF_ASSERT(!(data != NULL && bytes == 0), "");

    mBytes = bytes;
    mBufferAccess = access;

    MemoryCopy(&mVertexDeclaration, &declaration, OMAF_SIZE_OF(declaration));

    // Buffer description
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));

    bufferDesc.ByteWidth = (UINT) bytes;
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    if (access == BufferAccess::DYNAMIC)
    {
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    }

    if (data != OMAF_NULL)
    {
        D3D11_SUBRESOURCE_DATA subresourceData;
        ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
        subresourceData.pSysMem = data;
        subresourceData.SysMemPitch = 0;
        subresourceData.SysMemSlicePitch = 0;

        OMAF_DX_CHECK(device->CreateBuffer(&bufferDesc, &subresourceData, &mHandle));
    }
    else
    {
        OMAF_DX_CHECK(device->CreateBuffer(&bufferDesc, NULL, &mHandle));
    }

    return true;
}

void_t
VertexBufferD3D11::bufferData(ID3D11DeviceContext* deviceContext, size_t offset, const void_t* data, size_t bytes)
{
    OMAF_ASSERT_NOT_NULL(mHandle);
    OMAF_ASSERT_NOT_NULL(data);
    OMAF_ASSERT(bytes != 0, "");
    OMAF_ASSERT(mBufferAccess == BufferAccess::DYNAMIC, "");

    D3D11_MAP type = (D3D11_MAP) 0;

    if (mBufferAccess == BufferAccess::DYNAMIC)
    {
        type = D3D11_MAP_WRITE_DISCARD;
    }
    else
    {
        type = D3D11_MAP_WRITE_NO_OVERWRITE;
    }

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;

    OMAF_DX_CHECK(deviceContext->Map(mHandle, 0, type, 0, &mappedSubresource));
    MemoryCopy((uint8_t*) mappedSubresource.pData + offset, data, bytes);
    deviceContext->Unmap(mHandle, 0);

    mBytes = (offset + bytes);
}

void_t VertexBufferD3D11::bind(ID3D11DeviceContext* deviceContext)
{
    OMAF_ASSERT_NOT_NULL(deviceContext);
    OMAF_ASSERT_NOT_NULL(mHandle);

    UINT stride = mVertexDeclaration.getStride();
    UINT offset = 0;

    deviceContext->IASetVertexBuffers(0, 1, &mHandle, &stride, &offset);
}

void_t VertexBufferD3D11::unbind(ID3D11DeviceContext* deviceContext)
{
    deviceContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
}

void_t VertexBufferD3D11::bindCompute(ID3D11DeviceContext* deviceContext,
                                      uint16_t stage,
                                      ComputeBufferAccess::Enum computeAccess)
{
    OMAF_ASSERT_NOT_IMPLEMENTED();
}

void_t VertexBufferD3D11::unbindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage)
{
    OMAF_ASSERT_NOT_IMPLEMENTED();
}

void_t VertexBufferD3D11::destroy()
{
    OMAF_ASSERT_NOT_NULL(mHandle);

    OMAF_DX_RELEASE(mHandle, 0);

    mBytes = 0;
    mBufferAccess = BufferAccess::INVALID;

    Destruct<VertexDeclaration>(&mVertexDeclaration);
}

size_t VertexBufferD3D11::getNumVertices()
{
    return (mBytes / mVertexDeclaration.getStride());
}

const VertexDeclaration& VertexBufferD3D11::getVertexDeclaration() const
{
    return mVertexDeclaration;
}
OMAF_NS_END
