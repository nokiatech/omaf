
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

project(OmafCreator LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_CONFIGURATION_TYPES Debug Release)
set(CMAKE_EXT_LIBRARY_DIRECTORY ${CMAKE_SOURCE_DIR}/../extlibs)

if ("${CMAKE_BUILD_TYPE}" STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_VERBOSE_MAKEFILE ON)

# picked from http://stackoverflow.com/a/3818084
if(MSVC)
  # Force to always compile with W4
  set_property(GLOBAL PROPERTY USE_FOLDERS ON)
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Use MATCHES for Clang to also include AppleClang
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -W -Wall -Wall -Wextra -Wcast-align")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wcast-qual -Wdisabled-optimization")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Winit-self")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wmissing-declarations -Wmissing-include-dirs")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wredundant-decls -Wshadow -Wsign-conversion")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wsign-promo")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wlogical-op")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnoexcept")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wstrict-null-sentinel")
  endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-missing-braces")
  endif()

  # -pedantic
  # -Wctor-dtor-privacy
  # -Wformat=2
  # -Wold-style-cast
  # -Wundef
  # -Wstrict-overflow=5
  # -Wswitch-default
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")

  if(NOT NO_DEBUG_STL)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
  endif()

  # enable debug features 
  SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")
else()
    message(FATAL_ERROR "Unsupported compiler!")
endif()

if(ASAN)
  message("Enabling Clang address sanitizer for tracking memory issues")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=leak -fno-omit-frame-pointer")
endif(ASAN)

# Get build version from git
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe"
    OUTPUT_VARIABLE OMAF_CREATOR_VERSION
    RETURN_VALUE GIT_RES)

if (NOT GIT_RES EQUAL 0)
    message(WARNING "Could not get git info, is git in path, no tags? ${GIT_RES}")
	#TODO
    set(OMAF_CREATOR_VERSION "v1.0.0")
endif()

# Get build timestamp
string(TIMESTAMP BUILD_TIMESTAMP UTC)

macro (setup_package_version_variables _packageName)
    if (DEFINED ${_packageName}_VERSION)
        string (REGEX MATCHALL "[0-9]+" _versionComponents "${${_packageName}_VERSION}")
        list (LENGTH _versionComponents _len)
        if (${_len} GREATER 0)
            list(GET _versionComponents 0 ${_packageName}_VERSION_MAJOR)
        endif()
        if (${_len} GREATER 1)
            list(GET _versionComponents 1 ${_packageName}_VERSION_MINOR)
        endif()
        if (${_len} GREATER 2)
            list(GET _versionComponents 2 ${_packageName}_VERSION_REVISION)
        endif()
        set (${_packageName}_VERSION_COUNT ${_len})
    else()
        set (${_packageName}_VERSION_COUNT 0)
        set (${_packageName}_VERSION "")
    endif()
endmacro()

setup_package_version_variables(OMAF_CREATOR)

macro (set_sources SRCS)
    file (GLOB ${SRCS}
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/[^.]*.cpp
    ${ARGN})
endmacro()

macro(make_library name)
    add_library(${name} STATIC ${ARGN})
    add_library(${name}-object OBJECT ${ARGN})
    set_property(TARGET ${name}-object PROPERTY POSITION_INDEPENDENT_CODE 1)
    set_target_properties (${name}-object PROPERTIES FOLDER objectlibs)
endmacro()

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

if(CMAKE_USE_PTHREADS_INIT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif(CMAKE_USE_PTHREADS_INIT)

include_directories(.)

# Create the version number header
configure_file("${PROJECT_SOURCE_DIR}/buildinfo/buildinfo.h.in" "${PROJECT_BINARY_DIR}/buildinfo.h")
include_directories("${PROJECT_BINARY_DIR}")

#TODO
include_directories("../../Mp4/srcs/api")
include_directories("../../Mp4/srcs/api/reader")

# heif includes and libs
add_library(heif_static STATIC IMPORTED GLOBAL)
set(HEIF_LIB_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../Lib/${CMAKE_SYSTEM_NAME}/Release")
set(HEIF_LIB_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../Lib/${CMAKE_SYSTEM_NAME}/Debug")
set(HEIF_INCLUDE_APICPP "${CMAKE_CURRENT_SOURCE_DIR}/../../../heif/srcs/api-cpp")
set(HEIF_INCLUDE_API "${CMAKE_CURRENT_SOURCE_DIR}/../../../heif/srcs/api")

if(MSVC)
    if (MSVC_VERSION EQUAL 1800)
        set(VSVERSION VS2013)
    elseif (MSVC_VERSION EQUAL 1900)
        set(VSVERSION VS2015)
    elseif (MSVC_VERSION EQUAL 1910)
        set(VSVERSION VS2017)
    elseif (MSVC_VERSION EQUAL 1911)
        set(VSVERSION VS2017)
    elseif (MSVC_VERSION EQUAL 1914)
        set(VSVERSION VS2017)
    else()
        message(FATAL_ERROR "Unsupported visual studio version!" ${MSVC_VERSION})
    endif()
    
    set(HEIF_LIB_RELEASE "${HEIF_LIB_RELEASE}/${VSVERSION}")
    set(HEIF_LIB_DEBUG "${HEIF_LIB_DEBUG}/${VSVERSION}")
endif()

set_target_properties(heif_static PROPERTIES IMPORTED_LOCATION_DEBUG "${HEIF_LIB_DEBUG}/${CMAKE_STATIC_LIBRARY_PREFIX}heif_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(heif_static PROPERTIES IMPORTED_LOCATION_RELEASE "${HEIF_LIB_RELEASE}/${CMAKE_STATIC_LIBRARY_PREFIX}heif_static${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(heif_writer_static STATIC IMPORTED GLOBAL)
set_target_properties(heif_writer_static PROPERTIES IMPORTED_LOCATION_DEBUG "${HEIF_LIB_DEBUG}/${CMAKE_STATIC_LIBRARY_PREFIX}heif_writer_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(heif_writer_static PROPERTIES IMPORTED_LOCATION_RELEASE "${HEIF_LIB_RELEASE}/${CMAKE_STATIC_LIBRARY_PREFIX}heif_writer_static${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(heifpp_static STATIC IMPORTED GLOBAL)
set_target_properties(heifpp_static PROPERTIES IMPORTED_LOCATION_DEBUG "${HEIF_LIB_DEBUG}/${CMAKE_STATIC_LIBRARY_PREFIX}heifpp${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(heifpp_static PROPERTIES IMPORTED_LOCATION_RELEASE "${HEIF_LIB_RELEASE}/${CMAKE_STATIC_LIBRARY_PREFIX}heifpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

message("heifpp_static PROPERTIES IMPORTED_LOCATION_RELEASE ${HEIF_LIB_RELEASE}/${CMAKE_STATIC_LIBRARY_PREFIX}heifpp${CMAKE_STATIC_LIBRARY_SUFFIX}")

include_directories("${HEIF_INCLUDE_APICPP}")
include_directories("${HEIF_INCLUDE_API}/common")
include_directories("${HEIF_INCLUDE_API}/reader")
include_directories("${HEIF_INCLUDE_API}/writer")

add_library(mp4vr_static STATIC IMPORTED GLOBAL)
set_target_properties(mp4vr_static PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(mp4vr_static PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(mp4vr_static PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(mp4vr_static_fpic STATIC IMPORTED GLOBAL)
set_target_properties(mp4vr_static_fpic PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(mp4vr_static_fpic PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(mp4vr_static_fpic PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}mp4vr_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(streamsegmenter_static STATIC IMPORTED GLOBAL)
set_target_properties(streamsegmenter_static PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(streamsegmenter_static PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(streamsegmenter_static PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static${CMAKE_STATIC_LIBRARY_SUFFIX}")

add_library(streamsegmenter_static_fpic STATIC IMPORTED GLOBAL)
set_target_properties(streamsegmenter_static_fpic PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(streamsegmenter_static_fpic PROPERTIES IMPORTED_LOCATION_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")
set_target_properties(streamsegmenter_static_fpic PROPERTIES IMPORTED_LOCATION_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/../../Mp4/lib/${CMAKE_SYSTEM_NAME}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}streamsegmenter_static_fpic${CMAKE_STATIC_LIBRARY_SUFFIX}")

message(STATUS "System name       : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project Name      : ${PROJECT_NAME}")
message(STATUS "Project directory : ${PROJECT_SOURCE_DIR}")
message(STATUS "Project version   : ${OMAF_CREATOR_VERSION}")
message(STATUS "Build called from : ${PROJECT_BINARY_DIR}")
message(STATUS "Executables in    : ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Libraries in      : ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
message(STATUS "cxx Flags         : ${CMAKE_CXX_FLAGS}")
message(STATUS "BUILD TYPE        : ${CMAKE_BUILD_TYPE}")

add_subdirectory(jsonlib)
add_subdirectory(async)
add_subdirectory(common)
add_subdirectory(controller)
add_subdirectory(config)
add_subdirectory(concurrency)
add_subdirectory(tool)
add_subdirectory(processor)
add_subdirectory(mp4loader)
add_subdirectory(segmenter)
add_subdirectory(omaf)
add_subdirectory(raw)
add_subdirectory(log)


# For Ninja cmake generator
cmake_policy(SET CMP0058 NEW)
