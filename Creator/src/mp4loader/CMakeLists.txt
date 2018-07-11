
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

set_sources(MP4LOADER_SRCS)

message(STATUS "mp4loader files   : ${MP4LOADER_SRCS}")

make_library(mp4loader ${MP4LOADER_SRCS})
set_property(TARGET mp4loader PROPERTY CXX_STANDARD 11)
target_compile_definitions(mp4loader PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(mp4loader PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(mp4loader PRIVATE "_LARGEFILE64_SOURCE")
target_compile_definitions(mp4loader-object PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(mp4loader-object PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(mp4loader-object PRIVATE "_LARGEFILE64_SOURCE")
target_link_libraries(mp4loader mp4vr_static)
