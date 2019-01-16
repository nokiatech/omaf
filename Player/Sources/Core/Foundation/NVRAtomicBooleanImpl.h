
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
#include "Foundation/NVRAtomic.h"

OMAF_NS_BEGIN

AtomicBoolean::AtomicBoolean(bool_t value)
: mValue((int32_t)value)
{
}

AtomicBoolean::~AtomicBoolean()
{
}

bool_t AtomicBoolean::compareAndSet(bool_t value, bool_t compare)
{
    int32_t old = Atomic::compareExchange(&mValue, (int32_t)value, (int32_t)compare);
    
    return (old == (int32_t)compare);
}

bool_t AtomicBoolean::getAndSet(bool_t value)
{
    return (Atomic::exchange(&mValue, (int32_t)value) == 1);
}

void_t AtomicBoolean::set(bool_t value)
{
    Atomic::exchange(&mValue, (int32_t)value);
}

bool_t AtomicBoolean::get()
{
    return (mValue == 1);
}

bool_t AtomicBoolean::operator = (bool_t value)
{
    return getAndSet(value);
}

bool_t AtomicBoolean::operator == (bool_t value)
{
    return (mValue == (int32_t)value);
}

bool_t AtomicBoolean::operator != (bool_t value)
{
    return (mValue != (int32_t)value);
}

AtomicBoolean::operator bool_t () const OMAF_VOLATILE
{
    return (mValue == 1);
}

OMAF_NS_END
