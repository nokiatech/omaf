
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
#include "Foundation/NVRMemorySystem.h"

#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRMallocAllocator.h"
#include "Foundation/NVRConstruct.h"
#include "Foundation/NVRNew.h"

OMAF_NS_BEGIN

#if OMAF_DEBUG_BUILD || OMAF_ENABLE_LOG

#define OMAF_ENABLE_DEBUG_HEAP 1

#endif

namespace MemorySystem
{
    const uint32_t DEFAULT_SCRATCH_ALLOCATOR_SIZE = 1024 * 1024 * 4;
    
    static const uint32_t DEFAULT_BOOTSTRAP_ALLOCATOR_SIZE = 1024 * 1024 / 4;
    uint8_t bootstrapStack[DEFAULT_BOOTSTRAP_ALLOCATOR_SIZE];
    
    MemoryAllocator* defaultHeapAllocator = OMAF_NULL;
    MemoryAllocator* debugHeapAllocator = OMAF_NULL;
    
    void_t Create(MemoryAllocator* clientHeapAllocator, size_t scratchAllocatorSize)
    {
        if (clientHeapAllocator != OMAF_NULL) // Use client defined default heap allocator
        {
            defaultHeapAllocator = clientHeapAllocator;
        }
        else // Fallback to use malloc allocator as default heap allocator
        {
            OMAF_ASSERT(defaultHeapAllocator == OMAF_NULL, "");

            defaultHeapAllocator = new (bootstrapStack) MallocAllocator("Default heap allocator");
            OMAF_ASSERT_NOT_NULL(defaultHeapAllocator);
        }
        
        // TODO: Default scratch allocator
        OMAF_UNUSED_VARIABLE(scratchAllocatorSize);
        
        // Create debug heap allocator
        #if OMAF_ENABLE_DEBUG_HEAP

            debugHeapAllocator = OMAF_NEW(*defaultHeapAllocator, MallocAllocator)("Debug heap allocator");

        #endif
    }
    
    void_t Destroy()
    {
        #if OMAF_ENABLE_DEBUG_HEAP

            if (debugHeapAllocator != OMAF_NULL)
            {
                OMAF_DELETE(*defaultHeapAllocator, debugHeapAllocator);
                debugHeapAllocator = OMAF_NULL;
            }

        #endif
        
        if (defaultHeapAllocator != OMAF_NULL)
        {
            Destruct<MallocAllocator>(defaultHeapAllocator);
            defaultHeapAllocator = OMAF_NULL;
        }
    }
    
    MemoryAllocator* DefaultHeapAllocator()
    {
        // Make sure memory system and default heap allocator is initialized properly
        OMAF_ASSERT_NOT_NULL(defaultHeapAllocator);
        
        return defaultHeapAllocator;
    }
    
    MemoryAllocator* DefaultScratchAllocator()
    {
        OMAF_ASSERT_NOT_IMPLEMENTED();
        
        return OMAF_NULL;
    }
    
    MemoryAllocator* DebugHeapAllocator()
    {
        // Make sure memory system and debug heap allocator is initialized properly
        OMAF_ASSERT_NOT_NULL(debugHeapAllocator);
        
        return debugHeapAllocator;
    }
}

OMAF_NS_END
