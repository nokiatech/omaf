
#!/usr/bin/bash
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

set +x

function usage() {
 	echo ''
 	echo 'Usage:'
    echo 'Set ANDROID_NDK environment variable to point to your android-ndk-r16b directory'
    echo 'which contains unpacked android ndk downloaded from'
    echo 'https://developer.android.com/ndk/downloads/older_releases'
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
    local build_src_dir=${4:-../../../../srcs}
    cmake \
      -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake \
      -DANDROID_NATIVE_API_LEVEL=$api_level                           \
      -DANDROID_ABI=$abi                                              \
      -DANDROID_STL=c++_static                                        \
      -DANDROID_STL_FORCE_FEATURES=ON                                 \
      -DCMAKE_BUILD_TYPE=$build_type                                  \
      -DNO_TESTS=1                                                    \
      $build_src_dir                                                  \
      -DCMAKE_SHARED_LINKER_FLAGS=-Wl,--exclude-libs=c++_static.a
}

build() {
    cmake --build . -- -j6
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

if [ ! -f ../Lib/Android/$BUILDTYPE/$ANDROIDVERSION/libdash.so ]; then
    LIBDASHBUILDDIR=android/$BUILDTYPEDIR/$ANDROIDVERSION
    DSTDIR=$ORIGDIR/../Lib/Android/$BUILDTYPE/$ANDROIDVERSION
    cd $ORIGDIR/../../libdash/libdash
    rm -fr $LIBDASHBUILDDIR
    configure_and_build $LIBDASHBUILDDIR android-24 $ANDROIDVERSION $BUILDTYPE ../../../libdash
    mkdir -p $DSTDIR
    cp $LIBDASHBUILDDIR/libdash*.so $DSTDIR
    cd $ORIGDIR
fi

if [ ! -f ../Lib/Android/$BUILDTYPE/$ANDROIDVERSION/libheifpp.a ]; then
    HEIFBUILDDIR=android/$BUILDTYPEDIR/$ANDROIDVERSION
    DSTDIR=$ORIGDIR/../Lib/Android/$BUILDTYPE/$ANDROIDVERSION
    cd $ORIGDIR/../../heif
    rm -fr $HEIFBUILDDIR
    configure_and_build $HEIFBUILDDIR android-24 $ANDROIDVERSION $BUILDTYPE ../../..
    mkdir -p $DSTDIR
    cp $HEIFBUILDDIR/srcs/lib/* $DSTDIR
    cd $ORIGDIR
fi

if [ "$BUILDMP4" == "YES" ]; then
    MP4BUILDDIR=android/$BUILDTYPEDIR/$ANDROIDVERSION
    DSTDIR=$ORIGDIR/../Mp4/lib/android/$BUILDTYPEDIR/$ANDROIDVERSION
    cd $ORIGDIR/../Mp4/build
    configure_and_build $MP4BUILDDIR android-24 $ANDROIDVERSION $BUILDTYPE
    mkdir -p $DSTDIR
    cp $MP4BUILDDIR/lib/* $DSTDIR
    cd $ORIGDIR
fi

if [ "$BUILDPLAYER" == "YES" ]; then
    if [ ! -d "$ORIGDIR/../../libdash" ]; then
        echo "Please clone libdash to a folder adjacent to the root of this project"
        exit 1
    fi

    # build PLayer lib
    cd $ORIGDIR/../Player
    bash ./build-android.sh ${BUILDTYPE,,} $ANDROIDSHORTVERSION clean
    bash ./build-android.sh ${BUILDTYPE,,} $ANDROIDSHORTVERSION
fi
