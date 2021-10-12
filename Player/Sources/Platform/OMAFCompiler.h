
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

#include "Platform/OMAFPlatformDetection.h"

namespace OMAF
{
// http://sourceforge.net/p/predef/wiki/Compilers/

// Detect compiler
#if defined(_MSC_VER)

#if (_MSC_FULL_VER < 150030729)

#error Requires at least VS2008 with SP1 applied to compile. Please update your compiler.

#endif

#define OMAF_COMPILER_CL 1

#elif defined(__llvm__)

#define OMAF_COMPILER_LLVM 1

#elif defined(__GNUC__)

#define OMAF_COMPILER_GCC 1

#else

#error Unsupported compiler

#endif


#if OMAF_COMPILER_CL

#define OMAF_INLINE inline

#define OMAF_FUNCTION __FUNCSIG__

#define OMAF_ALIGN_OF _alignof

#define OMAF_UNUSED

#elif OMAF_COMPILER_GCC

#if __GNUC__ < 4

// GCC 3 support for "always_inline" is somewhat bugged, so fall back to just "inline"
#define OMAF_INLINE inline

#else

#define OMAF_INLINE inline __attribute__((always_inline))

#endif

#define OMAF_FUNCTION __PRETTY_FUNCTION__

#define OMAF_ALIGN_OF __alignof__

#define OMAF_UNUSED __attribute__((unused))

#elif OMAF_COMPILER_LLVM

#define OMAF_INLINE inline

#define OMAF_FUNCTION __PRETTY_FUNCTION__

#define OMAF_ALIGN_OF __alignof__

#define OMAF_UNUSED __attribute__((unused))

#else

#error Unsupported compiler

#endif

#define OMAF_VOLATILE volatile

#define OMAF_FILE __FILE__
#define OMAF_LINE __LINE__
#define OMAF_DATE __DATE__
#define OMAF_TIME __TIME__

#define OMAF_UNUSED_VARIABLE(variable) (void_t)(variable)
#define OMAF_EXPLICIT explicit
#define OMAF_DEFAULT_ALIGN sizeof(void*)
#define OMAF_SIZE_OF(x) sizeof(x)
#define OMAF_ARRAY_SIZE(x) (OMAF_SIZE_OF(x) / OMAF_SIZE_OF(x[0]))

#if OMAF_COMPILER_CL

#ifdef _MANAGED

#define OMAF_ISSUE_BREAK() System::Diagnostics::Debugger::Break()

#else

#define OMAF_ISSUE_BREAK() __debugbreak()

#endif

#elif OMAF_COMPILER_GCC || OMAF_COMPILER_LLVM

#if OMAF_CPU_X86_32

#define OMAF_ISSUE_BREAK() __asm__("int $3")

#elif OMAF_CPU_X86_64

#define OMAF_ISSUE_BREAK() __asm {int 3}  // __asm__ ("svc 0")

#elif OMAF_CPU_ARM

#define OMAF_ISSUE_BREAK() __builtin_trap()  // asm ("trap")

#else

#error Error unknown CPU family

#endif

#else

#define OMAF_ISSUE_BREAK() __asm__("int $3")

#endif
}  // namespace OMAF
