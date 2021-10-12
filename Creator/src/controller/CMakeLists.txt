
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


set_sources(CONTROLLER_SRCS)

message(STATUS "controller files  : ${CONTROLLER_SRCS}")

make_library(controller ${CONTROLLER_SRCS})

target_compile_definitions(controller PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(controller PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(controller PRIVATE "_LARGEFILE64_SOURCE")
target_compile_definitions(controller-object PRIVATE "MP4VR_USE_STATIC_LIB")
target_compile_definitions(controller-object PRIVATE "_FILE_OFFSET_BITS=64")
target_compile_definitions(controller-object PRIVATE "_LARGEFILE64_SOURCE")
target_link_libraries(controller mp4vr_static_fpic)
