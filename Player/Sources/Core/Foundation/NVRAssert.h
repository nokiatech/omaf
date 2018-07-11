
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
#pragma once

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFPlatformDetection.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRBuild.h"
#include "Foundation/NVRAlignment.h"

#if OMAF_DEBUG_BUILD

    #define OMAF_ENABLE_ASSERTIONS 1
#else

    #define OMAF_ENABLE_ASSERTIONS 0

#endif

#if OMAF_ENABLE_ASSERTIONS

    #define OMAF_ASSERT(condition, msg, ...)                     do { if(!(condition)) if (OMAF_NS::Assertion::reportFailure(#condition, OMAF_FILE, OMAF_LINE, msg, ##__VA_ARGS__) == OMAF_NS::Assertion::BreakType::HALT) OMAF_ISSUE_BREAK(); } while(0)

    #define OMAF_ASSERT_NOT_NULL(ptr)                            OMAF_ASSERT(ptr != OMAF_NULL, "Pointer is null")
    #define OMAF_ASSERT_UNREACHABLE()                            OMAF_ASSERT(false, "This code should never be reached")
    #define OMAF_ASSERT_NOT_IMPLEMENTED()                        OMAF_ASSERT(false, "Implementation is missing")
    #define OMAF_ASSERT_ALIGNMENT(argument, alignment, offset)   OMAF_ASSERT(OMAF_NS::isAligned(argument, alignment, offset), "Argument \"" #argument "\" is not properly aligned")

#else

    #define OMAF_ASSERT(condition, msg, ...)

    #define OMAF_ASSERT_NOT_NULL(ptr)
    #define OMAF_ASSERT_UNREACHABLE()
    #define OMAF_ASSERT_NOT_IMPLEMENTED()
    #define OMAF_ASSERT_ALIGNMENT(argument, alignment, offset)

#endif

#define OMAF_COMPILE_ASSERT(condition) OMAF_NS::StaticAssert<condition>::assertion();

OMAF_NS_BEGIN

// Compile asserts, these should be always enabled
template <bool_t b>
struct StaticAssert 
{
};

template <>
struct StaticAssert<true>
{
    static void assertion()
    {
    }
};

namespace Assertion
{
    struct BreakType
    {
        enum Enum
        {
            INVALID = -1,
            
            HALT,
            CONTINUE,
            
            COUNT
        };
    };
    
    typedef BreakType::Enum (*Handler)(const char_t* condition, const char_t* msg, const char_t* file, int32_t line);
    
    Handler getHandler();
    void_t setHandler(Handler handler);
    
    BreakType::Enum reportFailure(const char_t* condition, const char_t* file, int32_t line, const char_t* msg, ...);
}

OMAF_NS_END
