
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

cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# OMAF player and dependency libs
configure_file(${OMAF_PLAYER_LIBS}/${ANDROID_ABI}/libOMAFPlayer.so ${JNILIBS}/${ANDROID_ABI}/libOMAFPlayer.so COPYONLY)
configure_file(${OMAF_PLAYER_LIBS}/${ANDROID_ABI}/libmp4vr_shared.so ${JNILIBS}/${ANDROID_ABI}/libmp4vr_shared.so COPYONLY)
configure_file(${OMAF_PLAYER_LIBS}/${ANDROID_ABI}/libheif_shared.so ${JNILIBS}/${ANDROID_ABI}/libheif_shared.so COPYONLY)
configure_file(${OMAF_PLAYER_LIBS}/${ANDROID_ABI}/libdash.so ${JNILIBS}/${ANDROID_ABI}/libdash.so COPYONLY)
configure_file($ENV{ANDROID_NDK}/sources/cxx-stl/llvm-libc++/libs/${ANDROID_ABI}/libc++_shared.so ${JNILIBS}/${ANDROID_ABI}/libc++_shared.so COPYONLY)

add_library( OMAFPlayer SHARED IMPORTED)
add_library( MP4VR_shared SHARED IMPORTED)
add_library( dash SHARED IMPORTED)
add_library( heif_shared SHARED IMPORTED)
add_library( c++_shared SHARED IMPORTED)

set_target_properties(OMAFPlayer PROPERTIES IMPORTED_LOCATION    ${JNILIBS}/${ANDROID_ABI}/libOMAFPlayer.so)
set_target_properties(MP4VR_shared PROPERTIES IMPORTED_LOCATION  ${JNILIBS}/${ANDROID_ABI}/libmp4vr_shared.so)
set_target_properties(heif_shared PROPERTIES IMPORTED_LOCATION   ${JNILIBS}/${ANDROID_ABI}/libheif_shared.so)
set_target_properties(dash PROPERTIES IMPORTED_LOCATION          ${JNILIBS}/${ANDROID_ABI}/libdash.so)
set_target_properties(c++_shared PROPERTIES IMPORTED_LOCATION ${JNILIBS}/${ANDROID_ABI}/libc++_shared.so)

add_library( OMAFPlayerWrapper
             SHARED
             src/main/jni/ApplicationJNI.cpp
             )

target_compile_definitions(OMAFPlayerWrapper PRIVATE -DOMAF_ENABLE_OPENGL_GRAPHICS=1)

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_definitions(OMAFPlayerWrapper PRIVATE -DDEBUG=1)
endif()

target_include_directories( OMAFPlayerWrapper PRIVATE
                            ${OMAF_PLAYER_HEADERS}
                            )

target_link_libraries( OMAFPlayerWrapper
                       OMAFPlayer
                       MP4VR_shared
                       dash
                       heif_shared
                       c++_shared
                       android
                       EGL
                       GLESv3
                       log
                       )