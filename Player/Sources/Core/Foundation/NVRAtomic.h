
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
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

// http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
// http://msdn.microsoft.com/en-us/subscriptions/26td21ds(v=vs.85).aspx
// http://llvm.org/docs/Atomics.html

//
// Cross-platform atomic operations
//

namespace Atomic
{
    #if OMAF_COMPILER_CL
    
        // Force compiler to use instrinsic versions
        #pragma intrinsic (_InterlockedIncrement)
        #pragma intrinsic (_InterlockedDecrement)
        #pragma intrinsic (_InterlockedExchangeAdd)
        #pragma intrinsic (_InterlockedExchange)
        #pragma intrinsic (_InterlockedCompareExchange)
    
    #endif
    
    //
    // 32-bit atomic operations
    //
    
    OMAF_INLINE int32_t increment(OMAF_VOLATILE int32_t* ptr)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchangeAdd((LONG volatile *)ptr, 1);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_add(ptr, 1);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int32_t decrement(OMAF_VOLATILE int32_t* ptr)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchangeAdd((LONG volatile *)ptr, -1);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_sub(ptr, 1);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int32_t add(OMAF_VOLATILE int32_t* ptr, int32_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchangeAdd((LONG volatile *)ptr, value);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_add(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int32_t subtract(OMAF_VOLATILE int32_t* ptr, int32_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchangeAdd((LONG volatile *)ptr, -value);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_sub(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int32_t exchange(OMAF_VOLATILE int32_t* ptr, int32_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchange((LONG volatile *)ptr, value);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            __sync_synchronize();

            return __sync_lock_test_and_set(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int32_t compareExchange(OMAF_VOLATILE int32_t* ptr, int32_t value, int32_t compare)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 4, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedCompareExchange((LONG volatile *)ptr, value, compare);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_val_compare_and_swap(ptr, compare, value);
        
        #else

            #error Unsupported platform

        #endif
    }
    
    //
    // 64-bit atomic operations
    //
    
    OMAF_INLINE int64_t increment(OMAF_VOLATILE int64_t* ptr)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL
        
            return _InterlockedExchangeAdd64((LONGLONG volatile *)ptr, 1);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_add(ptr, 1);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int64_t decrement(OMAF_VOLATILE int64_t* ptr)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL
        
            return _InterlockedExchangeAdd64((LONGLONG volatile *)ptr, -1);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_sub(ptr, 1);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int64_t add(OMAF_VOLATILE int64_t* ptr, int64_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL
        
            return _InterlockedExchangeAdd64((LONGLONG volatile *)ptr, value);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_add(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int64_t subtract(OMAF_VOLATILE int64_t* ptr, int64_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL
        
            return _InterlockedExchangeAdd64((LONGLONG volatile *)ptr, -value);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_fetch_and_sub(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int64_t exchange(OMAF_VOLATILE int64_t* ptr, int64_t value)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL

            return _InterlockedExchange64((LONGLONG volatile *)ptr, value);
        
        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            __sync_synchronize();

            return __sync_lock_test_and_set(ptr, value);

        #else

            #error Unsupported platform

        #endif
    }
    
    OMAF_INLINE int64_t compareExchange(OMAF_VOLATILE int64_t* ptr, int64_t value, int64_t compare)
    {
        OMAF_ASSERT_ALIGNMENT(ptr, 8, 0);
        
        #if OMAF_COMPILER_CL
        
            return _InterlockedCompareExchange64((LONGLONG volatile *)ptr, value, compare);

        #elif OMAF_COMPILER_LLVM || OMAF_COMPILER_GCC

            return __sync_val_compare_and_swap(ptr, compare, value);

        #else

            #error Unsupported platform

        #endif
    }
}

OMAF_NS_END
