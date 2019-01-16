
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

#include "Platform/OMAFPlatformDetection.h"

#include "NVRNamespace.h"

// Common
#include <math.h>
#include <string.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include <errno.h>
#include <time.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <new>

// Platform specific
#if OMAF_PLATFORM_ANDROID

    #include <jni.h>
    #include <android/log.h>
    #include <pthread.h>
    #include <semaphore.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <sys/prctl.h>
    #include <sys/time.h>
    #include <sys/resource.h>

#elif OMAF_PLATFORM_WINDOWS

    #define WIN32_LEAN_AND_MEAN
    #define WIN32_EXTRA_LEAN
    #define VC_EXTRALEAN

    #define NOGDI

    #include <windows.h>
    #include <process.h>

    // Undefines
    #ifdef Yield
    #undef Yield
    #endif

    #ifdef DELETE
    #undef DELETE
    #endif

    #ifdef min
    #undef min
    #endif

    #ifdef max
    #undef max
    #endif

    #ifdef OPAQUE
    #undef OPAQUE // Brokes some enums in run-time
    #endif

    #ifdef MOUSE_EVENT
    #undef MOUSE_EVENT // Brokes some enums in run-time
    #endif

    #ifdef GetObject
    #undef GetObject
    #endif

    #ifdef HALT
    #undef HALT
    #endif
    // common behaviour which is just simple assign to make this work!
    #ifdef Assertion
    #undef Assertion
    #endif

#elif OMAF_PLATFORM_UWP

    #define WIN32_LEAN_AND_MEAN
    #define WIN32_EXTRA_LEAN
    #define VC_EXTRALEAN

    //- CC_*, LC_*, PC_*, CP_*, TC_*, RC_
    #define NOGDICAPMASKS
    //- VK_*
    #define NOVIRTUALKEYCODES
    //- WM_*, EM_*, LB_*, CB_*
    #define NOWINMESSAGES
    //- WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
    #define NOWINSTYLES
    //- SM_*
    #define NOSYSMETRICS
    //- MF_*
    #define NOMENUS
    //- IDI_*
    #define NOICONS
    //- MK_*
    #define NOKEYSTATES
    //- SC_*
    #define NOSYSCOMMANDS
    //- Binary and Tertiary raster ops
    #define NORASTEROPS
    //- SW_*
    #define NOSHOWWINDOW
    //- OEM Resource values
    #define OEMRESOURCE
    // - Atom Manager routines
    #define NOATOM
    // - Clipboard routines
    #define NOCLIPBOARD
    // - Screen colors
    #define NOCOLOR
    // - Control and Dialog routines
    #define NOCTLMGR
    // - DrawText() and DT_*
    #define NODRAWTEXT
    // - All GDI defines and routines
    #define NOGDI
    // - All KERNEL defines and routines
    #define NOKERNEL
    // - All USER defines and routines
    //#define NOUSER
    // - All NLS defines and routines
    //#define NONLS
    // - MB_* and MessageBox()
    #define NOMB
    //- GMEM_*, LMEM_*, GHND, LHND, associated routines
    #define NOMEMMGR
    //- typedef METAFILEPICT
    #define NOMETAFILE
    //- Macros min(a, b) and max(a, b)
    #define NOMINMAX
    //- typedef MSG and associated routines
    //#define NOMSG
    //- OpenFile(), OemToAnsi, AnsiToOem, and OF_*
    #define NOOPENFILE
    //- SB_* and scrolling routines
    #define NOSCROLL
    //- All Service Controller routines, SERVICE_ equates, etc.
    #define NOSERVICE
    //- Sound driver routines
    #define NOSOUND
    //- typedef TEXTMETRIC and associated routines
    #define NOTEXTMETRIC
    //- SetWindowsHook and WH_*
    #define NOWH
    //- GWL_*, GCL_*, associated routines
    #define NOWINOFFSETS
    //- COMM driver routines
    #define NOCOMM
    //- Kanji support stuff.
    #define NOKANJI
    //- Help engine interface.
    #define NOHELP
    //- Profiler interface.
    #define NOPROFILER
    //- DeferWindowPos routines
    #define NODEFERWINDOWPOS
    //- Modem Configuration Extensions
    #define NOMCX

    #include <windows.h>

    #else

#error Unsupported platform

#endif
