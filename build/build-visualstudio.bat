
@echo off
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

goto :endofhelpers
:usage
 	echo: 
 	echo Usage:
 	echo Run select one of the build environments and type of build that will be done
    echo:
 	echo "build-visualstudio.bat vs2017"       Builds VS2017 release binaries
    echo "build-visualstudio.bat vs2015 debug" Builds VS2015 debug binaries
 	echo: 
 	echo Build args:
 	echo "vs2015 || vs2017  (required arg)"
 	echo "debug  || release (defaults to release)"
 	echo "skipmp4           skip building mp4 libraries (must be built for creator / player)"
 	echo "skipcreator       skip building creator app"
 	echo "skipplayer        skip building player libraries"
 	echo:
 	echo Libraries and executables are built to Mp4/lib, Player/Lib and Creator/bin directories.
    exit /B 1

:check
    set temp=0
    if /I %1==%2 set temp=1
    if %temp% EQU 1 (
        set %3=%4
        set %5=YES
    )
    goto :eof

REM %1 src dir %2 vs2015 or vs2017 %3 Release or Debug %4 build directory to create
:cmakebuildvisualstudio
    @echo on
    call :recreatedir %4
    cd %4
    cmake %1 -G %2
    cmake --build . --config %3 -- /m
    cd ..
    @echo off
    goto :eof

:copymp4artifacts
    REM Copy compiled artifact to be available for creator
    IF NOT EXIST ..\lib\Windows\%BUILDTYPE% md ..\lib\Windows\%BUILDTYPE%
    copy !MP4BUILDDIR!\lib\%BUILDTYPE%\mp4vr_shared.* ..\lib\Windows\%BUILDTYPE%
    copy !MP4BUILDDIR!\lib\%BUILDTYPE%\mp4vr_static*.* ..\lib\Windows\%BUILDTYPE%
    copy !MP4BUILDDIR!\lib\%BUILDTYPE%\streamsegmenter_static*.* ..\lib\Windows\%BUILDTYPE%
    goto :eof

:copycreatorartifacts
    REM Copy compiled executables to Creator\bin
    IF NOT EXIST .\bin\Windows\%BUILDTYPE%\ md .\bin\Windows\%BUILDTYPE%
    copy !CREATORBUILDDIR!\bin\%BUILDTYPE%\*.exe .\bin\Windows\%BUILDTYPE%\
    goto :eof

REM %1 VS2015 or VS2017 depending on build
:copydashartifacts
    REM Copy libdash
    IF NOT EXIST ..\..\Lib\Windows\%BUILDTYPE%\%1 md ..\Lib\Windows\%BUILDTYPE%\%1
    copy !LIBDASHBUILDDIR!\%BUILDTYPE%\dash*.* ..\Lib\Windows\%BUILDTYPE%\%1
    goto :eof

:recreatedir
    rmdir /s /q %1
    mkdir %1    
    goto :eof

:endofhelpers

SETLOCAL enableextensions enabledelayedexpansion
set STARTTIME=%TIME%
set ORIGCD=%CD%

set BUILDTYPE=Release

set VS2017=NO
set VS2015=NO

set BUILDMP4=YES
set BUILDCREATOR=YES
set BUILDPLAYER=YES

REM Read args
for %%i in (%*) do (
    SET ISRECOGNIZED=NO
    call :check %%1 debug BUILDTYPE Debug ISRECOGNIZED
    call :check %%1 release BUILDTYPE Release ISRECOGNIZED
    call :check %%1 vs2015 VS2015 YES ISRECOGNIZED
    call :check %%1 vs2017 VS2017 YES ISRECOGNIZED
    call :check %%1 skipmp4 BUILDMP4 NO ISRECOGNIZED
    call :check %%1 skipcreator BUILDCREATOR NO ISRECOGNIZED
    call :check %%1 skipplayer BUILDPLAYER NO ISRECOGNIZED

    if '!ISRECOGNIZED!' == 'NO' (
        echo ERROR: Invalid argument "%%i"
        call :usage
        goto :eof
    )
    shift /1
)

if '%VS2017%%VS2015%' == 'NONO' (
    call :usage
)


if '%VS2015%' == 'YES' (
    cd %ORIGCD%\..\Mp4\build

    if '%BUILDMP4%' == 'YES' (
        SET MP4BUILDDIR=vs2015-%BUILDTYPE%
        call :cmakebuildvisualstudio ../../srcs "Visual Studio 14 2015 Win64" %BUILDTYPE% !MP4BUILDDIR!
        call :copymp4artifacts
    )

    if '%BUILDCREATOR%' == 'YES' (
        cd %ORIGCD%\..\Creator
        SET CREATORBUILDDIR=vs2015-build-%BUILDTYPE%
        call :cmakebuildvisualstudio ../src "Visual Studio 14 2015 Win64" %BUILDTYPE%  !CREATORBUILDDIR!
        call :copycreatorartifacts
    )

    if '%BUILDPLAYER%' == 'YES' (
    	IF EXIST %ORIGCD%\..\..\libdash (
	    	IF EXIST %ORIGCD%\..\OpenGLExt\gl\glext.h (
		    	cd %ORIGCD%\..\..\libdash\build
                SET LIBDASHBUILDDIR=vs2015-build-%BUILDTYPE%
                call :cmakebuildvisualstudio ..\..\libdash\libdash "Visual Studio 14 2015 Win64" %BUILDTYPE% !LIBDASHBUILDDIR!
                call :copydashartifacts VS2015
		    	cd %ORIGCD%\..\Player
			    call build.bat vs2015 %BUILDTYPE%
    		) ELSE (
	    		echo Please download OpenGL extension headers, see OpenGLExt\readme.txt
		    )
    	) ELSE (
	    	echo Please clone libdash to a folder adjacent to the root of this project	
	    )
    )

    cd %ORIGCD%
    goto :eof
)

if '%VS2017%' == 'YES' (

    if '%BUILDMP4%' == 'YES' (
        cd %ORIGCD%\..\Mp4\build
        SET MP4BUILDDIR=vs2017-%BUILDTYPE%
        call :cmakebuildvisualstudio ..\..\srcs "Visual Studio 15 2017 Win64" %BUILDTYPE% !MP4BUILDDIR!
        call :copymp4artifacts
    )

    if '%BUILDCREATOR%' == 'YES' (
        cd %ORIGCD%\..\Creator
        SET CREATORBUILDDIR=vs2017-build-%BUILDTYPE%
        call :cmakebuildvisualstudio ..\src "Visual Studio 15 2017 Win64" %BUILDTYPE% !CREATORBUILDDIR!
        call :copycreatorartifacts
    )

    if '%BUILDPLAYER%' == 'YES' (
    	IF EXIST %ORIGCD%\..\..\libdash (
	    	IF EXIST %ORIGCD%\..\OpenGLExt\gl\glext.h (
		    	cd %ORIGCD%\..\..\libdash\build
                SET LIBDASHBUILDDIR=vs2017-build-%BUILDTYPE%
                call :cmakebuildvisualstudio ..\..\libdash\libdash "Visual Studio 15 2017 Win64" %BUILDTYPE% !LIBDASHBUILDDIR!
                call :copydashartifacts VS2017
		    	cd %ORIGCD%\..\Player
			    call build.bat vs2017 %BUILDTYPE%
    		) ELSE (
	    		echo Please download OpenGL extension headers, see OpenGLExt\readme.txt
		    )
    	) ELSE (
	    	echo Please clone libdash to a folder adjacent to the root of this project	
	    )
    )

    cd %ORIGCD%
    goto :eof
)
