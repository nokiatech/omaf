
#!/usr/bin/bash
#
# This file is part of Nokia OMAF implementation
#
# Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
#
# Contact: omaf@nokia.com
#
# This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
# subsidiaries. All rights are reserved.
#
# Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
# written consent of Nokia.
#

if [ "$OS" = "Windows_NT" ]; then
    WIN=true
else
    WIN=false
fi
if $WIN; then
    compiler=msvc2015
else
    compiler="linux-gcc"$(gcc --version | grep -o '[0-9.]*' | head -1)
fi
ROOT=$PWD
version=$(git describe --tags)
zipname="$ROOT/mp4vr-${version}-$(uname -m)-${compiler}.zip"
set -x
set -e
if $WIN; then
    cd lib
    if [ -d Debug ]; then
	cd Debug
    else
	cd Release
    fi
    zip "$zipname" mp4vr_static.lib
    cd ..
else
    cd release
    cd lib
    zip "$zipname" libmp4vr_static.a
    cd ..
fi
cd ../..
zip "$filename" README.md
echo "Created $zipname"
