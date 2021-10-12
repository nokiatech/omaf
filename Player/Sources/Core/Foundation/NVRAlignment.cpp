
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
#include "Foundation/NVRAlignment.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

size_t align(size_t size, size_t alignment, size_t offset)
{
    OMAF_ASSERT((alignment & (alignment - 1)) == 0, "Alignment should be power-of-two");

    return (size + offset + alignment - 1) & ~(alignment - 1);
}

void_t* alignPtr(void_t* ptr, size_t alignment, size_t offset)
{
    OMAF_ASSERT_NOT_NULL(ptr);
    OMAF_ASSERT((alignment & (alignment - 1)) == 0, "Alignment should be power-of-two");

    size_t address = align((size_t) ptr, alignment, 0) + offset;

    return (void_t*) address;
}

void_t* ptrAdd(void_t* ptr, size_t offset)
{
    OMAF_ASSERT_NOT_NULL(ptr);

    return (void_t*) ((char_t*) ptr + offset);
}

const void_t* ptrAdd(const void_t* ptr, size_t offset)
{
    OMAF_ASSERT_NOT_NULL(ptr);

    return (const void_t*) ((const char_t*) ptr + offset);
}

void_t* ptrSubtract(void_t* ptr, size_t offset)
{
    OMAF_ASSERT_NOT_NULL(ptr);

    return (void_t*) ((char_t*) ptr - offset);
}

const void_t* ptrSubtract(const void_t* ptr, size_t offset)
{
    OMAF_ASSERT_NOT_NULL(ptr);

    return (const void_t*) ((const char_t*) ptr - offset);
}

OMAF_NS_END
