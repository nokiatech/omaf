
@echo off
REM This file is part of Nokia OMAF implementation
REM
REM Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
REM
REM Contact: omaf@nokia.com
REM
REM This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
REM subsidiaries. All rights are reserved.
REM
REM Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
REM written consent of Nokia.

SetLocal EnableDelayedExpansion enableextensions
set action=""
set abi=""
set buildType=Release
set apiLevel="android-28"
set buildMP4=1
set buildPlayer=1
set help=0

WHERE cmake > nul
IF %ERRORLEVEL% NEQ 0 (
    echo:
    echo "ERROR: CMake not found!"
    echo:
    exit /b 1
)

WHERE ninja > nul
IF %ERRORLEVEL% NEQ 0 (
    echo:
    echo "ERROR: Ninja not found!"
    echo:
    exit /b 1
)

if NOT DEFINED ANDROID_NDK (
    echo:
    echo "ERROR: ANDROID_NDK is not set!"
    echo:
    exit /b 1
) 

call :parsearguments %*
if %abi%=="" (
    call :usage
    exit /b 1
)
if %help%==1 (
    call :usage
    exit /b 1
)

set ORIGCD=%CD%
set SRCDIR="..\..\..\..\srcs"
set num_cores=%NUMBER_OF_PROCESSORS%
if %num_cores% LSS 1 (
    set num_cores=1
)
echo Cores: %num_cores%

IF NOT EXIST "%ORIGCD%\..\Lib\Android\%buildType%\%abi%\libdash.so" (
    call :build_dash %buildType% %abi%
) ELSE (
    echo libdash found.
)
IF NOT EXIST "%ORIGCD%\..\Lib\Android\%buildType%\%abi%\libheif_shared.so" (
    call :build_heif %buildType% %abi%
) ELSE  (
    echo libheif found.
)
IF %buildMP4%==1 (
    call :build_mp4vr %buildType% %abi%
)

IF %buildPlayer%==1 (
    IF NOT EXIST "%ORIGCD%\..\..\libdash" (
        echo "Please clone libdash to a folder adjacent to the root of this project"
        exit /b 1
    )
   
    call :build_player %buildType% %abi%
)
exit /b 0

:parsearguments

    :while
    IF "%1"=="" exit /b 0
    
    IF "%1"=="abiv7" set abi=armeabi-v7a
    IF "%1"=="abiv8" set abi=arm64-v8a
    IF "%1"=="debug" set buildType=Debug
    IF "%1"=="release" set buildType=Release
    IF "%1"=="skipmp4" set buildMP4=0
    IF "%1"=="skipplayer" set buildPlayer=0
    IF "%1"=="help" set help=1
    IF "%1"=="--help" set help=1
    shift
    goto :while

:clean
    cd %1
    IF EXIST "%2\%3" rd /s /q "%2\%3"
    cd %ORIGCD%
    exit /b 0 

:createdir
    md %1\%2\%3
    cd %ORIGCD%
    exit /b 0
    
:configure
    cd %1\%2\%3
    call cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=%ANDROID_NDK%\build\cmake\android.toolchain.cmake -DANDROID_NATIVE_API_LEVEL=%apiLevel% -DANDROID_ABI=%3 -DANDROID_STL=c++_static -DANDROID_STL_FORCE_FEATURES=ON -DCMAKE_BUILD_TYPE=%2 -DNO_TESTS=1 %4 -DCMAKE_SHARED_LINKER_FLAGS=-Wl,--exclude-libs=c++_static.a
    exit /b 0
    
:build
    cd %1\%2\%3
    call cmake --build . -- -j%num_cores%
    cd %ORIGCD%
    exit /b 0
    
:build_dash
    echo:
    echo Building dash
    echo:
    set LIBDASHDIR=%ORIGCD%\..\..\libdash\build\android
    set DSTDIR=%ORIGCD%\..\Lib\Android\%1\%2
    call :clean %LIBDASHDIR% %1 %2
    call :createdir %LIBDASHDIR% %1 %2
    call :configure %LIBDASHDIR% %1 %2 "..\..\..\..\libdash\libdash"
    call :build %LIBDASHDIR% %1 %2
    xcopy /b/v/y "%LIBDASHDIR%\%1\%2\libdash*.so" "%DSTDIR%\"
    cd %ORIGCD%
    set DSTDIR=
    set LIBDASHDIR=
    exit /b 0
    
:build_heif
    echo:
    echo building HEIF
    echo:
    set LIBHEIFDIR=%ORIGCD%\..\..\heif\build\android
    set DSTDIR=%ORIGCD%\..\Lib\Android\%1\%2
    call :clean %LIBHEIFDIR% %1 %2
    call :createdir %LIBHEIFDIR% %1 %2
    call :configure %LIBHEIFDIR% %1 %2 "..\..\..\.."
    call :build %LIBHEIFDIR% %1 %2
    mkdir %DSTDIR%
    xcopy /b/v/y "%LIBHEIFDIR%\%1\%2\srcs\lib\libheif*.*" "%DSTDIR%\"
    cd %ORIGCD%
    set DSTDIR=
    set LIBHEIFDIR=
    exit /b 0
    
:build_mp4vr
    echo:
    echo building mp4vr
    echo:
    set LIBMP4DIR=%ORIGCD%\..\Mp4\build\android
    set DSTDIR=%ORIGCD%\..\Mp4\lib\android\%1\%2
    call :clean %LIBMP4DIR% %1 %2
    call :createdir %LIBMP4DIR% %1 %2
    call :configure %LIBMP4DIR% %1 %2 "..\..\..\..\srcs"
    call :build %LIBMP4DIR% %1 %2
    mkdir %DSTDIR%
    xcopy /b/v/y "%LIBMP4DIR%\%1\%2\lib\*.*" "%DSTDIR%\"
    cd %ORIGCD%
    set DSTDIR=
    set LIBMP4DIR=
    exit /b 0
    
:build_player
    echo:
    echo building Player
    echo:
    cd %ORIGCD%\..\Player
    IF "%2%"=="armeabi-v7a" (
        call build.bat abiv7 %1% clean
        call build.bat abiv7 %1% Ninja
    ) ELSE IF "%2%"=="arm64-v8a" (
        call build.bat abiv8 %1% clean
        call build.bat abiv8 %1% Ninja
    )
    cd %ORIGCD%
    exit /b 0
    
:usage
    echo:
    echo Usage:
    echo Set ANDROID_NDK environment variable to point to your android-ndk-r17c directory
    echo which contains unpacked android ndk downloaded from
    echo https://developer.android.com/ndk/downloads/older_releases
    echo:
    echo Run script and select android abi version which should be compiled.
    echo NOTE: building creator for android is not supported
    echo:
    echo "build-android.sh abiv7"         Builds armeabi-v7a release binaries
    echo "build-android.sh abiv8 debug"   Builds arm64-v8a debug binaries
    echo:
    echo Build args:
    echo "abiv7  || abiv8   version of compiled libs"
    echo "debug  || release (defaults to release)"
    echo "help|--help       prints this usage information"
    echo "skipmp4           skip building mp4 libraries (must be built for creator / player)"
    echo "skipplayer        skip building player libraries"
    echo:
    echo Libraries and executables are built to Mp4/lib and Player/Lib directories.
    echo:
    
    exit /b 0