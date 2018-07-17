
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

mkdir ..\build-vs2015
cd ..\build-vs2015 || exit
IF EXIST CMakeCache.txt del CMakeCache.txt
IF EXIST CMakeFiles rmdir /S/Q CMakeFiles
IF EXIST build.bat del build.bat
copy ..\build\build.bat build.bat
cmake ..\src -G"Visual Studio 14 2015 Win64"
cmake --build . --config Debug -- /m
cmake --build . --config Release -- /m


IF NOT EXIST ..\bin\Windows\Debug\ md ..\bin\Windows\Debug
copy bin\Debug\*.exe ..\bin\Windows\Debug\
IF NOT EXIST ..\bin\Windows\Release\ md ..\bin\Windows\Release
copy bin\Release\*.exe ..\bin\Windows\Release\
cd ..
