
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
#pragma once

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFPlatformDetection.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Wrapper class for OS and POSIX condition variable.
//
// - OS condition variable: Windows family (Win32)
// - POSIX condition variable: Linux family (Android, Linux), Apple family (iOS, macOS, tvOS)
//
// Note: Condition variable doesn't have internal state e.g. if signal() is called before wait() it will wait until
// event is signaled again.
//

class Mutex;

class ConditionVariable
{
public:
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    typedef CONDITION_VARIABLE Handle;

#elif OMAF_PLATFORM_ANDROID

    typedef pthread_cond_t Handle;

#else

#error Unsupported platform

#endif

public:
    // Creates OS condition variable.
    ConditionVariable();

    // Destroys OS condition variable.
    ~ConditionVariable();

    // Blocks the calling thread until the specified condition is signalled.
    void_t wait(Mutex& mutex);

    // Blocks the calling thread until the specified condition is signalled or wait time has been passed.
    void_t wait(Mutex& mutex, uint32_t timeoutMs);

    // Signals to wake a waiting thread. It is a logical error to call signal before wait, check notes from class
    // description.
    void_t signal();

    // Signals to wake all waiting threads.
    void_t broadcast();

private:
    OMAF_NO_COPY(ConditionVariable);
    OMAF_NO_ASSIGN(ConditionVariable);

private:
    Handle mHandle;
};

OMAF_NS_END
