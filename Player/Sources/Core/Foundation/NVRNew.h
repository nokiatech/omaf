
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

#include "Foundation/NVRConstruct.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRMemorySystem.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

#if OMAF_MEMORY_TRACKING == 1
#define OMAF_ALLOC(allocator, size) \
    OMAF_NS::MemoryAllocationProxy::Allocate(allocator, size, OMAF_DEFAULT_ALIGN, OMAF_FILE, OMAF_LINE)
#define OMAF_ALLOC_ALIGN(allocator, size, align) \
    OMAF_NS::MemoryAllocationProxy::Allocate(allocator, size, align, OMAF_FILE, OMAF_LINE)
#else
#define OMAF_ALLOC(allocator, size) OMAF_NS::MemoryAllocationProxy::Allocate(allocator, size)
#define OMAF_ALLOC_ALIGN(allocator, size, align) OMAF_NS::MemoryAllocationProxy::Allocate(allocator, size, align)
#endif

#define OMAF_FREE(allocator, ptr) MemoryAllocationProxy::Free(allocator, ptr)

#if OMAF_MEMORY_TRACKING == 1
#define OMAF_NEW(allocator, type)                                                                                \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(allocator, OMAF_SIZE_OF(type), OMAF_ALIGN_OF(type), OMAF_FILE, \
                                                  OMAF_LINE)) type
#define OMAF_NEW_ALIGN(allocator, type, align) \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(allocator, OMAF_SIZE_OF(type), align, OMAF_FILE, OMAF_LINE)) type

#define OMAF_NEW_ARRAY(allocator, type, count) \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(allocator, count, OMAF_ALIGN_OF(type), OMAF_FILE, OMAF_LINE)
#define OMAF_NEW_ARRAY_ALIGN(allocator, type, count, align) \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(allocator, count, align, OMAF_FILE, OMAF_LINE)
#else
#define OMAF_NEW(allocator, type) \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(allocator, OMAF_SIZE_OF(type), OMAF_ALIGN_OF(type))) type
#define OMAF_NEW_ALIGN(allocator, type, align) \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(allocator, OMAF_SIZE_OF(type), align)) type

#define OMAF_NEW_ARRAY(allocator, type, count) \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(allocator, count, OMAF_ALIGN_OF(type))
#define OMAF_NEW_ARRAY_ALIGN(allocator, type, count, align) \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(allocator, count, align)
#endif

#define OMAF_DELETE(allocator, ptr) OMAF_NS::MemoryAllocationProxy::Delete(allocator, ptr)
#define OMAF_DELETE_ARRAY(allocator, ptr) OMAF_NS::MemoryAllocationProxy::DeleteArray(allocator, ptr)

#if OMAF_MEMORY_TRACKING == 1
#define OMAF_NEW_HEAP(type)                                                                                  \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(*MemorySystem::DefaultHeapAllocator(), OMAF_SIZE_OF(type), \
                                                  OMAF_ALIGN_OF(type), OMAF_FILE, OMAF_LINE)) type
#define OMAF_NEW_ALIGN_HEAP(type, align)                                                                              \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(*Private::MemorySystem::DefaultHeapAllocator(), OMAF_SIZE_OF(type), \
                                                  align, OMAF_FILE, OMAF_LINE)) type
#define OMAF_NEW_ARRAY_HEAP(type, count)                                                                  \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(*Private::MemorySystem::DefaultHeapAllocator(), count, \
                                                   OMAF_ALIGN_OF(type), OMAF_FILE, OMAF_LINE)
#define OMAF_NEW_ARRAY_ALIGN_HEAP(type, count, align)                                                            \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(*Private::MemorySystem::DefaultHeapAllocator(), count, align, \
                                                   OMAF_FILE, OMAF_LINE)
#else
#define OMAF_NEW_HEAP(type)                                                                                           \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(*Private::MemorySystem::DefaultHeapAllocator(), OMAF_SIZE_OF(type), \
                                                  OMAF_ALIGN_OF(type))) type
#define OMAF_NEW_ALIGN_HEAP(type, align)                                                                              \
    new (OMAF_NS::MemoryAllocationProxy::Allocate(*Private::MemorySystem::DefaultHeapAllocator(), OMAF_SIZE_OF(type), \
                                                  align)) type
#define OMAF_NEW_ARRAY_HEAP(type, count)                                                                  \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(*Private::MemorySystem::DefaultHeapAllocator(), count, \
                                                   OMAF_ALIGN_OF(type))
#define OMAF_NEW_ARRAY_ALIGN_HEAP(type, count, align) \
    OMAF_NS::MemoryAllocationProxy::NewArray<type>(*Private::MemorySystem::DefaultHeapAllocator(), count, align)
#endif

#define OMAF_DELETE_HEAP(ptr) \
    OMAF_NS::MemoryAllocationProxy::Delete(*Private::MemorySystem::DefaultHeapAllocator(), ptr)
#define OMAF_DELETE_ARRAY_HEAP(ptr) \
    OMAF_NS::MemoryAllocationProxy::DeleteArray(*Private::MemorySystem::DefaultHeapAllocator(), ptr)

namespace MemoryAllocationProxy
{
#if OMAF_MEMORY_TRACKING == 1
    OMAF_INLINE void_t* Allocate(MemoryAllocator& allocator, size_t size, const char_t* file, uint32_t line)
    {
        if (size == 0)
        {
            return OMAF_NULL;
        }

        return allocator.Allocate_Track(size, OMAF_DEFAULT_ALIGN, file, line);
    }

    OMAF_INLINE void_t*
    Allocate(MemoryAllocator& allocator, size_t size, size_t align, const char_t* file, uint32_t line)
    {
        if (size == 0)
        {
            return OMAF_NULL;
        }

        return allocator.Allocate_Track(size, align, file, line);
    }
#endif
    OMAF_INLINE void_t* Allocate(MemoryAllocator& allocator, size_t size)
    {
        if (size == 0)
        {
            return OMAF_NULL;
        }

        return allocator.Allocate(size);
    }

    OMAF_INLINE void_t* Allocate(MemoryAllocator& allocator, size_t size, size_t align)
    {
        if (size == 0)
        {
            return OMAF_NULL;
        }

        return allocator.Allocate(size, align);
    }

    OMAF_INLINE void_t Free(MemoryAllocator& allocator, void_t* ptr)
    {
        if (ptr != OMAF_NULL)
        {
            allocator.Free(ptr);
        }
    }

#if OMAF_MEMORY_TRACKING == 1
    template <class T>
    OMAF_INLINE T* New(MemoryAllocator& allocator, size_t align, const char_t* file, uint32_t line)
    {
        void_t* ptr = Allocate(allocator, OMAF_SIZE_OF(T), align, file, line);

        if (ptr != OMAF_NULL)
        {
            return Construct<T>(ptr);
        }

        return OMAF_NULL;
    }

    template <class T>
    OMAF_INLINE T* NewArray(MemoryAllocator& allocator, size_t count, size_t align, const char_t* file, uint32_t line)
    {
        void_t* ptr = Allocate(allocator, OMAF_SIZE_OF(T) * count, align, file, line);

        if (ptr != OMAF_NULL)
        {
            return ConstructArray<T>(ptr, count);
        }

        return OMAF_NULL;
    }
#endif
    template <class T>
    OMAF_INLINE T* New(MemoryAllocator& allocator, size_t align)
    {
        void_t* ptr = Allocate(allocator, OMAF_SIZE_OF(T), align);

        if (ptr != OMAF_NULL)
        {
            return Construct<T>(ptr);
        }

        return OMAF_NULL;
    }

    template <class T>
    OMAF_INLINE T* NewArray(MemoryAllocator& allocator, size_t count, size_t align)
    {
        void_t* ptr = Allocate(allocator, OMAF_SIZE_OF(T) * count, align);

        if (ptr != OMAF_NULL)
        {
            return ConstructArray<T>(ptr, count);
        }

        return OMAF_NULL;
    }

    template <class T>
    OMAF_INLINE void_t Delete(MemoryAllocator& allocator, T* ptr)
    {
        if (ptr != OMAF_NULL)
        {
            Destruct<T>(ptr);
            Free(allocator, ptr);
        }
    }

    template <class T>
    OMAF_INLINE void_t DeleteArray(MemoryAllocator& allocator, T* ptr)
    {
        if (ptr != OMAF_NULL)
        {
            size_t size = OMAF_SIZE_OF(T);
            size_t allocatedSize = allocator.AllocatedSize(ptr);
            size_t count = allocatedSize / size;

            DestructArray<T>(ptr, count);
            Free(allocator, ptr);
        }
    }
}  // namespace MemoryAllocationProxy
OMAF_NS_END
