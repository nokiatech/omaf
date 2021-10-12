
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

#include "Foundation/NVRArray.h"

OMAF_NS_BEGIN
template <typename T>
DataBuffer<T>::DataBuffer(MemoryAllocator& allocator, size_t capacity)
    : mAllocator(allocator)
    , mCapacity(capacity)
    , mSize(0)
    , mDataPtr(OMAF_NULL)
{
    if (mCapacity > 0)
    {
        mDataPtr = OMAF_NEW_ARRAY(mAllocator, T, mCapacity);
        OMAF_ASSERT_NOT_NULL(mDataPtr);
    }
}

template <typename T>
DataBuffer<T>::~DataBuffer()
{
    if (mDataPtr != OMAF_NULL)
    {
        OMAF_DELETE_ARRAY(mAllocator, mDataPtr);
    }
    mCapacity = 0;
}

// Keeps old data, adjusts size if new capacity is smaller.
template <typename T>
void_t DataBuffer<T>::reserve(size_t capacity)
{
    T* newPtr = OMAF_NULL;
    if (capacity > 0)
    {
        newPtr = OMAF_NEW_ARRAY(mAllocator, T, capacity);
        OMAF_ASSERT_NOT_NULL(newPtr);
    }

    if (mSize > capacity)
        mSize = capacity;

    if ((mSize > 0) && (capacity > 0))
    {
        memcpy(newPtr, mDataPtr, mSize * sizeof(T));
    }

    OMAF_DELETE_ARRAY(mAllocator, mDataPtr);
    mDataPtr = newPtr;
    mCapacity = capacity;
}

// See note in NVRDataBuffer.h!
template <typename T>
void_t DataBuffer<T>::reAllocate(size_t capacity)
{
    mSize = 0;
    if (mCapacity > 0)
    {
        OMAF_DELETE_ARRAY(mAllocator, mDataPtr);
        mDataPtr = OMAF_NULL;
        mCapacity = 0;
    }
    if (capacity > 0)
    {
        mDataPtr = OMAF_NEW_ARRAY(mAllocator, T, capacity);
        OMAF_ASSERT_NOT_NULL(mDataPtr);
        mCapacity = capacity;
    }
}

template <typename T>
MemoryAllocator& DataBuffer<T>::getAllocator()
{
    return mAllocator;
}

template <typename T>
size_t DataBuffer<T>::getCapacity() const
{
    return mCapacity;
}

template <typename T>
size_t DataBuffer<T>::getSize() const
{
    return mSize;
}
template <typename T>
void_t DataBuffer<T>::setSize(size_t dataSize)
{
    mSize = dataSize;
}

template <typename T>
void_t DataBuffer<T>::clear()
{
    mSize = 0;
}

template <typename T>
void_t DataBuffer<T>::setData(const T* data, size_t dataSize)
{
    memcpy(mDataPtr, data, dataSize * sizeof(T));
    mSize = dataSize;
}

template <typename T>
T* DataBuffer<T>::getDataPtr()
{
    return mDataPtr;
}

template <typename T>
const T* DataBuffer<T>::getDataPtr() const
{
    return mDataPtr;
}
OMAF_NS_END
