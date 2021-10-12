
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
#include "Graphics/D3D11/NVRIndexBufferD3D11.h"

#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Graphics/D3D11/NVRD3D11Utilities.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(IndexBufferD3D11)

IndexBufferD3D11::IndexBufferD3D11()
    : mHandle(OMAF_NULL)
    , mBytes(0)
    , mBufferAccess(BufferAccess::INVALID)
{
}

IndexBufferD3D11::~IndexBufferD3D11()
{
    OMAF_ASSERT(mHandle == OMAF_NULL, "Index buffer is not destroyed");
}

const ID3D11Buffer* IndexBufferD3D11::getHandle() const
{
    return mHandle;
}

bool_t IndexBufferD3D11::create(ID3D11Device* device, BufferAccess::Enum access, const void_t* data, size_t bytes)
{
    OMAF_ASSERT_NOT_NULL(device);

    OMAF_ASSERT(!(data != OMAF_NULL && bytes == 0), "");
    OMAF_ASSERT(!(data == OMAF_NULL && bytes != 0), "");

    mBytes = bytes;
    mBufferAccess = access;

    // Buffer description
    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, OMAF_SIZE_OF(bufferDesc));

    bufferDesc.ByteWidth = (UINT) bytes;
    bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
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

void_t IndexBufferD3D11::bufferData(ID3D11DeviceContext* deviceContext, size_t offset, const void_t* data, size_t bytes)
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

void_t IndexBufferD3D11::bind(ID3D11DeviceContext* deviceContext)
{
    OMAF_ASSERT_NOT_NULL(deviceContext);
    OMAF_ASSERT_NOT_NULL(mHandle);

    ID3D11Device* device = OMAF_NULL;
    deviceContext->GetDevice(&device);

    OMAF_ASSERT_NOT_NULL(device);
    OMAF_ASSERT(device->GetFeatureLevel() > D3D_FEATURE_LEVEL_9_1,
                "Only DXGI_FORMAT_R32_UINT index buffer format is supported currently");

    // DXGI_FORMAT_R32_UINT;
    DXGI_FORMAT format = DXGI_FORMAT_R32_UINT;
    deviceContext->IASetIndexBuffer(mHandle, format, 0);

    OMAF_DX_RELEASE(device, 0);
}

void_t IndexBufferD3D11::unbind(ID3D11DeviceContext* deviceContext)
{
    deviceContext->IASetIndexBuffer(NULL, (DXGI_FORMAT) 0, 0);
}

void_t IndexBufferD3D11::bindCompute(ID3D11DeviceContext* deviceContext,
                                     uint16_t stage,
                                     ComputeBufferAccess::Enum computeAccess)
{
    OMAF_ASSERT_NOT_IMPLEMENTED();
}

void_t IndexBufferD3D11::unbindCompute(ID3D11DeviceContext* deviceContext, uint16_t stage)
{
    OMAF_ASSERT_NOT_IMPLEMENTED();
}

void_t IndexBufferD3D11::destroy()
{
    OMAF_ASSERT_NOT_NULL(mHandle);

    OMAF_DX_RELEASE(mHandle, 0);

    mBytes = 0;
    mBufferAccess = BufferAccess::INVALID;
}
OMAF_NS_END
