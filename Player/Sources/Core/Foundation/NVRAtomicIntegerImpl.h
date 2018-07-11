
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
#include "Foundation/NVRAtomic.h"

OMAF_NS_BEGIN

AtomicInteger::AtomicInteger(int32_t value)
: mValue(value)
{
}

AtomicInteger::~AtomicInteger()
{
}

int32_t AtomicInteger::compareAndSet(int32_t value, int32_t compare)
{
    return Atomic::compareExchange(&mValue, value, compare);
}

int32_t AtomicInteger::getAndSet(int32_t value)
{
    return Atomic::exchange(&mValue, value);
}

void_t AtomicInteger::set(int32_t value)
{
    Atomic::exchange(&mValue, value);
}

int32_t AtomicInteger::get()
{
    return mValue;
}

int32_t AtomicInteger::incrementAndGet()
{
    int32_t previous = Atomic::increment(&mValue);
    previous++;
    
    return previous;
}

int32_t AtomicInteger::decrementAndGet()
{
    int32_t previous = Atomic::decrement(&mValue);
    previous--;
    
    return previous;
}

int32_t AtomicInteger::getAndAdd(int32_t value)
{
    return Atomic::add(&mValue, value);
}

int32_t AtomicInteger::getAndSubtract(int32_t value)
{
    return Atomic::subtract(&mValue, value);
}

int32_t AtomicInteger::operator = (int32_t value)
{
    return Atomic::exchange(&mValue, value);
}

bool_t AtomicInteger::operator == (int32_t value)
{
    return (mValue == value);
}

bool_t AtomicInteger::operator != (int32_t value)
{
    return (mValue != value);
}

int32_t AtomicInteger::operator ++ ()
{
    return incrementAndGet();
}

int32_t AtomicInteger::operator -- ()
{
    return decrementAndGet();
}

int32_t AtomicInteger::operator += (int32_t value)
{
    return getAndAdd(value);
}

int32_t AtomicInteger::operator -= (int32_t value)
{
    return getAndSubtract(value);
}

AtomicInteger::operator int32_t () const OMAF_VOLATILE
{
    return mValue;
}

OMAF_NS_END
