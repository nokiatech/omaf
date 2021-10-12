
#!/usr/bin/bash
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

num_cores=$(grep -c '^physical id\s*:\s0$' /proc/cpuinfo)

# failsafe
if [ -z "$num_cores" ]; then
    num_cores=1
fi

configure() {
    local api_level=$1
    local abi=$2
    local build_type=$3
    cmake \
      -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
      -DANDROID_NATIVE_API_LEVEL=$api_level                           \
      -DANDROID_ABI=$abi                                              \
      -DANDROID_STL=c++_static                                        \
      -DANDROID_STL_FORCE_FEATURES=ON                                 \
      -DCMAKE_BUILD_TYPE=$build_type                                  \
      -DNO_TESTS=1                                                    \
      ../../../../srcs                                                \
      -DCMAKE_SHARED_LINKER_FLAGS=-Wl,--exclude-libs=c++_static.a
}

build() {
    cmake --build . -- -j$num_cores -O
}

configure_and_build() {
    local dir=$1
    shift
    mkdir -p "$dir"
    if ! ( cd "$dir" &&
	   configure "$@" &&
	   build ); then
	echo "Failed to compile $*"
	exit 1
    fi
}

#                   directory           abi level  abi         build type
if [ "$BUILD_SINGLE" = "1" ]; then
    configure_and_build debug/armeabi-v7a   android-23 armeabi-v7a Debug
else
    configure_and_build debug/armeabi-v7a   android-23 armeabi-v7a Debug
    configure_and_build debug/arm64-v8a     android-23 arm64-v8a   Debug
    configure_and_build release/armeabi-v7a android-23 armeabi-v7a Release
    configure_and_build release/arm64-v8a   android-23 arm64-v8a   Release
fi