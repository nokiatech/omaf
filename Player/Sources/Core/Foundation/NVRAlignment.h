
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

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"

#include "Foundation/NVRCompatibility.h"

OMAF_NS_BEGIN

size_t align(size_t size, size_t alignment, size_t offset = 0);
void_t* alignPtr(void_t* ptr, size_t align, size_t offset = 0);

void_t* ptrAdd(void_t* ptr, size_t offset);
const void_t* ptrAdd(const void_t* ptr, size_t offset);

void_t* ptrSubtract(void_t* ptr, size_t offset);
const void_t* ptrSubtract(const void_t* ptr, size_t offset);

template <typename T>
OMAF_INLINE bool_t isAligned(T value, size_t alignment, size_t offset)
{
    return ((value + offset) % alignment) == 0;
}

template <typename T>
OMAF_INLINE bool_t isAligned(T* value, size_t alignment, size_t offset)
{
    return ((size_t(value) + offset) % alignment) == 0;
}

OMAF_NS_END
