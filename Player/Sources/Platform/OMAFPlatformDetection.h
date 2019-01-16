
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

#if defined(__ANDROID__) // Android

    #define OMAF_PLATFORM_ANDROID 1

    #define OMAF_CPU_ARM 1 // TODO: Support for x86, x86-64 and MIPS

#elif defined(__gnu_linux__) // Linux

    #define OMAF_PLATFORM_LINUX 1

    #if defined(__x86_64__)

        #define OMAF_CPU_X86 1
        #define OMAF_CPU_X86_64 1

    #else

        #define OMAF_CPU_X86 1
        #define OMAF_CPU_X86_32 1

    #endif

#elif defined(_WIN64)

#ifndef NOMINMAX
    #define NOMINMAX
#endif
    #include <winapifamily.h>

#if WINAPI_PARTITION_DESKTOP
    #define OMAF_PLATFORM_WINDOWS 1
    #define OMAF_PLATFORM_WINDOWS_64 1
#elif WINAPI_PARTITION_APP

    #define OMAF_PLATFORM_UWP 1
    #define OMAF_PLATFORM_WINDOWS_64 1

#elif
    #error Unknown WINAPI_PARTITION
#endif

#if _M_ARM 
    #define OMAF_CPU_ARM 1
    #define OMAF_CPU_ARM_64 1
#else
    #define OMAF_CPU_X86 1
    #define OMAF_CPU_X86_64 1
#endif

#elif defined(_WIN32)

#ifndef NOMINMAX
    #define NOMINMAX
#endif
    #include <winapifamily.h>

#if WINAPI_PARTITION_DESKTOP
    #define OMAF_PLATFORM_WINDOWS 1
    #define OMAF_PLATFORM_WINDOWS_32 1
#elif WINAPI_PARTITION_APP

    #define OMAF_PLATFORM_UWP 1
    #define OMAF_PLATFORM_WINDOWS_32 1
#elif
    #error Unknown WINAPI_PARTITION
#endif

#if _M_ARM 
    #define OMAF_CPU_ARM 1
#else
    #define OMAF_CPU_X86 1
    #define OMAF_CPU_X86_32 1
#endif

#endif
