
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
#include "Foundation/NVRSemaphore.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRDependencies.h"

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
// https://developer.apple.com/library/content/documentation/Darwin/Conceptual/KernelProgramming/synchronization/synchronization.html
// http://www.linuxdevcenter.com/pub/a/linux/2007/05/24/semaphores-in-linux.html?page=4

OMAF_NS_BEGIN

Semaphore::Semaphore(uint32_t initialCount)
: mInitialCount(initialCount)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    mHandle = (Handle)::CreateSemaphore(NULL, mInitialCount, 0x7fffffff, NULL);
    OMAF_ASSERT_NOT_NULL(mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::sem_init(&mHandle, 1, mInitialCount);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

Semaphore::~Semaphore()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    BOOL result = ::CloseHandle(mHandle);
    OMAF_ASSERT(result == TRUE,"");
    
#elif OMAF_PLATFORM_ANDROID

    int result = ::sem_destroy(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

void_t Semaphore::signal()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    LONG count = 0;
    BOOL result = ::ReleaseSemaphore(mHandle, 1, &count);
    OMAF_ASSERT(result == TRUE,"");
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::sem_post(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

void_t Semaphore::wait()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    DWORD result = ::WaitForSingleObject(mHandle, INFINITE);
    OMAF_ASSERT(result != WAIT_FAILED,"");
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::sem_wait(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

bool_t Semaphore::tryWait()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    DWORD result = ::WaitForSingleObject(mHandle, 0);
    
    return (result == WAIT_OBJECT_0);
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::sem_trywait(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    
    return (result == 0);
    
#else
    
    #error Unsupported platform
    
#endif
}

void_t Semaphore::reset()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    BOOL result = ::CloseHandle(mHandle);
    OMAF_ASSERT(result == TRUE,"");
    
    mHandle = (Handle)::CreateSemaphore(NULL, mInitialCount, 0x7fffffff, NULL);
    OMAF_ASSERT_NOT_NULL(mHandle);
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = ::sem_destroy(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    
    result = ::sem_init(&mHandle, 0, mInitialCount);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);
    
#else
    
    #error Unsupported platform
    
#endif
}

OMAF_NS_END
