
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

@ECHO OFF
set ORIGCD=%CD%
IF EXIST ..\..\libdash (
	IF EXIST ..\OpenGLExt\gl\glext.h (
	cd ..\..\libdash\build
	call build-vs2015.bat
	cd %ORIGCD%\..\Mp4\build\
	call build_win64.bat
	cd %ORIGCD%\..\Player\
	call build.bat vs2015
	) ELSE (
	echo Please download OpenGL extension headers, see OpenGLExt\readme.txt
	)
) ELSE (
echo Please clone libdash to a folder adjacent to the root of this project	
)
cd %ORIGCD%
