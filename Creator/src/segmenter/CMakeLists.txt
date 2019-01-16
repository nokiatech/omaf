
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

cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set_sources(SEGMENTER_SRCS)

message(STATUS "segmenter files   : ${SEGMENTER_SRCS}")

make_library(segmenter ${SEGMENTER_SRCS})
