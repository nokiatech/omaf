
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
#include "NVRRenderBackend.h"

OMAF_NS_BEGIN

template <typename T>
struct StreamBuffer
{
    MemoryBuffer* buffer;

    StreamBuffer()
    : buffer(OMAF_NULL)
    {
    }

    ~StreamBuffer()
    {
        OMAF_ASSERT(buffer == OMAF_NULL, "");
    }

    bool_t allocate(size_t capacity)
    {
        OMAF_ASSERT(buffer == OMAF_NULL, "");

        buffer = RenderBackend::allocate(OMAF_SIZE_OF(T) * (uint32_t)capacity);

        if (buffer == OMAF_NULL)
        {
            return false;
        }

        return true;
    }

    void_t free()
    {
        RenderBackend::free(buffer);
        buffer = OMAF_NULL;
    }

    void_t add(const T* item, size_t addCount = 1)
    {
        OMAF_ASSERT(buffer != OMAF_NULL, "");
        OMAF_ASSERT(buffer->size + (addCount * OMAF_SIZE_OF(T)) <= buffer->capacity, "");
        
        T* ptr = (T*)buffer->data;
        size_t count = getCount();

        for (size_t i = 0; i < addCount; ++i)
        {
            ptr[count + i] = item[i];
        }

        buffer->size += (uint32_t)(OMAF_SIZE_OF(T) * addCount);
    }

    void_t clear()
    {
        buffer->size = 0;
    }

    size_t getCapacity() const
    {
        return buffer->capacity / OMAF_SIZE_OF(T);
    }
    
    size_t getCount() const
    {
        return buffer->size / OMAF_SIZE_OF(T);
    }
    
    bool_t isFull() const
    {
        return getCapacity() == getCount();
    }
};

OMAF_NS_END
