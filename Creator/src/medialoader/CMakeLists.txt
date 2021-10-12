
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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set_sources(MEDIALOADER_SRCS)

message(STATUS "medialoader files : ${MEDIALOADER_SRCS}")

make_library(medialoader ${MEDIALOADER_SRCS})
set_property(TARGET medialoader PROPERTY CXX_STANDARD 11)
target_compile_definitions(medialoader PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(medialoader PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(medialoader PRIVATE "_LARGEFILE64_SOURCE")
target_compile_definitions(medialoader-object PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(medialoader-object PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(medialoader-object PRIVATE "_LARGEFILE64_SOURCE")
target_link_libraries(medialoader mp4vr_static_fpic)
