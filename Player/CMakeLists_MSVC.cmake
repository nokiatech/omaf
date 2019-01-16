
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

set(TARGET_OS_WINDOWS "windows")
set(OMAF_CMAKE_TARGET_OS ${TARGET_OS_WINDOWS})
#the default is x86...?
set(OMAF_CMAKE_PLATFORM x86)
string(FIND ${CMAKE_GENERATOR} ARM64 isArm64)
string(FIND ${CMAKE_GENERATOR} ARM isArm)
string(FIND ${CMAKE_GENERATOR} Win64 isWin64)
string(FIND ${CMAKE_GENERATOR} Win32 isWin32)

if (NOT isArm64 EQUAL -1)
	set(OMAF_CMAKE_PLATFORM ARM64)
elseif (NOT isArm EQUAL -1)
	set(OMAF_CMAKE_PLATFORM ARM)
elseif (NOT isWin64 EQUAL -1)
	set(OMAF_CMAKE_PLATFORM x64)
elseif (NOT isWin32 EQUAL -1)
	set(OMAF_CMAKE_PLATFORM x86)
endif()

add_definitions(-D_CRT_SECURE_NO_WARNINGS -DTARGET_OS_WINDOWS)


if (MSVC_VERSION EQUAL 1900)
    set(VSVERSION VS2015)
elseif (MSVC_VERSION GREATER 1900)
    set(VSVERSION VS2017)
else()
    message(FATAL_ERROR "Unsupported visual studio version!" ${MSVC_VERSION})
endif()

#make sure that we dont link release crt in debug builds... this should be removed, and all debug/release conflicts erased!
set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:msvcrt")

set(OMAF_ROOT ..)
set(OMAF_EXT_LIBPATH_WINDOWS ${OMAF_ROOT}/Lib/Windows/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>)
set(OMAF_EXT_LIBPATH_WINDOWS_VS ${OMAF_EXT_LIBPATH_WINDOWS}/${VSVERSION})


set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} ${CMAKE_SOURCE_DIR}/${OMAF_ROOT}/Mp4/lib/Windows/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>/mp4vr_shared.lib)

if (ENABLE_GRAPHICS_API_D3D11)
    set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} d3d11.lib)
    set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} dxgi.lib)
    set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} dxguid.lib)
    set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} d3dcompiler.lib)
endif()

set(OMAF_COMMON_DEPS ${OMAF_COMMON_DEPS} shcore.lib)

set(libprefix "")
set(libsuffix "lib")
set(dashsuffix "lib")
