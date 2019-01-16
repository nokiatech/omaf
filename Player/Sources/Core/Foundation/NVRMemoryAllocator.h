
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

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"

#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRBuild.h"

OMAF_NS_BEGIN
    class MemoryAllocator
    {
        public:
            
            MemoryAllocator(const char_t* name);
            virtual ~MemoryAllocator();
            
            const char_t* GetName();
        
            virtual void_t* Allocate(size_t size, size_t align = OMAF_DEFAULT_ALIGN) = 0;
            virtual void_t Free(void_t* ptr) = 0;
        
#if OMAF_MEMORY_TRACKING == 1

            virtual void_t* Allocate_Track(size_t size, size_t align = OMAF_DEFAULT_ALIGN, const char_t* file=NULL,uint32_t line=0) = 0;

#endif
            virtual size_t AllocatedSize(void_t* ptr);
            virtual size_t TotalAllocatedSize();
        
        private:
            
            OMAF_NO_COPY(MemoryAllocator);

        private:
            const char_t* _name;
    };
OMAF_NS_END
