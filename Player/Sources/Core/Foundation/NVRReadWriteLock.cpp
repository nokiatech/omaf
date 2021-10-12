
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
#include "Foundation/NVRReadWriteLock.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRDependencies.h"

// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
// http://pubs.opengroup.org/onlinepubs/007908775/xsh/pthread_rwlock_init.html

OMAF_NS_BEGIN

ReadWriteLock::ReadWriteLock()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::InitializeSRWLock(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_init(&mHandle, NULL);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

ReadWriteLock::~ReadWriteLock()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    // SRW locks do not need to be explicitly destroyed.

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_destroy(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ReadWriteLock::lockShared()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::AcquireSRWLockShared(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_rdlock(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ReadWriteLock::unlockShared()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::ReleaseSRWLockShared(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_unlock(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ReadWriteLock::lockExclusive()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::AcquireSRWLockExclusive(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_wrlock(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

void_t ReadWriteLock::unlockExclusive()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::ReleaseSRWLockExclusive(&mHandle);

#elif OMAF_PLATFORM_ANDROID

    int result = ::pthread_rwlock_unlock(&mHandle);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    OMAF_UNUSED_VARIABLE(result);

#else

#error Unsupported platform

#endif
}

OMAF_NS_END
