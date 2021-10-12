
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

cmake_minimum_required(VERSION 3.3)
set(MODULE_NAME OMAFPlayer)

project(${MODULE_NAME} CXX)

# We are not defining any library / framework dependencies here, since cmake wont link them by default.
# We need a custom build target for that.
if (TARGET_OS_WINDOWS)
    message("Build For WINDOWS")
elseif(TARGET_OS_ANDROID)
    message("Build For ANDROID")
else()
    message(FATAL_ERROR "Unsupported OS.")
endif()

# Get build version from git
exec_program(
    "git"
    ${CMAKE_CURRENT_SOURCE_DIR}
    ARGS "describe"
    OUTPUT_VARIABLE SDK_VERSION
    RETURN_VALUE GIT_RES)

if (NOT GIT_RES EQUAL 0)
    message(WARNING "Could not get git info, is git in path, no tags? ${GIT_RES}")
    set(SDK_VERSION "v0.0.0-develop")
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

setup_package_version_variables(SDK)


# Create the version number header
configure_file("${PROJECT_SOURCE_DIR}/buildinfo.hpp.in" "${PROJECT_BINARY_DIR}/buildinfo.hpp")
message("SDK Version: ${SDK_VERSION} TimeStamp: ${BUILD_TIMESTAMP}")

add_definitions(-DOMAF_BUILD_SHARED_LIBRARY)
set(COMMONDEPS ${OMAF_COMMON_DEPS})


# Get source files and sort by name
file(GLOB_RECURSE sources
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.inc)

list(SORT sources)


# Create platform source filters
set(source_filter_patterns "Windows" "Android")

# Create audio backend source filters
set(source_filter_patterns ${source_filter_patterns} "OpenSL" "WASAPI")

# Create graphics backend feature source filters
set(source_filter_patterns ${source_filter_patterns} "OpenGL" "Null" "D3D11")


# Exclude platforms
if (TARGET_OS_WINDOWS)
    list(REMOVE_ITEM source_filter_patterns "Windows")
    list(REMOVE_ITEM source_filter_patterns "WASAPI")
elseif(TARGET_OS_ANDROID)
    list(REMOVE_ITEM source_filter_patterns "Android")
    list(REMOVE_ITEM source_filter_patterns "OpenSL")
endif()

# Exclude graphics backends
if(ENABLE_GRAPHICS_API_NIL)
    message("Renderbackend: Null enabled")
    list(REMOVE_ITEM source_filter_patterns "Null")
else()
    message("Renderbackend: Null disabled")
endif()

if(ENABLE_GRAPHICS_API_D3D11)
    message("Renderbackend: D3D11 enabled")
    list(REMOVE_ITEM source_filter_patterns "D3D11")
else()
    message("Renderbackend: D3D11 disabled")
endif()

if(ENABLE_GRAPHICS_API_OPENGL OR ENABLE_GRAPHICS_API_OPENGL_ES)
    message("Renderbackend: OpenGL/OpenGL ES enabled")
    list(REMOVE_ITEM source_filter_patterns "OpenGL")
else()
    message("Renderbackend: OpenGL/OpenGL ES disabled")
endif()



# Filter out unwanted source files based on filter patterns
foreach(FILTER_ITEM ${source_filter_patterns})
    foreach(ITEM ${sources})
        STRING(REGEX REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" ITEM ${ITEM})
        if(${ITEM} MATCHES "/${FILTER_ITEM}/*")
            list(REMOVE_ITEM sources "${CMAKE_CURRENT_SOURCE_DIR}${ITEM}")
        endif()
    endforeach()
endforeach()


# Sanity check to make sure that we have source files to compile
if(NOT sources)
  message(FATAL_ERROR, "No source files cannot be found.")
endif()

# Recursively scans given directory and create source groups according to the directory structure.
# Usage: source_group_tree(groupName path), if you don't need root path just use " " as path.
macro(source_group_tree)

    file(GLOB all_files ${ARGV0}/*)
    set(files "")

    foreach(each ${all_files})

        if(NOT IS_DIRECTORY ${each})
            get_filename_component(file_name ${each} NAME)
            list(APPEND files "${ARGV0}/${file_name}")
        endif()

    endforeach()

    list(LENGTH files files_len)

    if(${files_len} GREATER 0)
        if(${ARGV1} STREQUAL " ")
        else()
            string(REPLACE "/" "\\\\" group_name ${ARGV1})
            source_group("${group_name}" FILES ${files})
        endif()
    endif()

    file(GLOB directories ${ARGV0}/*/)

    foreach(each ${directories})

        if(IS_DIRECTORY ${each})
            get_filename_component(file_name ${each} NAME)

            if(${ARGV1} STREQUAL " ")
                source_group_tree("${ARGV0}/${file_name}" "${file_name}")
            else()
                source_group_tree("${ARGV0}/${file_name}" "${ARGV1}/${file_name}")
            endif()
        endif()

    endforeach()

endmacro(source_group_tree)


# Create source groups based in directory structure
source_group_tree(${CMAKE_CURRENT_SOURCE_DIR} " ")

set(sources ${sources} "${PROJECT_BINARY_DIR}/buildinfo.hpp")

# Add the 3rd party API
set(framework_public_headers "${CMAKE_SOURCE_DIR}/Sources/API/OMAFPlayer.h" "${CMAKE_SOURCE_DIR}/Sources/API/OMAFPlayerDataTypes.h" "${CMAKE_SOURCE_DIR}/Sources/API/OMAFPlayerPlatformParameters.h")

set(sources ${sources} ${framework_public_headers})
source_group("Public" FILES ${framework_public_headers})


add_library(${MODULE_NAME} SHARED ${sources} ${headers})
target_link_libraries(${MODULE_NAME} ${PLATDEPS} ${COMMONDEPS})
target_compile_definitions(${MODULE_NAME} PUBLIC "OMAF_BUILD_SHARED_LIBRARY")

set_property(TARGET ${MODULE_NAME} PROPERTY CXX_STANDARD 11)
target_link_libraries(${MODULE_NAME} ${PLATDEPS})
set_target_properties(${MODULE_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Core/")
target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/API/")
target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Platform/")

target_include_directories(${MODULE_NAME} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Player/")

target_include_directories(${MODULE_NAME} PRIVATE "${PROJECT_BINARY_DIR}")

target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../Mp4/srcs/api/")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../Mp4/srcs/api/reader/")

target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../../libdash/libdash/libdash/include/")

target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../../heif/srcs/api/common/")
target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../../heif/srcs/api/reader/")


if(TARGET_OS_ANDROID)

    #enable debug info for release builds too.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g")

elseif(TARGET_OS_WINDOWS)

    #enable debug info for release builds too.
    set_property(TARGET ${MODULE_NAME} APPEND PROPERTY LINK_FLAGS_RELEASE /DEBUG)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")

    target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/../OpenGLExt/")

endif()

if (TARGET_OS_WINDOWS)
	#CopyBins created by main cmakefile..
	add_custom_command(TARGET ${MODULE_NAME} POST_BUILD COMMAND "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/CopyBins.bat" $<CONFIG>)
endif()
