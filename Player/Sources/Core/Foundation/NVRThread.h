
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
#include "Foundation/NVRDelegate.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

class Thread
{
public:
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    typedef HANDLE Handle;
    typedef DWORD Id;

#elif OMAF_PLATFORM_ANDROID

    typedef pthread_t Handle;
    typedef pid_t Id;

#else

#error Unsupported platform

#endif

    static const Handle InvalidHandle;
    static const Id InvalidId;

    static const uint32_t DefaultStackSize;

    struct Priority
    {
        enum Enum
        {
            INVALID = -1,

            LOWEST,
            LOW,
            NORMAL,
            HIGH,
            HIGHEST,

            INHERIT,  // Default, inherited priority from spawner thread

            COUNT = HIGHEST + 1,
        };
    };

public:
    typedef uint32_t ReturnValue;
    typedef Delegate<ReturnValue(const Thread&, void_t*)> EntryFunction;

public:
    Thread();
    Thread(const char_t* name, uint32_t stackSize = DefaultStackSize);

    ~Thread();

    // Returns name of OS thread.
    const char_t* getName() const;

    // Set name of OS thread. Needs to be called before start(), values are stored and applied when the OS thread is
    // started.
    bool_t setName(const char_t* name);

    // Returns stack size of OS thread.
    uint32_t getStackSize() const;

    // Set stack size of OS thread. Needs to be called before start(), values are stored and applied when the OS thread
    // is started.
    bool_t setStackSize(uint32_t stackSize);

    // Returns priority of OS thread.
    Priority::Enum getPriority() const;

    // Sets priority of OS thread. Needs to be called before start(), values are stored and applied when the OS thread
    // is started.
    bool_t setPriority(Priority::Enum priority);

    // Creates OS thread and starts it immediately.
    bool_t start(EntryFunction function, void_t* userData = OMAF_NULL);

    // Signals spawned thread to stop. This should be called in context of spawning thread.
    void_t stop();

    // Checks if thread is signalled to stop. This should be called in context of spawned thread.
    bool_t shouldStop() const;

    // Waits OS thread to finish execution.
    bool_t join();

    // Returns if OS thread hasn't finished execution.
    bool_t isRunning() const;

    // Returns if OS thread is valid.
    bool_t isValid() const;

public:
    // Sleeps calling thread given number of milliseconds.
    static void_t sleep(uint32_t milliseconds);

    // Yields the calling thread.
    static void_t yield();

    // Returns current thread ID.
    static Id getCurrentThreadId();

    // Returns main() thread ID.
    static Id getMainThreadId();

    // Checks if calling thread is main() thread.
    static bool_t isMainThread();

private:
    OMAF_NO_COPY(Thread);
    OMAF_NO_ASSIGN(Thread);

private:
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    static DWORD WINAPI threadMain(LPVOID threadInstance);

#elif OMAF_PLATFORM_ANDROID

    static void_t* threadMain(void_t* threadInstance);

#else

#error Unsupported platform

#endif

private:
    static Thread::Id mMainThreadId;

    EntryFunction mEntryFunction;
    void_t* mUserData;
    mutable const char_t* mName;
    uint32_t mStackSize;
    Priority::Enum mPriority;

    Handle mHandle;
    Id mId;

    OMAF_VOLATILE bool_t mStopping;
    OMAF_VOLATILE bool_t mRunning;
};

OMAF_NS_END
