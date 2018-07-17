
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

rd /S /Q wineclipse
md wineclipse
cd wineclipse
cmake ..\..\srcs -G "Eclipse CDT4 - MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE -DCMAKE_ECLIPSE_MAKE_ARGUMENTS="-j4 -O"