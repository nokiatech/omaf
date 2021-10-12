
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Foundation/NVRConditionVariable.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRDependencies.h"

#include "Foundation/NVRMutex.h"

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
// http://pubs.opengroup.org/onlinepubs/007908799/xsh/pthread_condattr_init.html

OMAF_NS_BEGIN

ConditionVariable::ConditionVariable()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::InitializeConditionVariable(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    pthread_condattr_t attr;
    ::pthread_condattr_init(&attr);

    int result = ::pthread_cond_init(&mHandle, &attr);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

ConditionVariable::~ConditionVariable()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_cond_destroy(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ConditionVariable::wait(Mutex& mutex)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::SleepConditionVariableCS(&mHandle, &mutex.mHandle, INFINITE);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_cond_wait(&mHandle, &mutex.mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ConditionVariable::wait(Mutex& mutex, uint32_t timeoutMs)
{
    if (timeoutMs == 0)
    {
        return;
    }

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::SleepConditionVariableCS(&mHandle, &mutex.mHandle, timeoutMs);

#elif OMAF_PLATFORM_ANDROID

    struct timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += timeoutMs / 1000;
    ts.tv_nsec += (timeoutMs % 1000) * 1000000;

    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_nsec -= 1000000000;
        ts.tv_sec++;
    }

    int result = ::pthread_cond_timedwait(&mHandle, &mutex.mHandle, &ts);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ConditionVariable::signal()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::WakeConditionVariable(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_cond_signal(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ConditionVariable::broadcast()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::WakeAllConditionVariable(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_cond_broadcast(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

OMAF_NS_END
