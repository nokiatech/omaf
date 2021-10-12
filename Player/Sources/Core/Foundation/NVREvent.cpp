
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
#include "Foundation/NVREvent.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

#if OMAF_PLATFORM_ANDROID

// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html
// http://www.cs.wustl.edu/~schmidt/win32-cv-2.html

void eventInit(Event::Handle* event, bool manualReset, bool initialState)
{
    event->manualReset = manualReset;
    event->isSignaled = initialState;
    event->waitingThreads = 0;

    ::pthread_cond_init(&event->condition, NULL);
    ::pthread_mutex_init(&event->lock, NULL);
}

void eventDestroy(Event::Handle* event)
{
    ::pthread_mutex_destroy(&event->lock);
    ::pthread_cond_destroy(&event->condition);
}

bool eventWait(Event::Handle* event, const timespec* timeout)
{
    bool result = true;

    // Grab the lock first
    ::pthread_mutex_lock(&event->lock);

    // Event is currently signaled
    if (event->isSignaled)
    {
        if (!event->manualReset)
        {
            // AUTO: reset state
            event->isSignaled = 0;
        }
    }
    else  // Event is currently not signaled
    {
        event->waitingThreads++;

        if (timeout)
        {
            if (::pthread_cond_timedwait(&event->condition, &event->lock, timeout) == ETIMEDOUT)
            {
                result = false;
            }
        }
        else
        {
            ::pthread_cond_wait(&event->condition, &event->lock);
        }

        event->waitingThreads--;
    }

    // Now we can let go of the lock
    pthread_mutex_unlock(&event->lock);

    return result;
}

void eventSignal(Event::Handle* event)
{
    // Grab the lock first
    ::pthread_mutex_lock(&event->lock);

    // Manual-reset event
    if (event->manualReset)
    {
        // Signal event
        event->isSignaled = true;

        // Wakeup all
        ::pthread_cond_broadcast(&event->condition);
    }
    else  // Auto-reset event
    {
        if (event->waitingThreads == 0)
        {
            // No waiters: signal event
            event->isSignaled = true;
        }
        else
        {
            // Waiters: wakeup one waiter
            ::pthread_cond_signal(&event->condition);
        }
    }

    // Now we can let go of the lock
    ::pthread_mutex_unlock(&event->lock);
}

void eventPulse(Event::Handle* event)
{
    // Grab the lock first
    ::pthread_mutex_lock(&event->lock);

    // Manual-reset event
    if (event->manualReset)
    {
        // Wakeup all waiters
        ::pthread_cond_broadcast(&event->condition);
    }
    else  // Auto-reset event: wakeup one waiter
    {
        ::pthread_cond_signal(&event->condition);

        // Reset event
        event->isSignaled = false;
    }

    // Now we can let go of the lock
    ::pthread_mutex_unlock(&event->lock);
}

void eventReset(Event::Handle* event)
{
    // Grab the lock first
    ::pthread_mutex_lock(&event->lock);

    // Reset event
    event->isSignaled = false;

    // Now we can let go of the lock
    ::pthread_mutex_unlock(&event->lock);
}

#endif

Event::Event(bool_t manualReset, bool_t initialState)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    mHandle = ::CreateEvent(NULL, manualReset, initialState, NULL);
    OMAF_ASSERT(mHandle != NULL, "Failed to create condition");

#elif OMAF_PLATFORM_ANDROID

    eventInit(&mHandle, manualReset, initialState);

#endif
}

Event::~Event()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    BOOL result = ::CloseHandle(mHandle);
    OMAF_ASSERT(result == TRUE, "Failed to close condition");

#elif OMAF_PLATFORM_ANDROID

    eventDestroy(&mHandle);

#endif
}

void_t Event::signal()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    BOOL result = ::SetEvent(mHandle);
    OMAF_ASSERT(result == TRUE, "Failed to signal condition");

#elif OMAF_PLATFORM_ANDROID

    eventSignal(&mHandle);

#endif
}

void_t Event::reset()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    BOOL result = ::ResetEvent(mHandle);
    OMAF_ASSERT(result == TRUE, "Failed to reset condition");

#elif OMAF_PLATFORM_ANDROID

    eventReset(&mHandle);

#endif
}

bool_t Event::wait()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    DWORD result = ::WaitForSingleObject(mHandle, INFINITE);

    return (result == WAIT_OBJECT_0);

#elif OMAF_PLATFORM_ANDROID

    return eventWait(&mHandle, NULL);

#endif
}

bool_t Event::wait(uint32_t timeoutMs)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    DWORD result = ::WaitForSingleObject(mHandle, timeoutMs);

    if (result != WAIT_OBJECT_0)
    {
        OMAF_ASSERT(result == WAIT_TIMEOUT, "Failed to wait for condition");

        return false;
    }

    return true;

#elif OMAF_PLATFORM_ANDROID

    // pthread_cond_timedwait takes absolute time
    timespec abstime;

    ::clock_gettime(CLOCK_REALTIME, &abstime);

    abstime.tv_sec += timeoutMs / 1000;
    abstime.tv_nsec += (timeoutMs % 1000) * 1000000;

    if (abstime.tv_nsec >= 1000000000)
    {
        ++abstime.tv_sec;
        abstime.tv_nsec -= 1000000000;
    }

    return eventWait(&mHandle, &abstime);

#endif
}

OMAF_NS_END
