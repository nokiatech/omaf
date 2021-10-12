
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

set -x

# build / make sure that compiling environment is uptodate
cd docker-env
docker build -f Dockerfile-ubuntu -t android-compile-env .

# git checkout on windows with some configuration seem to mess up line endings, so lets fix line endings first
cd ../..
docker run --rm -t -i -v $PWD:/MP4VR -w /MP4VR/build/android android-compile-env dos2unix build.sh

# run build command
docker run --rm -t -i -e BUILD_SINGLE=${BUILD_SINGLE:-0} -v $PWD:/MP4VR -w /MP4VR/build/android android-compile-env ./build.sh
