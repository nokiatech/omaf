
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

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

if(${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")
    set(CMAKE_ECLIPSE_VERSION "4.4" CACHE STRING "Eclipse version" FORCE)
    set(CMAKE_CXX_COMPILER_ARG1 "-std=c++11" CACHE STRING "C++ version for eclipse" FORCE)
endif()

project(reader LANGUAGES CXX)

set(MP4VR_FOR_LIB_NAME mp4vr_for_lib)
set(MP4VR_FOR_DLL_NAME mp4vr_for_dll)

if(IOS)
    if(${IOS_PLATFORM} STREQUAL "OS")
        set(MP4VR_LIB_NAME mp4vr_static_iphoneos)
    elseif(${IOS_PLATFORM} STREQUAL "SIMULATOR64")
        set(MP4VR_LIB_NAME mp4vr_static_iphonesimulator)
    endif()
else()
    set(MP4VR_LIB_NAME mp4vr_static)
endif()
set(MP4VR_FPIC_LIB_NAME ${MP4VR_LIB_NAME}_fpic)

set(READER_SRCS
    mp4vrfiledatatypes.cpp
    mp4vrfilereaderaccessors.cpp
    mp4vrfilereaderimpl.cpp
    mp4vrfilereaderutil.cpp
    mp4vrfilestreamfile.cpp
    mp4vrfilestreamgeneric.cpp
    mp4vrfilestreaminterface.cpp
    mp4vrfilestreaminternal.cpp
    mp4vrfilesegment.cpp
    mp4vrfilereaderimpl.cpp
    mp4vrhvc2extractor.cpp
    )

if(ANDROID)
  set(USE_LINUX_FILESTREAM 1)
else(ANDROID)
  set(USE_LINUX_FILESTREAM 0)
endif(ANDROID)

if(USE_LINUX_FILESTREAM)
  set(READER_SRCS
      ${READER_SRCS}
      mp4vrfilestreamlinux.cpp)
endif(USE_LINUX_FILESTREAM)

set(API_HDRS
    ../api/reader/mp4vrfilestreaminterface.h
    ../api/reader/mp4vrfilereaderinterface.h
    ../api/reader/mp4vrfiledatatypes.h
    ../api/reader/mp4vrfileexport.h
    )

set(READER_HDRS
    mp4vrfiledatatypesinternal.hpp
    mp4vrfilereaderimpl.hpp
    mp4vrfilereaderutil.hpp
    mp4vrfilereaderutil.hpp
    mp4vrfilesegment.hpp
    mp4vrfilestreamfile.hpp
    mp4vrfilestreamgeneric.hpp
    mp4vrfilestreaminternal.hpp
    )

macro(split_debug_info target)
  # objcopy doesn't work on MacOSX, so exclude that
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
      # Use MATCHES for Clang to also include AppleClang.. though it's not really needed as it's excluded.
      add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:${target}> $<TARGET_FILE:${target}>.debug
        COMMAND ${CMAKE_OBJCOPY} --strip-debug $<TARGET_FILE:${target}>
        COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=$<TARGET_FILE:${target}>.debug $<TARGET_FILE:${target}>
    )
    endif()
  endif()
endmacro()

add_library(${MP4VR_FOR_LIB_NAME} OBJECT ${READER_SRCS} ${API_HDRS} ${READER_HDRS})
set_property(TARGET ${MP4VR_FOR_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${MP4VR_FOR_LIB_NAME} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${MP4VR_FOR_LIB_NAME} PRIVATE "MP4VR_BUILDING_LIB")
target_compile_definitions(${MP4VR_FOR_LIB_NAME} PUBLIC "MP4VR_USE_STATIC_LIB")
target_compile_definitions(${MP4VR_FOR_LIB_NAME} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${MP4VR_FOR_LIB_NAME} PUBLIC "_LARGEFILE64_SOURCE")
if(USE_LINUX_FILESTREAM)
  target_compile_definitions(${MP4VR_FOR_LIB_NAME} PRIVATE "MP4VR_USE_LINUX_FILESTREAM")
endif()
target_include_directories(${MP4VR_FOR_LIB_NAME} PRIVATE ../common)
if (IOS)
    set_xcode_property(${MP4VR_FOR_LIB_NAME} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)

add_library(${MP4VR_FOR_DLL_NAME} OBJECT ${READER_SRCS} ${API_HDRS} ${READER_HDRS})
set_property(TARGET ${MP4VR_FOR_DLL_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${MP4VR_FOR_DLL_NAME} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${MP4VR_FOR_DLL_NAME} PRIVATE "MP4VR_BUILDING_LIB")
target_compile_definitions(${MP4VR_FOR_DLL_NAME} PUBLIC "MP4VR_USE_STATIC_LIB")
target_compile_definitions(${MP4VR_FOR_DLL_NAME} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${MP4VR_FOR_DLL_NAME} PUBLIC "_LARGEFILE64_SOURCE")
if(USE_LINUX_FILESTREAM)
  target_compile_definitions(${MP4VR_FOR_DLL_NAME} PRIVATE "MP4VR_USE_LINUX_FILESTREAM")
endif()
target_include_directories(${MP4VR_FOR_DLL_NAME} PRIVATE ../common)
if (IOS)
    set_xcode_property(${MP4VR_FOR_DLL_NAME} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)

add_library(${MP4VR_LIB_NAME} STATIC ${READER_SRCS} ${API_HDRS} ${READER_HDRS} $<TARGET_OBJECTS:common_for_lib> )
set_property(TARGET ${MP4VR_LIB_NAME} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${MP4VR_LIB_NAME} PRIVATE "MP4VR_BUILDING_LIB")
target_compile_definitions(${MP4VR_LIB_NAME} PUBLIC "MP4VR_USE_STATIC_LIB")
target_compile_definitions(${MP4VR_LIB_NAME} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${MP4VR_LIB_NAME} PUBLIC "_LARGEFILE64_SOURCE")
if(USE_LINUX_FILESTREAM)
  target_compile_definitions(${MP4VR_LIB_NAME} PRIVATE "MP4VR_USE_LINUX_FILESTREAM")
endif()
target_include_directories(${MP4VR_LIB_NAME} PRIVATE ../common)
if (IOS)
    set_xcode_property(${MP4VR_LIB_NAME} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)

add_library(${MP4VR_FPIC_LIB_NAME} STATIC ${READER_SRCS} ${API_HDRS} ${READER_HDRS} $<TARGET_OBJECTS:common_for_lib> )
set_property(TARGET ${MP4VR_FPIC_LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${MP4VR_FPIC_LIB_NAME} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${MP4VR_FPIC_LIB_NAME} PRIVATE "MP4VR_BUILDING_LIB")
target_compile_definitions(${MP4VR_FPIC_LIB_NAME} PUBLIC "MP4VR_USE_STATIC_LIB")
target_compile_definitions(${MP4VR_FPIC_LIB_NAME} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${MP4VR_FPIC_LIB_NAME} PUBLIC "_LARGEFILE64_SOURCE")
if(USE_LINUX_FILESTREAM)
  target_compile_definitions(${MP4VR_FPIC_LIB_NAME} PRIVATE "MP4VR_USE_LINUX_FILESTREAM")
endif()
target_include_directories(${MP4VR_FPIC_LIB_NAME} PRIVATE ../common)
if (IOS)
    set_xcode_property(${MP4VR_FPIC_LIB_NAME} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)

if(NOT IOS)
    set(MP4VR_LIB_NAME mp4vr_shared)
    add_library(${MP4VR_LIB_NAME} SHARED ${READER_SRCS} ${API_HDRS} ${READER_HDRS} $<TARGET_OBJECTS:common_for_dll> )
    if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
      split_debug_info(${MP4VR_LIB_NAME})
    endif()
    target_compile_definitions(${MP4VR_LIB_NAME} PRIVATE "MP4VR_BUILDING_DLL")
    target_compile_definitions(${MP4VR_LIB_NAME} PRIVATE "MP4VR_BUILDING_LIB")
    target_compile_definitions(${MP4VR_LIB_NAME} PUBLIC "_FILE_OFFSET_BITS=64")
    target_compile_definitions(${MP4VR_LIB_NAME} PUBLIC "_LARGEFILE64_SOURCE")
    if(USE_LINUX_FILESTREAM)
      target_compile_definitions(${MP4VR_LIB_NAME} PRIVATE "MP4VR_USE_LINUX_FILESTREAM")
    endif()
    set_property(TARGET ${MP4VR_LIB_NAME} PROPERTY CXX_STANDARD 11)
    target_include_directories(${MP4VR_LIB_NAME} PRIVATE ../common)
endif(NOT IOS)
