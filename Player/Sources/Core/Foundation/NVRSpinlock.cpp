
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
#include "Foundation/NVRSpinlock.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRAtomic.h"
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

Spinlock::Spinlock()
    : mCounter(0)
{
}

Spinlock::~Spinlock()
{
}

void_t Spinlock::lock()
{
    while (Atomic::compareExchange(&mCounter, 1, 0) != 0)
    {
        Thread::yield();
    }
    mThreadID = Thread::getCurrentThreadId();
}

bool_t Spinlock::tryLock()
{
    int32_t locked = Atomic::compareExchange(&mCounter, 1, 0);

    return (locked == 0);
}

bool_t Spinlock::tryLock(uint32_t numTries)
{
    uint32_t counter = 0;

    while (counter < numTries)
    {
        bool_t result = tryLock();

        if (result)
        {
            return true;
        }

        counter++;
    }

    return true;
}

void_t Spinlock::unlock()
{
    OMAF_ASSERT(mCounter != 0, "Spinlock unlock failed");

    Atomic::exchange(&mCounter, 0);
}

void_t Spinlock::waitUnlock()
{
    while (mCounter != 0)
    {
        Thread::yield();
    }
}

OMAF_NS_END
