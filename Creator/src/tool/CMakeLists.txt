
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

set(OMAFVI_SRCS omafvi.cpp commandline.cpp)
message(STATUS "OMAF VI          : ${OMAFVI_SRCS}")

set(OMAFVD_SRCS omafvd.cpp commandline.cpp)
message(STATUS "OMAF VD          : ${OMAFVD_SRCS}")

set(OMAFIMAGE_SRCS ${OMAFIMAGE_SRCS} omafimage.cpp localcommandline.cpp localoptional.cpp)
message(STATUS "OMAF HEIF         : ${OMAFVD_HEIF}")

# in order of omafvd linking errors...
set(VDDP_LIBS ${VDDP_LIBS} config)
set(VDDP_LIBS ${VDDP_LIBS} mp4vr_static)
set(VDDP_LIBS ${VDDP_LIBS} common)
set(VDDP_LIBS ${VDDP_LIBS} controller)
set(VDDP_LIBS ${VDDP_LIBS} log)
set(VDDP_LIBS ${VDDP_LIBS} json)
set(VDDP_LIBS ${VDDP_LIBS} processor)
set(VDDP_LIBS ${VDDP_LIBS} omaf)
set(VDDP_LIBS ${VDDP_LIBS} segmenter)
set(VDDP_LIBS ${VDDP_LIBS} medialoader)
set(VDDP_LIBS ${VDDP_LIBS} concurrency)
set(VDDP_LIBS ${VDDP_LIBS} async)
set(VDDP_LIBS ${VDDP_LIBS} streamsegmenter_static)
set(VDDP_LIBS ${VDDP_LIBS} raw)
set(VDDP_LIBS ${VDDP_LIBS} ${CMAKE_THREAD_LIBS_INIT})

# omafvi not needed for generating omafv2 test content
#add_executable(omafvi ${OMAFVI_SRCS})
#target_link_libraries(omafvi ${VDDP_LIBS})
#set_property(TARGET omafvi PROPERTY FOLDER "executables")
#install (TARGETS omafvi
#         RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)

		 
add_executable(omafvd ${OMAFVD_SRCS})
target_link_libraries(omafvd ${VDDP_LIBS})
set_property(TARGET omafvd PROPERTY FOLDER "executables")
install (TARGETS omafvd
         RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
		 


# heif creator not needed with omafv2 release
if(BUILD_HEIF)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} streamsegmenter_static)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} common)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} mp4vr_static)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} heifpp_static)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} heif_writer_static)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} heif_static)
        set(OMAFIMAGE_LIBS ${OMAFIMAGE_LIBS} h265parser)

        add_executable(omafimage ${OMAFIMAGE_SRCS})
        target_link_libraries(omafimage ${OMAFIMAGE_LIBS})
        set_property(TARGET omafimage PROPERTY FOLDER "executables")
        target_compile_definitions(omafimage PRIVATE "HEIF_USE_STATIC_LIB")
        install (TARGETS omafimage
                RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)
endif(BUILD_HEIF)
