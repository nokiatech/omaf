
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

set(OMAFVI_SRCS omafvi.cpp commandline.cpp)
message(STATUS "OMAF VI          : ${OMAFVI_SRCS}")


set(VDDP_LIBS ${VDDP_LIBS} controller)
set(VDDP_LIBS ${VDDP_LIBS} segmenter)
set(VDDP_LIBS ${VDDP_LIBS} mp4loader)
set(VDDP_LIBS ${VDDP_LIBS} omafvdproc)
set(VDDP_LIBS ${VDDP_LIBS} concurrency)
set(VDDP_LIBS ${VDDP_LIBS} async)
set(VDDP_LIBS ${VDDP_LIBS} config)
set(VDDP_LIBS ${VDDP_LIBS} streamsegmenter_static)
set(VDDP_LIBS ${VDDP_LIBS} json)
set(VDDP_LIBS ${VDDP_LIBS} mp4vr_static)
set(VDDP_LIBS ${VDDP_LIBS} raw)
set(VDDP_LIBS ${VDDP_LIBS} common)
set(VDDP_LIBS ${VDDP_LIBS} processor)
set(VDDP_LIBS ${VDDP_LIBS} log)
set(VDDP_LIBS ${VDDP_LIBS} ${CMAKE_THREAD_LIBS_INIT})

add_executable(omafvi ${OMAFVI_SRCS})
target_link_libraries(omafvi ${VDDP_LIBS})
set_property(TARGET omafvi PROPERTY FOLDER "executables")
install (TARGETS omafvi
         RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)

		 		 