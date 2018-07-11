
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
#include "Foundation/NVRMutex.h"

#include "Foundation/NVRAssert.h"

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
// http://pubs.opengroup.org/onlinepubs/7908799/xsh/pthread_mutex_init.html

OMAF_NS_BEGIN

Mutex::Mutex()
{
#if OMAF_PLATFORM_WINDOWS || defined(OMAF_PLATFORM_UWP)
    
    ::InitializeCriticalSection(&mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    ::pthread_mutexattr_t attr;
    ::pthread_mutexattr_init(&attr);
    ::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    ::pthread_mutex_init(&mHandle, &attr);
    ::pthread_mutexattr_destroy(&attr);
    
#else
    
    #error Unsupported platform
    
#endif
}

Mutex::~Mutex()
{
#if defined(OMAF_PLATFORM_WINDOWS) || defined(OMAF_PLATFORM_UWP)
    
    ::DeleteCriticalSection(&mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    ::pthread_mutex_destroy(&mHandle);
    
#else
    
    #error Unsupported platform
    
#endif
}

void_t Mutex::lock()
{
#if defined(OMAF_PLATFORM_WINDOWS) || defined(OMAF_PLATFORM_UWP)
    
    ::EnterCriticalSection(&mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::pthread_mutex_lock(&mHandle);
    OMAF_ASSERT(result == 0, "Mutex lock failed");
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

void_t Mutex::unlock()
{
#if defined(OMAF_PLATFORM_WINDOWS) || defined(OMAF_PLATFORM_UWP)
    
    ::LeaveCriticalSection(&mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::pthread_mutex_unlock(&mHandle);
    OMAF_ASSERT(result == 0, "Mutex unlock failed");
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

bool_t Mutex::tryLock()
{
#if defined(OMAF_PLATFORM_WINDOWS) || defined(OMAF_PLATFORM_UWP)
    
    return ::TryEnterCriticalSection(&mHandle) != FALSE;
    
#elif OMAF_PLATFORM_ANDROID
    
    return (::pthread_mutex_trylock(&mHandle) == 0);
    
#else
    
    #error Unsupported platform
    
#endif
}

OMAF_NS_END
