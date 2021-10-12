
#!/usr/bin/bash
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

mkdir -p ../build-linux
cd ../build-linux
rm -f CMakeCache.txt build.sh
rm -rf CMakeFiles
cp ../build/build.sh build.sh
cmake ../src -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug 
## USE265=TRUE enables x265 support
## USEVAAPI=TRUE enables VAAPI support
#cmake ../src -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_BUILD_TYPE=Debug -DUSEX265=TRUE -DUSEVAAPI=TRUE
cmake --build . -- -j$(nproc)
