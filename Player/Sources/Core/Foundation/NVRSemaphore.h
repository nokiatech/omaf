
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
// Wrapper class for OS and POSIX semaphore.
//
// - OS semaphores: Windows family (Win32), Apple family (iOS, macOS, tvOS)
// - POSIX semaphores: Linux family (Android, Linux)
//
// Note: Apple family (iOS, macOS, tvOS) doesn't support unnamed POSIX semaphores so it will use Mach semaphores instead.
//

class Semaphore
{
    public:
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
        typedef HANDLE Handle;
    
#elif OMAF_PLATFORM_ANDROID
    
        typedef sem_t Handle;
    
#else
    
    #error Unsupported platform
    
#endif
    
    public:
    
        // Creates OS semaphore with initial count.
        Semaphore(uint32_t initialCount = 0);
    
        // Destroys OS semaphore.
        ~Semaphore();
    
        // Increments (unlocks) the count of OS semaphore.
        void_t signal();
    
        // Decrements (locks) the count of OS semaphore.
        void_t wait();
    
        // Decrements (locks) the count of OS semaphore, on success returns true. If the decrement cannot be immediately performed, then call returns false.
        bool_t tryWait();
    
        // Destroys and creates a new OS semaphore.
        void_t reset();
    
    private:
    
        OMAF_NO_COPY(Semaphore);
        OMAF_NO_ASSIGN(Semaphore);
    
    private:
    
        Handle mHandle;
    
        uint32_t mInitialCount;
};

OMAF_NS_END
