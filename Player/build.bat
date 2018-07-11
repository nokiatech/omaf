
REM This file is part of Nokia OMAF implementation
REM
REM Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
REM
REM Contact: omaf@nokia.com
REM 
REM This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
REM subsidiaries. All rights are reserved.
REM
REM Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
REM written consent of Nokia.

@echo off
SETLOCAL enableextensions enabledelayedexpansion
set STARTTIME=%TIME%
set ORIGCD=%CD%
call :getargc argc %*
if %argc%==0 (
 	echo: 
 	echo Usage:
 	echo Run either one combination of SDK commands or one command from the Other commands
 	echo If an option is omitted from SDK commands it runs automatically both
    echo "<script> debug vs2015" creates cmakes and builds vs2015 debug binaries
 	echo "<script> external" builds external binaries
 	echo: 
 	echo SDK commands
 	echo "generate || build"
 	echo "debug    || release"
 	echo "vs2013   || vs2015  || vs2017  || abiv7 || abiv8 || android"
 	echo:
 	echo Other commands
 	echo "all ^ clean"
 	echo:    
    exit /B 1
)

REM # Set SDK parameters
set SDK_PARAMETERS=
REM set SDK_PARAMETERS=%SDK_PARAMETERS% -DBUILD_APPS=OFF

REM Echo commands instead of executing
REM set DEBUG_PRINT=YES

if '%DEBUG_PRINT%' == 'YES' (
 	echo Echoing commands
 	set DEBUG_COMMAND=echo
) ELSE (
    set DEBUG_COMMAND=
)

REM # Set default values
SET GENERATE=NO
SET BUILD=NO
SET DEBUG=NO
SET RELEASE=NO
SET ANDROID=NO
SET VS2013=NO
SET VS2015=NO
SET VS2017=NO
SET ANDROID_ABIV7=NO
SET ANDROID_ABIV8=NO
SET NINJA=NO
SET EXTERNAL=NO
SET ALL=NO
SET CLEAN=NO
SET EXTRA_PARAMETERS=

REM # Read command line arguments
REM # start-of-workaround for the argument parsing "issue"
REM # for /L %%G IN (1,1,1000) do (
REM #     call :getarg %%G tok %*
REM #     if '!tok!'=='' goto done
REM #     echo res: !tok!
REM # )
REM #:done
REM # end-of-workaround
REM currently using a flag (EXTRA) and appending...
set EXTRA=
for %%i in (%*) do (
    SET OK=NO
    SET SUB=NO
    if '!EXTRA!'=='YES' (
        REM okay, we got -D last time so append this to EXTRA_PARAMETERS..
        REM this is because the -Dkey=value arguments get parsed incorrectly by cmd..
        REM NOTE: there must be one whitespace after the following line!
        set EXTRA_PARAMETERS=!EXTRA_PARAMETERS!%%i 
        SET EXTRA=NO
    ) else (
        call :check %%1 ninja NINJA OK
        call :check %%1 generate GENERATE OK
        call :check %%1 build BUILD OK
        call :check %%1 debug DEBUG OK
        call :check %%1 release RELEASE OK
        call :check %%1 vs2013 VS2013 OK
        call :check %%1 vs2015 VS2015 OK
        call :check %%1 vs2017 VS2017 OK
        call :check %%1 android ANDROID OK
        call :check %%1 abiv7 ANDROID_ABIV7 OK
        call :check %%1 abiv8 ANDROID_ABIV8 OK
        call :check %%1 all ALL OK
        call :check %%1 clean CLEAN OK
        call :getsub %%1 SUB
        if '!SUB!'=='^"-' (
            call :getsub2 %%1 SUB
            rem NOTE: there must be one whitespace after the following line!
            set EXTRA_PARAMETERS=!EXTRA_PARAMETERS!!SUB! 
            set OK=YES
        )    
        if '!SUB!'=='-D' (
            set EXTRA_PARAMETERS=!EXTRA_PARAMETERS!%%i^=
            set OK=YES
            set EXTRA=YES
        )        
        if '!OK!' == 'NO' (
            echo UNKNOWN OPTION: %%i
            exit /B 1
        )        
    )
    shift /1
)

if '%ALL%'=='YES' (
    REM Does a full clean build of all variants of the sdk.
    SET GENERATE=YES
    SET BUILD=YES
    SET DEBUG=YES
    SET RELEASE=YES
    SET VS2013=NO
    SET VS2015=YES
    SET VS2017=NO
    SET ANDROID=YES
    SET ANDROID_ABIV7=NO
    SET ANDROID_ABIV8=NO
    SET CLEAN=YES
)

if '%GENERATE%%BUILD%%CLEAN%' == 'NONONO' (
    rem No commands set, so Clean, Generate and Build.
    set CLEAN=YES
    set GENERATE=YES
    set BUILD=YES
)

if '%DEBUG%%RELEASE%' == 'NONO' (
    set DEBUG=YES
    set RELEASE=YES
)

if '%ANDROID%%ANDROID_ABIV7%%ANDROID_ABIV8%'=='YESNONO' (
    set ANDROID_ABIV7=YES
    set ANDROID_ABIV8=YES
)

if '%ANDROID_ABIV7%'=='YES' (
    set ANDROID=YES
)

if '%ANDROID_ABIV8%'=='YES' (
    set ANDROID=YES
)

if '%VS2013%%VS2015%%VS2017%%ANDROID%'=='NONONONO' (
    REM Default to VS2015 build if compiler not set.
    REM set VS2013=YES
    set VS2015=YES
    REM set VS2017=YES
)


if '%NINJA%'=='YES' (
    REM check if ninja can be found.
    set MAKE_BIN="%CD%\ninja.exe"
    call :checkninja !MAKE_BIN!
    if NOT !ERRORLEVEL! EQU 0 goto :error_exit

    REM ninja requested.. fix VS builds..
    if '%VS2013%'=='YES' (
        set VS2013=NO
        set NINJA2013=YES
        %DEBUG_COMMAND% call "%VS120COMNTOOLS%..\..\vc\vcvarsall.bat" x64
    )
    if '%VS2015%'=='YES' (
        set VS2015=NO
        set NINJA2015=YES
        %DEBUG_COMMAND% call "%VS140COMNTOOLS%..\..\vc\vcvarsall.bat" x64
    )
    if '%VS2017%'=='YES' (
        set VS2017=NO
        set NINJA2017=YES
        REM see the following links
        REM https://blogs.msdn.microsoft.com/vcblog/2017/03/06/finding-the-visual-c-compiler-tools-in-visual-studio-2017/
        REM https://github.com/Microsoft/vswhere
        REM other option is to use powershell....
        for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
            set InstallDir=%%i
        )
        %DEBUG_COMMAND% call "!InstallDir!\VC\Auxiliary\Build\vcvarsall" x64
    )
)

if '%ANDROID%'=='YES' (
    if NOT DEFINED ANDROID_NDK (
        echo "ANDROID_NDK is not set"
        exit /B 1
    )    
    set ANDROID_NATIVE_API_LEVEL=android-23
    set ANDROID_STL=c++_shared
    if '%NINJA%'=='YES' (
        set ANDROID_GENERATOR="Ninja"
    ) else (
        set ANDROID_GENERATOR="MinGW Makefiles"
        set MAKE_BIN="%ANDROID_NDK%\prebuilt\windows-x86_64\bin\make.exe"
        call :checkmake !MAKE_BIN!
        if NOT !ERRORLEVEL! EQU 0 goto :error_exit
    )
    if '%CMAKE_TOOLCHAIN%'=='YES' (
        echo Using CMAKE pre-installed toolchain
        set ANDROID_PARAMETERS=-DCMAKE_ANDROID_NDK=c:/NDK/r11c -DCMAKE_SYSTEM_NAME=Android -DCMAKE_ANDROID_STL_TYPE=!ANDROID_STL! -DCMAKE_SYSTEM_VERSION=23 -DCMAKE_MAKE_PROGRAM=!MAKE_BIN!
    ) else (
        echo Using our custom hack patched toolchain
        set ANDROID_PARAMETERS=-DCMAKE_TOOLCHAIN_FILE=android.toolchain.cmake -DANDROID_STL=!ANDROID_STL! -DANDROID_NATIVE_API_LEVEL=!ANDROID_NATIVE_API_LEVEL! -DCMAKE_MAKE_PROGRAM=!MAKE_BIN!
    )    
)

echo:
echo GENERATE                   = %GENERATE%
echo BUILD                      = %BUILD%
echo DEBUG                      = %DEBUG%
echo RELEASE                    = %RELEASE%
echo NINJA                      = %NINJA%
echo VS2013                     = %VS2013%
echo VS2015                     = %VS2015%
echo VS2017                     = %VS2017%
echo ANDROID                    = %ANDROID%
echo ANDROID_ABIV7              = %ANDROID_ABIV7%
echo ANDROID_ABIV8              = %ANDROID_ABIV8%
echo CLEAN                      = %CLEAN%
echo EXTRA_PARAMETERS           = %EXTRA_PARAMETERS%
echo SDK_PARAMETERS             = %SDK_PARAMETERS%
echo ALL                        = %ALL%
echo:

rem init some variables.
set PLATFORM_WINDOWS=Windows
set PLATFORM_ANDROID=Android
set BUILD_TYPE_DEBUG=Debug
set BUILD_TYPE_RELEASE=Release
set BUILDERS_ANDROID=ANDROID_ABIV7 ANDROID_ABIV8
set BUILDERS_WINDOWS=VS2013 VS2015 VS2017 NINJA2013 NINJA2015 NINJA2017

set IS_IDE_VS2013=IDE
set IS_IDE_VS2015=IDE
set IS_IDE_VS2017=IDE
set IS_IDE_ANDROID_ABIV7=NO_IDE
set IS_IDE_ANDROID_ABIV8=NO_IDE
set IS_IDE_NINJA2013=NO_IDE
set IS_IDE_NINJA2015=NO_IDE
set IS_IDE_NINJA2017=NO_IDE

set EXT=

set GENERATOR_ANDROID_ABIV7=!ANDROID_GENERATOR!
set BUILD_PATH_ANDROID_ABIV7_DEBUG=bld_android_DEBUG!EXT!\armeabi-v7a
set BUILD_PATH_ANDROID_ABIV7_RELEASE=bld_android_RELEASE!EXT!\armeabi-v7a
if '%CMAKE_TOOLCHAIN%'=='YES' (
    set SDK_PARAMETERS_ANDROID_ABIV7=!ANDROID_PARAMETERS! -DANDROID_ABI=armeabi-v7a -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a !SDK_PARAMETERS! 
) else (
    set SDK_PARAMETERS_ANDROID_ABIV7=!ANDROID_PARAMETERS! -DANDROID_ABI=armeabi-v7a !SDK_PARAMETERS! 
)
set ANDROID_ABIV7_ABI=\armeabi-v7a

set GENERATOR_ANDROID_ABIV8=!ANDROID_GENERATOR!
set BUILD_PATH_ANDROID_ABIV8_DEBUG=bld_android_DEBUG!EXT!\arm64-v8a
set BUILD_PATH_ANDROID_ABIV8_RELEASE=bld_android_RELEASE!EXT!\arm64-v8a
if '%CMAKE_TOOLCHAIN%'=='YES' (
    set SDK_PARAMETERS_ANDROID_ABIV8=!ANDROID_PARAMETERS! -DANDROID_ABI=arm64-v8a -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a !SDK_PARAMETERS! 
) else (
    set SDK_PARAMETERS_ANDROID_ABIV8=!ANDROID_PARAMETERS! -DANDROID_ABI=arm64-v8a !SDK_PARAMETERS! 
)
set ANDROID_ABIV8_ABI=\arm64-v8a

set GENERATOR_VS2013="Visual Studio 12 2013 Win64"
set BUILD_PATH_VS2013_DEBUG=bld_windows_2013!EXT!
set BUILD_PATH_VS2013_RELEASE=bld_windows_2013!EXT!
set SDK_PARAMETERS_VS2013=!SDK_PARAMETERS!

set GENERATOR_VS2015="Visual Studio 14 2015 Win64"
set BUILD_PATH_VS2015_DEBUG=bld_windows_2015!EXT!
set BUILD_PATH_VS2015_RELEASE=bld_windows_2015!EXT!
set SDK_PARAMETERS_VS2015=!SDK_PARAMETERS!

set GENERATOR_VS2017="Visual Studio 15 2017 Win64"
set BUILD_PATH_VS2017_DEBUG=bld_windows_2017!EXT!
set BUILD_PATH_VS2017_RELEASE=bld_windows_2017!EXT!
set SDK_PARAMETERS_VS2017=!SDK_PARAMETERS!

rem NINJA for vs2013
set GENERATOR_NINJA2013="Ninja"
set BUILD_PATH_NINJA2013_DEBUG=bld_windows_NINJA_2013_DEBUG!EXT!
set BUILD_PATH_NINJA2013_RELEASE=bld_windows_NINJA_2013_RELEASE!EXT!
set SDK_PARAMETERS_NINJA2013=!SDK_PARAMETERS! -DCMAKE_MAKE_PROGRAM="%CD%\ninja.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_CXX_COMPILER="cl.exe"

rem NINJA for vs2015
set GENERATOR_NINJA2015="Ninja"
set BUILD_PATH_NINJA2015_DEBUG=bld_windows_NINJA_2015_DEBUG!EXT!
set BUILD_PATH_NINJA2015_RELEASE=bld_windows_NINJA_2015_RELEASE!EXT!
set SDK_PARAMETERS_NINJA2015=!SDK_PARAMETERS! -DCMAKE_MAKE_PROGRAM="%CD%\ninja.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_CXX_COMPILER="cl.exe"

rem NINJA for vs2015
set GENERATOR_NINJA2017="Ninja"
set BUILD_PATH_NINJA2017_DEBUG=bld_windows_NINJA_2017_DEBUG!EXT!
set BUILD_PATH_NINJA2017_RELEASE=bld_windows_NINJA_2017_RELEASE!EXT!
set SDK_PARAMETERS_NINJA2017=!SDK_PARAMETERS! -DCMAKE_MAKE_PROGRAM="%CD%\ninja.exe" -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_CXX_COMPILER="cl.exe"

set CMAKE_GENERATE_NO_IDE_DEBUG=-DCMAKE_BUILD_TYPE=!BUILD_TYPE_DEBUG!
set CMAKE_GENERATE_NO_IDE_RELEASE=-DCMAKE_BUILD_TYPE=!BUILD_TYPE_RELEASE!

set CMAKE_BUILD_NO_IDE_DEBUG=
set CMAKE_BUILD_NO_IDE_RELEASE=

set CMAKE_GENERATE_IDE_DEBUG=
set CMAKE_GENERATE_IDE_RELEASE=

set CMAKE_BUILD_IDE_DEBUG=--config !BUILD_TYPE_DEBUG!
set CMAKE_BUILD_IDE_RELEASE=--config !BUILD_TYPE_RELEASE!

for %%p in (WINDOWS ANDROID) do (
    for %%a in (!BUILDERS_%%p!) do (
        set GENERATED=NO
        set CLEANED=NO
        if '!%%a!'=='YES' (
            echo -----------------------------------------------------
            echo Current platform %%p
            echo -----------------------------------------------------
            set PLATFORM=!PLATFORM_%%p!
            for %%c in (CLEAN GENERATE BUILD) do (
                if '!%%c!'=='YES' (
                    set FINAL_GENERATOR=!GENERATOR_%%a!
                    set SDK_FINAL_PARAMETERS=-G!FINAL_GENERATOR! !SDK_PARAMETERS_%%a! !EXTRA_PARAMETERS!
					set SDK_FINAL_PARAMETERS=!SDK_FINAL_PARAMETERS! -DINSTALLER=NO
                    
                    if '%%c'=='CLEAN' (
                        for %%t in (DEBUG RELEASE) do (
                            if '!%%t!'=='YES' (                                
                                echo -----------------------------------------------------
                                echo Cleaning %%p %%a %%t 
                                echo -----------------------------------------------------
    
                                set doit=YES
                                if '!IS_IDE_%%a!!CLEANED!'=='IDEYES' (set doit=NO)
                                if '!IS_IDE_%%a!!CLEANED!'=='IDENO' (set CLEANED=YES)
                                if '!doit!'=='YES' (
                                    %DEBUG_COMMAND% rd /S /Q !BUILD_PATH_%%a_%%t!
                                )
                                %DEBUG_COMMAND% rd /S /Q ..\Lib\!PLATFORM!\!BUILD_TYPE_%%t!!%%a_ABI!
                            )
                        )
                    )

                    if '%%c'=='GENERATE' (
                        for %%t in (DEBUG RELEASE) do (
                            if '!%%t!'=='YES' (
                                set doit=YES
                                if '!IS_IDE_%%a!!GENERATED!'=='IDEYES' (set doit=NO)
                                if '!IS_IDE_%%a!!GENERATED!'=='IDENO' (set GENERATED=YES)

                                if '!doit!'=='YES' (
                                    echo -----------------------------------------------------
                                    set final='NOTSET'
                                    if '!IS_IDE_%%a!'=='IDE' (
                                        echo Generating %%p %%a
                                        set final=!CMAKE_GENERATE_IDE_%%t!
                                    )
                                    if '!IS_IDE_%%a!'=='NO_IDE' (
                                        echo Generating %%p %%a %%t
                                        set final=!CMAKE_GENERATE_NO_IDE_%%t!
                                    )
									echo From %ORIGCD% 
                                    echo -----------------------------------------------------
                                    %DEBUG_COMMAND% mkdir !BUILD_PATH_%%a_%%t!
                                    %DEBUG_COMMAND% cd !BUILD_PATH_%%a_%%t!
                                    if NOT !ERRORLEVEL! EQU 0 goto :error_exit
                                    %DEBUG_COMMAND% cmake !final! !SDK_FINAL_PARAMETERS! %ORIGCD%
                                    if NOT !ERRORLEVEL! EQU 0 goto :error_exit
                                    %DEBUG_COMMAND% cd %ORIGCD%
                                )
                            )
                        )
                    )
                    
                    if '%%c'=='BUILD' (
                        for %%t in (DEBUG RELEASE) do (
                            if '!%%t!'=='YES' (
                                echo -----------------------------------------------------
                                echo Building %%p %%a %%t 
                                echo -----------------------------------------------------
                                set final='NOTSET'
                                if '!IS_IDE_%%a!'=='IDE' (set final=!CMAKE_BUILD_IDE_%%t!)
                                if '!IS_IDE_%%a!'=='NO_IDE' (set final=!CMAKE_BUILD_NO_IDE_%%t!)
                                %DEBUG_COMMAND% cmake --build !BUILD_PATH_%%a_%%t!\ !final!
                                if NOT !ERRORLEVEL! EQU 0 goto :error_exit
                            )
                        )
                    )
                    )
                )
            )
        )
    )
)

if NOT !ERRORLEVEL! EQU 0 goto :error_exit
cd %ORIGCD%
set ENDTIME=%TIME%
echo Build started: %STARTTIME%
echo Build ended: %ENDTIME%
goto :eof

rem Helper functions after this
:checkmake
    echo checkmake %1
    if NOT EXIST %1 (
        echo "Check ANDROID_NDK environment variable, make.exe not found!"
        exit /B 1
    )
    goto :eof
    
:checkninja:
    if NOT EXIST %1 (
        echo "ninja.exe is missing."
        exit /B 1
    )
    goto :eof

:getsub
    SET _test=%1
    SET _result=%_test:~0,2%
    set %2=%_result%
    goto :eof

:getsub2
    SET _test2=%1
    SET _result2=%_test:~1,-1%
    set %2=%_result2%
    goto :eof
    
:check
    set "temp=0"
    if /I %1==%2 set temp=1
    if %temp% EQU 1 (
        set %3=YES
        set %4=YES
    )    
    goto :eof

:getarg
    set %2=
    for /F "tokens=%1" %%i in ("%*") do (
        set %2=%%i
    )
    goto :eof

:getargc
    set getargc_v0=%1
    set /a "%getargc_v0% = 0"
:getargc_l0
    if not x%2x==xx (
        shift
        set /a "%getargc_v0% = %getargc_v0% + 1"
        goto :getargc_l0
    )
    set getargc_v0=
    goto :eof

:error_exit
set terrorlevel=%ERRORLEVEL%
echo FAILED: %terrorlevel%
cd %ORIGCD%

for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set DOUBLECLICKED=1
if defined DOUBLECLICKED pause

exit /B %terrorlevel%
