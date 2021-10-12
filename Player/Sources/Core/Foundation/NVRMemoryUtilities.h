
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRDependencies.h"

OMAF_NS_BEGIN
// Copy data from one region of memory to another
void_t MemoryCopy(void_t* destination, const void_t* source, size_t size);

// Move data from one region of memory to another
void_t MemoryMove(void_t* destination, const void_t* source, size_t size);

// Set each byte in a region of memory to a given value
void_t MemorySet(void_t* destination, int32_t value, size_t size);

// Set each byte in a region of memory to zero
void_t MemoryZero(void_t* destination, size_t size);

// Compare each byte between two regions of memory
int32_t MemoryCompare(const void_t* ptr0, const void_t* ptr1, size_t size);


template <typename T>
void_t CopyAscending(T* destination, const T* source, size_t size = 1)
{
    for (size_t i = 0; i < size; ++i)
    {
        destination[i] = source[i];
    }
}

template <typename T>
void_t CopyDescending(T* destination, const T* source, size_t size = 1)
{
    for (size_t c = 0, i = size; c < size; ++c, --i)
    {
        destination[i - 1] = source[i - 1];
    }
}
OMAF_NS_END
