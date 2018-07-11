
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
#include "Foundation/NVRMemoryUtilities.h"

#include "Foundation/NVRDependencies.h"

OMAF_NS_BEGIN
    void_t MemoryCopy(void_t* destination, const void_t* source, size_t size)
    {
#if OMAF_PLATFORM_WINDOWS
        
        ::memcpy_s(destination, size, source, size);
        
#else
        
        ::memcpy(destination, source, size);
        
#endif
    }
    
    void_t MemoryMove(void_t* destination, const void_t* source, size_t size)
    {
#if OMAF_PLATFORM_WINDOWS
        
        ::memmove_s(destination, size, source, size);
        
#else
        
        ::memmove(destination, source, size);
        
#endif
    }
    
    void_t MemorySet(void_t* destination, int32_t value, size_t size)
    {
        ::memset(destination, value, size);
    }
    
    void_t MemoryZero(void_t* destination, size_t size)
    {
        ::memset(destination, 0, size);
    }
    
    int32_t MemoryCompare(const void_t* ptr0, const void_t* ptr1, size_t size)
    {
        return ::memcmp(ptr0, ptr1, size);
    }
OMAF_NS_END
