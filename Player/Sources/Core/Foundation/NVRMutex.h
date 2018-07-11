
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

class ConditionVariable;

//
// Recursive mutex
//
// Note: Windows family implementation uses OS critical section instead OS mutex.
//       Critical section is lightweight solution for exclusive locking and can be used only by the threads of a single process.
//       More information about this decision can be found here: https://software.intel.com/en-us/articles/choosing-between-synchronization-primitives
//

class Mutex
{
    public:
    
        friend class ConditionVariable;
    
    public:
        
        Mutex();
        ~Mutex();
    
        // Acquires (locks) OS mutex / critical section. If mutex / critical section is already locked by another thread method blocks thread until mutex / critical section is unlocked.
        void_t lock();
    
        // Tries to acquire (lock) OS mutex / critical section. If mutex / critical section is already locked by another thread method returns false without locking mutex / critical section.
        bool_t tryLock();
    
        // Releases (unlocks) OS mutex / critical section.
        void_t unlock();

    public:
    
        // Lock guard to aid mutex usage. Combines explicit lock/unlock access and RIAA style scope lock functionality.
        class LockGuard
        {
            public:
                
                LockGuard(Mutex& lock, bool_t locked = true)
                : mLock(lock)
                , mLocked(locked)
                {
                    if (locked)
                    {
                        this->lock();
                    }
                }
                
                ~LockGuard()
                {
                    if (mLocked)
                    {
                        this->unlock();
                    }
                }
            
                void_t lock()
                {
                    mLock.lock();
                    mLocked = true;
                }
            
                void_t unlock()
                {
                    mLock.unlock();
                    mLocked = false;
                }
            
                bool_t tryLock()
                {
                    mLocked = mLock.tryLock();
                    
                    return mLocked;
                }
            
                bool_t isLocked() const
                {
                    return mLocked;
                }

            private:
                
                OMAF_NO_DEFAULT(LockGuard);
                OMAF_NO_COPY(LockGuard);
                OMAF_NO_ASSIGN(LockGuard);
                
            private:
                
                Mutex& mLock;
                OMAF_VOLATILE bool_t mLocked;
        };
    
        // Mutex wrapper that provides a RAII-style mechanism for owning a mutex for the duration of a scoped block.
        class ScopeLock
        {
            public:
            
                ScopeLock(Mutex& lock)
                : mLock(lock)
                {
                    mLock.lock();
                }
                
                ~ScopeLock()
                {
                    mLock.unlock();
                }
                
            private:
                
                OMAF_NO_DEFAULT(ScopeLock);
                OMAF_NO_COPY(ScopeLock);
                OMAF_NO_ASSIGN(ScopeLock);
                
            private:
                
                Mutex& mLock;
        };
    
    private:
        
        OMAF_NO_COPY(Mutex);
        OMAF_NO_ASSIGN(Mutex);
        
    private:
        
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        
        typedef CRITICAL_SECTION Handle;
        
#elif OMAF_PLATFORM_ANDROID
        
        typedef pthread_mutex_t Handle;
        
#else
        
    #error Unsupported platform
        
#endif
        
        Handle mHandle;
};

OMAF_NS_END
