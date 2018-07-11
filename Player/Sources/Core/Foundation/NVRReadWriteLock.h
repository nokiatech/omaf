
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
#include "Platform/OMAFPlatformDetection.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Wrapper class for OS read-write lock.
//
// Usage: When multiple threads are reading and writing using a shared resource, exclusive locks can become a bottleneck.
//        This occurs if the reader threads run continuously but write operations are rare.
//
// Note: There is no guarantee about the order in which threads that request ownership will be granted ownership.
//

class ReadWriteLock
{
    public:
        
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
        typedef SRWLOCK Handle;
    
#elif OMAF_PLATFORM_ANDROID
    
        typedef pthread_rwlock_t Handle;
    
#else
    
    #error Unsupported platform
    
#endif
    
    public:
    
        // Creates OS read-write lock.
        ReadWriteLock();
        
        // Destroys OS read-write lock.
        ~ReadWriteLock();
        
        // Acquire a shared read-only access.
        void_t lockShared();
        
        // Release a shared read-only access.
        void_t unlockShared();
        
        // Acquire a exclusive read/write access.
        void_t lockExclusive();
        
        // Release a exclusive read/write access.
        void_t unlockExclusive();
        
    private:
    
        OMAF_NO_COPY(ReadWriteLock);
        OMAF_NO_ASSIGN(ReadWriteLock);
        
    private:
        
        Handle mHandle;
};

OMAF_NS_END
