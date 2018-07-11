
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

set -x

# build / make sure that compiling environment is uptodate
cd docker-env
docker build -f Dockerfile-ubuntu -t android-compile-env .

# create build directories for linux build
cd ..

if [ "$BUILD_DEBUG" = "1" ]; then
  mkdir -p linux/debug
  docker run --rm -t -i -v $PWD/..:/MP4VR -w /MP4VR/build/linux/debug android-compile-env cmake ../../../srcs -DCMAKE_BUILD_TYPE=Debug
  docker run --rm -t -i -v $PWD/..:/MP4VR -w /MP4VR/build/linux/debug android-compile-env cmake --build . -- -j8
else
  mkdir -p linux/release
  docker run --rm -t -i -v $PWD/..:/MP4VR -w /MP4VR/build/linux/release android-compile-env cmake ../../../srcs -DCMAKE_BUILD_TYPE=Release
  docker run --rm -t -i -v $PWD/..:/MP4VR -w /MP4VR/build/linux/release android-compile-env cmake --build . -- -j8
fi