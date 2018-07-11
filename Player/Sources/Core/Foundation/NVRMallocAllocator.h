
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
#include "Foundation/NVRMemoryAllocator.h"
#if OMAF_MEMORY_TRACKING == 1
#include "Foundation/NVRMutex.h"
#endif
OMAF_NS_BEGIN
    class MallocAllocator
    : public MemoryAllocator
    {
        public:
        
            MallocAllocator(const char_t* name);
            virtual ~MallocAllocator();
        
        public: // MemoryAllocator

            virtual void_t* Allocate(size_t size, size_t align);
            virtual void_t Free(void_t* ptr);

#if OMAF_MEMORY_TRACKING == 1

            virtual void_t* Allocate_Track(size_t size, size_t align = OMAF_DEFAULT_ALIGN, const char_t* file = NULL, uint32_t line = 0);

#endif

            virtual size_t AllocatedSize(void_t* ptr);
            virtual size_t TotalAllocatedSize();
    private:
            struct MemoryBlockHeader;
#if OMAF_MEMORY_TRACKING == 1
            Mutex gMutex;
            MemoryBlockHeader * gRoot = NULL;
#endif

    };
OMAF_NS_END
