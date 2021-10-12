
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
#include "Foundation/NVRThreadLocalStorage.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

ThreadLocalStorage::ThreadLocalStorage()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    mHandle = ::TlsAlloc();
    OMAF_ASSERT(mHandle != TLS_OUT_OF_INDEXES, "");

#elif OMAF_PLATFORM_ANDROID

    int status = ::pthread_key_create(&mHandle, NULL);
    OMAF_ASSERT(status == 0, "");
    OMAF_UNUSED_VARIABLE(status);

#else

#error Unsupported platform

#endif
}

ThreadLocalStorage::~ThreadLocalStorage()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::TlsFree(mHandle);

#elif OMAF_PLATFORM_ANDROID

    int status = ::pthread_key_delete((pthread_key_t) mHandle);
    OMAF_ASSERT(status == 0, "");
    OMAF_UNUSED_VARIABLE(status);

#else

#error Unsupported platform

#endif
}

void_t* ThreadLocalStorage::getValue() const
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    return ::TlsGetValue(mHandle);

#elif OMAF_PLATFORM_ANDROID

    return (void_t*) ::pthread_getspecific((pthread_key_t) mHandle);

#else

#error Unsupported platform

#endif
}

bool_t ThreadLocalStorage::setValue(void_t* value)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    return (::TlsSetValue(mHandle, value) == TRUE);

#elif OMAF_PLATFORM_ANDROID

    int status = ::pthread_setspecific((pthread_key_t) mHandle, value);

    return (status == 0);

#else

#error Unsupported platform

#endif
}

OMAF_NS_END
