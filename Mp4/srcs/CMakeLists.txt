
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

cmake_minimum_required(VERSION 3.3.2 FATAL_ERROR)

if(${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")
    set(CMAKE_ECLIPSE_VERSION "4.4" CACHE STRING "Eclipse version" FORCE)
    set(CMAKE_CXX_COMPILER_ARG1 "-std=c++11" CACHE STRING "C++ version for eclipse" FORCE)
endif()

project(mp4vr LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 11)

# picked from http://stackoverflow.com/a/3818084
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /bigobj")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
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

  # -pedantic
  # -Wctor-dtor-privacy
  # -Wformat=2
  # -Wold-style-cast
  # -Wundef
  # -Wstrict-overflow=5
  # -Wswitch-default
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")

  if(NOT NO_DEBUG_STL)
    SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")
  endif()
else()
  if (NOT IOS)
    message(FATAL_ERROR "Unsupported compiler!")
  endif()
endif()

if(DISABLE_UNCOVERED_CODE)
  message("Strip some code behind #ifdef for coverage. May cause runtime errors! For coverage analysis only!")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDISABLE_UNCOVERED_CODE=1")
endif()

if(ANDROID)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D__GXX_EXPERIMENTAL_CXX0X__ -D_FILE_OFFSET_BITS=64")
endif()

# # Path to where the executable resides under where build is called from
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get build version from git
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe"
    OUTPUT_VARIABLE GIT_DESCRIBE)

# Get build timestamp
string(TIMESTAMP BUILD_TIMESTAMP UTC)

find_package(Threads)

# Create the version number header
configure_file("${PROJECT_SOURCE_DIR}/buildinfo/buildinfo.hpp.in" "${PROJECT_BINARY_DIR}/buildinfo.hpp")
include_directories("${PROJECT_BINARY_DIR}")
include_directories(".")

add_subdirectory(common)
add_subdirectory(reader)
add_subdirectory(streamsegmenter)
add_subdirectory(jsonlib)

message(STATUS "System name       : ${CMAKE_SYSTEM_NAME}")
message(STATUS "Project Name      : ${PROJECT_NAME}")
message(STATUS "Project directory : ${PROJECT_SOURCE_DIR}")
message(STATUS "Build called from : ${PROJECT_BINARY_DIR}")
message(STATUS "Executables in    : ${EXECUTABLE_OUTPUT_PATH}")
message(STATUS "Libraries in      : ${LIBRARY_OUTPUT_DIRECTORY}")

# For Ninja cmake generator
cmake_policy(SET CMP0058 NEW)
