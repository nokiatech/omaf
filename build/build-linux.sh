
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

function usage() {
 	echo ''
 	echo 'Usage:'
 	echo 'Run script and optionally select which parts should be compiled.'
 	echo 'NOTE: building player on linux is not supported'
    echo ''
 	echo '"build-linux.sh"       Builds release binaries'
    echo '"build-linux.sh debug" Builds debug binaries'
 	echo ''
 	echo 'Build args:'
 	echo "debug  || release (defaults to release)"
 	echo "help|--help       prints this usage information"
 	echo "skipmp4           skip building mp4 libraries (must be built for creator / player)"
 	echo "skipcreator       skip building creator app"
 	echo ''
 	echo 'Libraries and executables are built to Mp4/lib and Creator/bin directories.'
 	echo ''
    exit $([ -z $1 ] && echo 0 || echo 1)
}

ORIGDIR=$PWD

BUILDMP4=YES
BUILDCREATOR=YES
BUILDTYPE=release
BUILDTYPEDIR=Release


while (( "$#" )); do
case $1 in
    help|--help)
    usage
    ;;
    debug)
    BUILDTYPE=debug
    BUILDTYPEDIR=Debug
    ;;
    skipmp4)
    BUILDMP4=NO
    ;;
    skipcreator)
    BUILDCREATOR=NO
    ;;
    *)
    echo 'ERROR: Invalid argument "$1"'
    usage $1
    ;;
esac
shift
done

# echo compilation commands
set -x

if [ ! -f ../Lib/Linux/$BUILDTYPEDIR/libheifpp.a ]; then
    HEIFBUILDDIR=linux/$BUILDTYPE
    DSTDIR=$ORIGDIR/../Lib/Linux/$BUILDTYPEDIR
    cd $ORIGDIR/../../heif
    rm -fr $HEIFBUILDDIR
    mkdir -p $HEIFBUILDDIR
    cd $HEIFBUILDDIR
    cmake . -G "Unix Makefiles" ../.. -DCMAKE_BUILD_TYPE=${BUILDTYPE^^}
    make -j6
    mkdir -p $DSTDIR
    cp srcs/lib/* $DSTDIR
    cd $ORIGDIR
fi

if [ "$BUILDMP4" == "YES" ]; then
    MP4BUILDDIR=linux/$BUILDTYPE
    DSTDIR=$ORIGDIR/../Mp4/lib/Linux/$BUILDTYPEDIR
    cd $ORIGDIR/../Mp4/build
    rm -fr $MP4BUILDDIR
    mkdir -p $MP4BUILDDIR
    cd $MP4BUILDDIR
    cmake . -G "Unix Makefiles" ../../../srcs -DCMAKE_BUILD_TYPE=${BUILDTYPE^^}
    make -j6
    mkdir -p $DSTDIR
    cp lib/* $DSTDIR
    cd $ORIGDIR
fi

if [ "$BUILDCREATOR" == "YES" ]; then
    CREATORBUILDDIR=linux-build-$BUILDTYPE
    DSTDIR=$ORIGDIR/../Creator/bin/Linux/$BUILDTYPEDIR
    cd $ORIGDIR/../Creator
    rm -fr $CREATORBUILDDIR
    mkdir -p $CREATORBUILDDIR
    cd $CREATORBUILDDIR
    cmake . -G "Unix Makefiles" ../src -DCMAKE_BUILD_TYPE=${BUILDTYPE^^}
    make -j6
    mkdir -p $DSTDIR
    cp bin/* $DSTDIR
    cd $ORIGDIR
fi
