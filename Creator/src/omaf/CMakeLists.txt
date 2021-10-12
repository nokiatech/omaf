
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

add_subdirectory(parser)

set_sources(OMAF_SRCS)

message(STATUS "OMAF-specific files   : ${OMAF_SRCS}")

make_library(omaf ${OMAF_SRCS})
target_link_libraries(omaf h265parser)
