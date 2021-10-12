
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

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "API/OMAFIDataBuffer.h"

OMAF_NS_BEGIN
template <typename T>
class
DataBuffer : public IDataBuffer<T>
{
public:
    DataBuffer(MemoryAllocator& allocator, size_t capacity);

    ~DataBuffer();

    MemoryAllocator& getAllocator();

    // Does not keep the old data, so typically is used before adding any data, e.g. if using a list of pre-allocated
    // buffers and reallocating if needed
    void_t reAllocate(size_t capacity);

    // Keeps old data, adjusts size if new capacity is smaller.
    void_t reserve(size_t capacity);

    size_t getCapacity() const;
    size_t getSize() const;
    void_t setSize(size_t dataSize);

    void_t clear();
    void_t setData(const T* data, size_t dataSize);
    T* getDataPtr();
    const T* getDataPtr() const;

private:
    OMAF_NO_DEFAULT(DataBuffer);
    OMAF_NO_ASSIGN(DataBuffer);

private:
    MemoryAllocator& mAllocator;

    size_t mCapacity;
    size_t mSize;
    T* mDataPtr;
};
OMAF_NS_END

#include "Foundation/NVRDataBufferImpl.h"
