
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
#include "Foundation/NVRMallocAllocator.h"

#include "Foundation/NVRAlignment.h"
#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFPlatformDetection.h"
#if OMAF_MEMORY_TRACKING == 1
#include "Foundation/NVRLogger.h"
#endif
OMAF_NS_BEGIN
struct MallocAllocator::MemoryBlockHeader
{
#if OMAF_MEMORY_TRACKING == 1
    unsigned int BLOB;
    char filename[256];
    unsigned int line;
    MemoryBlockHeader* prev;
    MemoryBlockHeader* next;
#endif
    size_t baseAddress;
    size_t allocSize;
};

MallocAllocator::MallocAllocator(const char_t* name)
    : MemoryAllocator(name)
{
}

MallocAllocator::~MallocAllocator()
{
#if OMAF_MEMORY_TRACKING == 1
    gMutex.lock();
    MemoryBlockHeader* cur = gRoot;
    int leaked = 0;
    while (cur)
    {
        if (cur->BLOB != 0xFEEDCAFE)
        {
            LogDispatcher::logError("MallocAllocator", "MEMORY CORRUPTION DETECTED!");
            break;
        }
        if (cur->filename[0] == 0)
        {
            LogDispatcher::logError("MallocAllocator", "MEMORY LEAK: <NoInfo> %d %p", cur->allocSize,
                                    cur + OMAF_SIZE_OF(MemoryBlockHeader));
        }
        else
        {
            LogDispatcher::logError("MallocAllocator", "MEMORY LEAK: %s:%d", cur->filename, cur->line);
        }
        leaked++;
        cur = cur->next;
    }
    OMAF_ASSERT(leaked == 0, "Memory leak detected");
    gMutex.unlock();
#endif
}

void_t* MallocAllocator::Allocate(size_t size, size_t align)
{
    if (align < OMAF_DEFAULT_ALIGN)
    {
        align = OMAF_DEFAULT_ALIGN;
    }

    const size_t totalBytes = size + (align) + OMAF_SIZE_OF(MemoryBlockHeader);

    char_t* data = (char_t*) ::malloc(totalBytes);

    if (data)
    {
        const void_t* const basePtr = data;

        // Forward ptr by amount of block header
        data += OMAF_SIZE_OF(MemoryBlockHeader);

        // Forward ptr by amount of aligment
        const size_t offset = align - (((size_t) data) % align);
        data += offset;

        // Write block header
        MemoryBlockHeader* headerPtr = (MemoryBlockHeader*) (data - OMAF_SIZE_OF(MemoryBlockHeader));
        headerPtr->baseAddress = (size_t) basePtr;
        headerPtr->allocSize = size;
#if OMAF_MEMORY_TRACKING == 1
        headerPtr->BLOB = 0xFEEDCAFE;
        headerPtr->filename[0] = 0;
        headerPtr->line = 0;
        gMutex.lock();
        headerPtr->prev = NULL;
        headerPtr->next = gRoot;
        if (gRoot)
            gRoot->prev = headerPtr;
        gRoot = headerPtr;
        gMutex.unlock();

#endif
    }

    return data;
}
#if OMAF_MEMORY_TRACKING == 1

void_t* MallocAllocator::Allocate_Track(size_t size, size_t align, const char_t* file, uint32_t line)
{
    if (align < OMAF_DEFAULT_ALIGN)
    {
        align = OMAF_DEFAULT_ALIGN;
    }

    const size_t totalBytes = size + (align) + OMAF_SIZE_OF(MemoryBlockHeader);

    char_t* data = (char_t*) ::malloc(totalBytes);

    if (data)
    {
        const void_t* const basePtr = data;

        // Forward ptr by amount of block header
        data += OMAF_SIZE_OF(MemoryBlockHeader);

        // Forward ptr by amount of aligment
        const size_t offset = align - (((size_t) data) % align);
        data += offset;

        // Write block header
        MemoryBlockHeader* headerPtr = (MemoryBlockHeader*) (data - OMAF_SIZE_OF(MemoryBlockHeader));
        headerPtr->baseAddress = (size_t) basePtr;
        headerPtr->allocSize = size;
        headerPtr->BLOB = 0xFEEDCAFE;
        strncpy(headerPtr->filename, file, 255);
        headerPtr->filename[255] = 0;
        headerPtr->line = line;

        gMutex.lock();
        headerPtr->prev = NULL;
        headerPtr->next = gRoot;
        if (gRoot)
        {
            gRoot->prev = headerPtr;
        }
        gRoot = headerPtr;
        gMutex.unlock();
    }

    return data;
}

#endif
void_t MallocAllocator::Free(void_t* ptr)
{
    if (ptr != OMAF_NULL)
    {
        char_t* dataPtr = (char_t*) ptr;

        // Get block header
        MemoryBlockHeader* headerPtr = (MemoryBlockHeader*) (dataPtr - OMAF_SIZE_OF(MemoryBlockHeader));

        // Construct base address
        dataPtr = (char_t*) headerPtr->baseAddress;
#if OMAF_MEMORY_TRACKING == 1
        if (headerPtr->BLOB != 0xFEEDCAFE)
        {
            LogDispatcher::logError("MallocAllocator", "MEMORY CORRUPTION DETECTED!");
            OMAF_ISSUE_BREAK();
        }
        gMutex.lock();
        if (headerPtr->prev == NULL)
        {
            // headerPtr was the root, so next is now root.
            gRoot = headerPtr->next;
        }
        else
        {
            headerPtr->prev->next = headerPtr->next;
        }
        if (headerPtr->next)
        {
            headerPtr->next->prev = headerPtr->prev;
        }
        gMutex.unlock();
#endif


        ::free(dataPtr);
    }
}

size_t MallocAllocator::AllocatedSize(void_t* ptr)
{
    if (ptr != OMAF_NULL)
    {
        char_t* dataPtr = (char_t*) ptr;

        // Get block header
        MemoryBlockHeader* headerPtr = (MemoryBlockHeader*) (dataPtr - OMAF_SIZE_OF(MemoryBlockHeader));
#if OMAF_MEMORY_TRACKING == 1
        if (headerPtr->BLOB != 0xFEEDCAFE)
        {
            LogDispatcher::logError("MallocAllocator", "MEMORY CORRUPTION DETECTED!");
            OMAF_ISSUE_BREAK();
        }
#endif
        return headerPtr->allocSize;
    }

    return 0;
}

size_t MallocAllocator::TotalAllocatedSize()
{
    return (size_t) -1;
}
OMAF_NS_END
