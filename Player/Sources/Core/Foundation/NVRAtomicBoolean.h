
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
// Atomic boolean
//

class AtomicBoolean
{
    public:

        // Creates atomic boolean with initial value
        OMAF_INLINE OMAF_EXPLICIT AtomicBoolean(bool_t value);
        OMAF_INLINE ~AtomicBoolean();

        // Atomically sets the value to the given updated value if the current value equals compare value and returns true if valua was changed.
        OMAF_INLINE bool_t compareAndSet(bool_t value, bool_t compare);
    
        // Atomically sets to the given value and returns the previous value.
        OMAF_INLINE bool_t getAndSet(bool_t value);
    
        // Sets to the given value.
        OMAF_INLINE void_t set(bool_t value);
    
        // Returns the current value.
        OMAF_INLINE bool_t get();
    
    
        // Sets to the given value.
        OMAF_INLINE bool_t operator = (bool_t value);
    
        // Returns the current value.
        OMAF_INLINE operator bool_t () const OMAF_VOLATILE;
    
        // Compares if values are equals.
        OMAF_INLINE bool_t operator == (bool_t value);
    
        // Compares if values are non-equals.
        OMAF_INLINE bool_t operator != (bool_t value);

    private:

        OMAF_NO_DEFAULT(AtomicBoolean);
        OMAF_NO_ASSIGN(AtomicBoolean);

    private:

        OMAF_VOLATILE int32_t mValue;
};

OMAF_NS_END

#include "Foundation/NVRAtomicBooleanImpl.h"
