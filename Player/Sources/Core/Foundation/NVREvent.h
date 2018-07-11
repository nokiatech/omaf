
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Wrapper class for OS / POSIX condition variable based manual-reset / auto-reset event.
//
// - OS event: Windows family (Win32)
// - POSIX condition variable based event: Linux family (Android, Linux), Apple family (iOS, macOS, tvOS)
//
// Manual-reset event: An event object whose state remains signaled until it is explicitly reset to
//                     non-signaled by the reset function. While it is signaled, any number of waiting threads,
//                     or threads that subsequently specify the same event object in one of the wait functions, can be released.
//
// Auto-reset event: An event object whose state remains signaled until a single waiting thread is released,
//                   at which time the system automatically sets the state to nonsignaled.
//                   If no threads are waiting, the event object's state remains signaled.
//                   If more than one thread is waiting, a waiting thread is selected.

class Event
{
    public:
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
        typedef void* Handle;
    
#elif OMAF_PLATFORM_ANDROID
    
        struct Handle
        {
            // Protect critical section
            pthread_mutex_t lock;
        
            // Keeps track of waiters
            pthread_cond_t condition;
        
            // Specifies if this is an auto- or manual-reset event
            bool manualReset;
        
            // "True" if signaled
            bool isSignaled;
        
            // Number of waiting threads
            unsigned waitingThreads;
        };
    
#else
    
    #error Unsupported platform
    
#endif
    
        // Creates event with manual- / auto-reset functionality and initial state.
        OMAF_EXPLICIT Event(bool_t manualReset, bool_t initialState);
    
        // Destroys event.
        ~Event();
    
        // Sets event to the signaled state.
        void_t signal();
    
        // Sets event to the nonsignaled state.
        void_t reset();
    
        // Waits until event is in signaled state.
        bool_t wait();
    
        // Waits until event is in signaled state or the time-out interval elapses.
        bool_t wait(uint32_t timeoutMs);
    
    private:
    
        OMAF_NO_DEFAULT(Event);
        OMAF_NO_COPY(Event);
        OMAF_NO_ASSIGN(Event);
    
    private:
    
        Handle mHandle;
};

OMAF_NS_END
