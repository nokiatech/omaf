
@echo off
REM This file is part of Nokia OMAF implementation
REM
REM Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
REM
REM Contact: omaf@nokia.com
REM
REM This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
REM subsidiaries. All rights are reserved.
REM
REM Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
REM written consent of Nokia.

rd /S /Q msvcwin64
md msvcwin64
cd msvcwin64
cmake ..\..\srcs -G "Visual Studio 16 2019" 

cmake --build . --config Debug -- /m 

cmake --build . --config Release -- /m

IF NOT EXIST ..\..\lib\Windows\Debug md ..\..\lib\Windows\Debug

copy lib\Debug\mp4vr_shared.* ..\..\lib\Windows\Debug
copy lib\Debug\mp4vr_static*.* ..\..\lib\Windows\Debug
copy lib\Debug\streamsegmenter_static*.* ..\..\lib\Windows\Debug

IF NOT EXIST ..\..\lib\Windows\Release md ..\..\lib\Windows\Release

copy lib\Release\mp4vr_shared.* ..\..\lib\Windows\Release
copy lib\Release\mp4vr_static*.* ..\..\lib\Windows\Release
copy lib\Release\streamsegmenter_static*.* ..\..\lib\Windows\Release
cd ..
