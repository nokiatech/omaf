
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

set(STREAMSEGMENTER_EXE streamsegmenter)
set(STREAMSEGMENTER_LIB streamsegmenter_static)
set(STREAMSEGMENTER_FOR_LIB streamsegmenter_for_lib)
set(STREAMSEGMENTER_FOR_DLL streamsegmenter_for_dll)
set(STREAMSEGMENTER_FPIC_LIB ${STREAMSEGMENTER_LIB}_fpic)

include_directories(${PROJECT_SOURCE_DIR}/common)

set(STREAMSEGMENTER_SRCS
  autosegmenter.cpp
  debug.cpp
  segmenterapi.cpp
  segmenter.cpp
  utils.cpp
  mp4access.cpp
  block.cpp
  track.cpp
  decodeptsutils.cpp
  memistreambuf.cpp
  frame.cpp
  frameproxy.cpp
  mpdtree.cpp
  config.cpp
  exceptions.cpp
  )

set(STREAMSEGMENTER_HDRS
  ../api/streamsegmenter/segmenterapi.hpp
  ../api/streamsegmenter/track.hpp
  ../api/streamsegmenter/optional.hpp
  ../api/streamsegmenter/frame.hpp
  ../api/streamsegmenter/frameproxy.hpp
  ../api/streamsegmenter/mpdtree.hpp
  ../api/streamsegmenter/exceptions.hpp
  ../api/streamsegmenter/autosegmenter.hpp
  debug.hpp
  utils.hpp
  mp4access.hpp
  block.hpp
  decodeptsutils.hpp
  memistreambuf.hpp
  config.hpp
  segmenter.hpp
  )

include_directories(
  ${PROJECT_SOURCE_DIR}/jsonlib
  ${PROJECT_SOURCE_DIR}
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

add_library(${STREAMSEGMENTER_FOR_LIB} OBJECT ${STREAMSEGMENTER_SRCS} ${STREAMSEGMENTER_HDRS})
set_property(TARGET ${STREAMSEGMENTER_FOR_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${STREAMSEGMENTER_FOR_LIB} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${STREAMSEGMENTER_FOR_LIB} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${STREAMSEGMENTER_FOR_LIB} PUBLIC "_LARGEFILE64_SOURCE")
target_compile_definitions(${STREAMSEGMENTER_FOR_LIB} PRIVATE "MP4VR_USE_STATIC_LIB")

add_library(${STREAMSEGMENTER_FOR_DLL} OBJECT ${STREAMSEGMENTER_SRCS} ${STREAMSEGMENTER_HDRS})
set_property(TARGET ${STREAMSEGMENTER_FOR_DLL} PROPERTY POSITION_INDEPENDENT_CODE 1)
set_property(TARGET ${STREAMSEGMENTER_FOR_DLL} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${STREAMSEGMENTER_FOR_DLL} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${STREAMSEGMENTER_FOR_DLL} PUBLIC "_LARGEFILE64_SOURCE")
target_compile_definitions(${STREAMSEGMENTER_FOR_DLL} PRIVATE "MP4VR_USE_STATIC_LIB")

add_library(${STREAMSEGMENTER_LIB} STATIC ${STREAMSEGMENTER_SRCS} ${STREAMSEGMENTER_HDRS} $<TARGET_OBJECTS:common_for_lib>)
set_property(TARGET ${STREAMSEGMENTER_LIB} PROPERTY CXX_STANDARD 11)
target_compile_definitions(${STREAMSEGMENTER_LIB} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${STREAMSEGMENTER_LIB} PUBLIC "_LARGEFILE64_SOURCE")
# should use instead target_link_libraries?
target_compile_definitions(${STREAMSEGMENTER_LIB} PRIVATE "MP4VR_USE_STATIC_LIB")
if (IOS)
    set_xcode_property(${STREAMSEGMENTER_LIB} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  split_debug_info(${STREAMSEGMENTER_LIB})
endif()

add_library(${STREAMSEGMENTER_FPIC_LIB} STATIC ${STREAMSEGMENTER_SRCS} ${STREAMSEGMENTER_HDRS} $<TARGET_OBJECTS:common_for_lib>)
set_property(TARGET ${STREAMSEGMENTER_FPIC_LIB} PROPERTY CXX_STANDARD 11)
set_property(TARGET ${STREAMSEGMENTER_FPIC_LIB} PROPERTY POSITION_INDEPENDENT_CODE 1)
target_compile_definitions(${STREAMSEGMENTER_FPIC_LIB} PUBLIC "_FILE_OFFSET_BITS=64")
target_compile_definitions(${STREAMSEGMENTER_FPIC_LIB} PUBLIC "_LARGEFILE64_SOURCE")
if (IOS)
    set_xcode_property(${STREAMSEGMENTER_FPIC_LIB} IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)
if (CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
  split_debug_info(${STREAMSEGMENTER_FPIC_LIB})
endif()

if (NOT (ANDROID OR IOS))
  add_executable(${STREAMSEGMENTER_EXE} tool.cpp)
  set_property(TARGET ${STREAMSEGMENTER_EXE} PROPERTY CXX_STANDARD 11)
  target_compile_definitions(${STREAMSEGMENTER_EXE} PUBLIC "_FILE_OFFSET_BITS=64")
  target_compile_definitions(${STREAMSEGMENTER_EXE} PUBLIC "_LARGEFILE64_SOURCE")

  target_link_libraries(${STREAMSEGMENTER_EXE} mp4vr_static json ${STREAMSEGMENTER_LIB})
endif()

