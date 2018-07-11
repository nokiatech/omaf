
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
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

//
// Lightweight polling user-mode locking primitive.
// Waiting thread is actively waiting in busy loop guarded by atomic counter.
//
// Note: Spinlock is suited best in cases where lock is not contended and perform poorly when a lock is contended for an extended period.
//       Use spinlocks when performance is absolutely critical and contention is rare.
//

class Spinlock
{
    public:
    
        Spinlock();
        ~Spinlock();
    
        void_t lock();
    
        bool_t tryLock();
        bool_t tryLock(uint32_t numTries);
    
        void_t unlock();
    
        void_t waitUnlock();
    
    public:

        // Lock guard to aid mutex usage. Combines explicit lock/unlock access and RIAA style scope lock functionality.
        class LockGuard
        {
            public:
            
                LockGuard(Spinlock& lock, bool_t locked = true)
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
            
                Spinlock& mLock;
                OMAF_VOLATILE bool_t mLocked;
        };
    
        // Spinlock wrapper that provides a RAII-style mechanism for owning a mutex for the duration of a scoped block.
        class ScopeLock
        {
            public:
            
                ScopeLock(Spinlock& lock)
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
            
                Spinlock& mLock;
        };
    
    private:
        Thread::Id mThreadID;
        OMAF_VOLATILE int32_t mCounter;
};

OMAF_NS_END
