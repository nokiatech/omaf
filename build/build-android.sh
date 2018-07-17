
#!/usr/bin/bash
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

set +x
@echo off

function usage() {
 	echo ''
 	echo 'Usage:'
    echo 'Set ANDROID_NDK environment variable to point to your android-ndk-r14b directory'
    echo 'which contains unpacked android ndk downloaded from'
    echo 'https://dl.google.com/android/repository/android-ndk-r14b-linux-x86_64.zip'
 	echo ''
 	echo 'Run script and select android abi version which should be compiled.'
 	echo 'NOTE: building creator for android is not supported'
    echo ''
 	echo '"build-android.sh abiv7"         Builds armeabi-v7a release binaries'
    echo '"build-android.sh abiv8 debug"   Builds arm64-v8a debug binaries'
 	echo ''
 	echo 'Build args:'
 	echo "abiv7  || abiv8   version of compiled libs"
 	echo "debug  || release (defaults to release)"
 	echo "help|--help       prints this usage information"
 	echo "skipmp4           skip building mp4 libraries (must be built for creator / player)"
 	echo "skipplayer        skip building player libraries"
 	echo ''
 	echo 'Libraries and executables are built to Mp4/lib and Player/Lib directories.'
 	echo ''
    exit $([ -z $1 ] && echo 0 || echo 1)
}

ORIGDIR=$PWD
ANDROIDVERSION=arm64-v8a
ANDROIDSHORTVERSION=abiv8
BUILDMP4=YES
BUILDPLAYER=YES
BUILDTYPE=Release
BUILDTYPEDIR=release
LIBDAHSFILENAME=libdash.so

MISSINGREQUIREDARG=YES

while (( "$#" )); do
case $1 in
    help|--help)
    usage
    ;;
    debug)
    BUILDTYPE=Debug
    BUILDTYPEDIR=debug
    LIBDAHSFILENAME=libdashd.so
    ;;
    abiv7)
    ANDROIDSHORTVERSION=abiv7
    ANDROIDVERSION=armeabi-v7a
    MISSINGREQUIREDARG=NO
    ;;
    abiv8)
    ANDROIDVERSION=arm64-v8a
    MISSINGREQUIREDARG=NO
    ;;
    skipmp4)
    BUILDMP4=NO
    ;;
    skipplayer)
    BUILDPLAYER=NO
    ;;
    *)
    echo 'ERROR: Invalid argument "$1"'
    usage $1
    ;;
esac
shift
done

if [ "$MISSINGREQUIREDARG" == "YES" ]; then
    echo 'ERROR: Either "abiv7" or "abiv8" must be given to build script'
    usage $1
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
    cmake --build . -- -j6 -O
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

# echo compilation commands
set -x

if [ "$BUILDMP4" == "YES" ]; then
    MP4BUILDDIR=android/$BUILDTYPEDIR/$ANDROIDVERSION
    DSTDIR=$ORIGDIR/../Mp4/lib/android/$BUILDTYPEDIR/$ANDROIDVERSION
    cd $ORIGDIR/../Mp4/build
    configure_and_build $MP4BUILDDIR android-23 $ANDROIDVERSION $BUILDTYPE
    mkdir -p $DSTDIR
    cp $MP4BUILDDIR/lib/* $DSTDIR
    cd $ORIGDIR
fi

if [ "$BUILDPLAYER" == "YES" ]; then
    if [ ! -d "$ORIGDIR/../../libdash" ]; then
        echo "Please clone libdash to a folder adjacent to the root of this project"
        exit 1
    fi

    if [ ! -f "$ORIGDIR/../OpenGLExt/gl/glext.h" ]; then
        echo "Please download OpenGL extension headers, see OpenGLExt/readme.txt"
        exit 1
    fi

    # build libdash
    cd $ORIGDIR/../../libdash/libdash
    LIBDASHBUILDDIR=android-build-$BUILDTYPE/$ANDROIDVERSION
    rm -fr $LIBDASHBUILDDIR
    mkdir -p $LIBDASHBUILDDIR
    cd $LIBDASHBUILDDIR
    cmake -G "Unix Makefiles" \
        -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_NATIVE_API_LEVEL=android-23 \
        -DANDROID_ABI=$ANDROIDVERSION \
        -DANDROID_STL=c++_shared \
        -DCMAKE_BUILD_TYPE=$BUILDTYPE ../../libdash
    cmake --build . -- -j6

    # build PLayer lib
    DSTDIR=$ORIGDIR/../../libdash/lib/android/$BUILDTYPEDIR/$ANDROIDVERSION
    mkdir -p $DSTDIR
    cp libdash.so $DSTDIR/$LIBDAHSFILENAME

    cd $ORIGDIR/../Player
    ./build-android.sh ${BUILDTYPE,,} $ANDROIDSHORTVERSION clean
    ./build-android.sh ${BUILDTYPE,,} $ANDROIDSHORTVERSION
fi
