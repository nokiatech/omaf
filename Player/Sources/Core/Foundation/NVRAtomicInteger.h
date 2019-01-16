
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
#pragma once

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Atomic integer
//

class AtomicInteger
{
    public:

        OMAF_INLINE OMAF_EXPLICIT AtomicInteger(int32_t value);
        OMAF_INLINE ~AtomicInteger();

        // Atomically sets the value to the given updated value if the current value equals compare value and returns the previous value.
        OMAF_INLINE int32_t compareAndSet(int32_t value, int32_t compare);
    
        // Atomically sets to the given value and returns the previous value.
        OMAF_INLINE int32_t getAndSet(int32_t value);
    
        // Sets to the given value.
        OMAF_INLINE void_t set(int32_t value);
    
        // Returns the current value.
        OMAF_INLINE int32_t get();
    
        // Atomically increments by one the current value and returns the updated value.
        OMAF_INLINE int32_t incrementAndGet();
    
        // Atomically decrements by one the current value and returns the updated value.
        OMAF_INLINE int32_t decrementAndGet();

        // Atomically adds value to the current value and returns the previous value.
        OMAF_INLINE int32_t getAndAdd(int32_t value);
    
        // Atomically subtracts value from the current value and returns the previous value.
        OMAF_INLINE int32_t getAndSubtract(int32_t value);
    
    
        // Sets to the given value.
        OMAF_INLINE int32_t operator = (int32_t value);
    
        // Returns the current value.
        OMAF_INLINE operator int32_t () const OMAF_VOLATILE;

        // Atomically increments by one the current value and returns the updated value.
        OMAF_INLINE int32_t operator ++ ();
    
        // Atomically decrements by one the current value and returns the updated value.
        OMAF_INLINE int32_t operator -- ();

        // Atomically adds value to the current value and returns the previous value.
        OMAF_INLINE int32_t operator += (int32_t value);
    
        // Atomically subtracts value from the current value and returns the previous value.
        OMAF_INLINE int32_t operator -= (int32_t value);

        // Compares if values are equals.
        OMAF_INLINE bool_t operator == (int32_t value);
    
        // Compares if values are non-equals.
        OMAF_INLINE bool_t operator != (int32_t value);

    private:

        OMAF_NO_DEFAULT(AtomicInteger);
        OMAF_NO_ASSIGN(AtomicInteger);

    private:

        OMAF_VOLATILE int32_t mValue;
};

OMAF_NS_END

#include "Foundation/NVRAtomicIntegerImpl.h"
