
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
#include "Foundation/NVRMemoryAllocator.h"

#include "Platform/OMAFCompiler.h"

OMAF_NS_BEGIN
    MemoryAllocator::MemoryAllocator(const char_t* name)
    : _name(name)
    {
    }
    
    MemoryAllocator::~MemoryAllocator()
    {
    }
            
    const char_t* MemoryAllocator::GetName()
    {
        return _name;
    }

    size_t MemoryAllocator::AllocatedSize(void_t* ptr)
    {
        OMAF_UNUSED_VARIABLE(ptr);
        
        return (size_t)-1;
    }
    
    size_t MemoryAllocator::TotalAllocatedSize()
    {
        return (size_t)-1;
    }
OMAF_NS_END
