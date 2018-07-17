
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

if [[ -z "$ANDROID_NDK" ]]; then echo "ANDROID_NDK is not set" && exit 1; fi

if [ "$#" -eq 0 ]; then
	echo 
	echo Usage:
	echo Run either one combination of SDK commands or one command from the Other commands
	echo If an option is omitted from SDK commands it runs automatically both
	echo "<script> build debug abiv7" creates cmakes and builds abiv7 debug binaries
    echo
    echo Build requires cmake 3.6 to be found from path and ANDROID_NDK environment variable
    echo pointing to android-ndk-r14b folder. 
	echo 
	echo SDK commands
	echo "generate || build"
	echo "debug    || release"
	echo "abiv7    || abiv8"
	echo 
	echo Other commands
    echo "all ^ clean "
	echo 
	
    exit 1;
fi


# Exit immediately if a command exits with a non-zero status.
set -e


# Set SDK parameters
SDK_PARAMETERS=""
SDK_PARAMETERS="$SDK_PARAMETERS -DBUILD_SHARED=ON"
SDK_PARAMETERS="$SDK_PARAMETERS -DENABLE_ANALYTICS=ON"
SDK_PARAMETERS="$SDK_PARAMETERS -DBUILD_APPS=OFF"


# Echo commands instead of executing
DEBUG_PRINT="NO"

if [ $DEBUG_PRINT == "YES" ]; then
	echo Echoing commands
	DEBUG_COMMAND=echo
else
	DEBUG_COMMAND=
fi


# Android setup
ANDROID_NATIVE_API_LEVEL=android-23
ANDROID_STL=c++_shared


# Set default values
GENERATE=NO
BUILD=NO
DEBUG=NO
RELEASE=NO
ABIV7=NO
ABIV8=NO
ALL=NO
CLEAN=NO

EXTRA_PARAMETERS=""

# Read command line arguments
FAIL=NO
for i in "$@"
do
case $i in
    generate)
    GENERATE=YES
    shift
    ;;
    build)
    BUILD=YES
    shift
    ;;
    debug)
    DEBUG=YES
    shift
    ;;
    release)
    RELEASE=YES
    shift
    ;;
    abiv7)
    ABIV7=YES
    shift
    ;;
    abiv8)
    ABIV8=YES
    shift
    ;;
    all)
    ALL=YES
    shift
    ;;
    clean)
    CLEAN=YES
    shift
    ;;
    -D*)
    EXTRA_PARAMETERS="$i $EXTRA_PARAMETERS"
    shift
    ;;
    *)
    echo UNKNOWN OPTION: $i
    FAIL=YES
    ;;
esac
done

if [ $FAIL == "YES" ]; then
    exit 1;
fi

if [[ $GENERATE == "NO" && $BUILD == "NO" ]]; then
	GENERATE=YES
	BUILD=YES
fi

if [[ $DEBUG == "NO" && $RELEASE == "NO" ]]; then
	DEBUG=YES
	RELEASE=YES
fi

if [[ $ABIV7 == "NO" && $ABIV8 == "NO" ]]; then
	ABIV7=YES
	ABIV8=YES
fi

if [ $CLEAN == "YES" ]; then

	$DEBUG_COMMAND rm -fr bld_android_*
    exit 1;
    
elif [ $ALL == "YES" ]; then

	GENERATE=YES
	BUILD=YES
	DEBUG=YES
	RELEASE=YES
	ABIV7=YES
	ABIV8=YES
	CLEAN=NO
	
fi

echo
echo "GENERATE         = ${GENERATE}"
echo "BUILD            = ${BUILD}"
echo "DEBUG            = ${DEBUG}"
echo "RELEASE          = ${RELEASE}"
echo "ABIV7            = ${ABIV7}"
echo "ABIV8            = ${ABIV8}"
echo "CLEAN            = ${CLEAN}"
echo "EXTRA_PARAMETERS = ${EXTRA_PARAMETERS}"
echo

set -x

for target in "DEBUG" "RELEASE"
do
	for abi in "ABIV7" "ABIV8"
	do
		for command in "GENERATE" "BUILD"
		do
		
			if [ $abi == "ABIV7" ]; then
				ANDROID_ABI=armeabi-v7a
			else
				ANDROID_ABI=arm64-v8a
			fi
			
			if [ $target == "DEBUG" ]; then
				BUILD_TYPE="Debug"
			else
				BUILD_TYPE="Release"
			fi
			
		   	if [[ ${!command} == "YES" && ${!target} == "YES" && ${!abi} == "YES" ]]; then
				echo
				echo Running $command $target $ANDROID_ABI
			else
			   	continue
		   	fi  	
		   	
			$DEBUG_COMMAND mkdir -p bld_android_$target/$ANDROID_ABI
		
			if [ $command == "GENERATE" ]; then
				$DEBUG_COMMAND cd bld_android_$target/$ANDROID_ABI
				$DEBUG_COMMAND cmake -G"Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DANDROID_STL=$ANDROID_STL -DANDROID_NATIVE_API_LEVEL=$ANDROID_NATIVE_API_LEVEL -DANDROID_ABI=$ANDROID_ABI $SDK_PARAMETERS $EXTRA_PARAMETERS ../../
				$DEBUG_COMMAND cd ../..
			fi
		   	
			if [ $command == "BUILD" ]; then
				$DEBUG_COMMAND cmake --build bld_android_$target/$ANDROID_ABI/ -- -j6
			fi
		   	
		done
	done
done
