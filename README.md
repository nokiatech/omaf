[![Build Status](https://travis-ci.org/nokiatech/omaf.svg?branch=master)](https://travis-ci.org/nokiatech/omaf)

# Omnidirectional MediA Format (OMAF)
OMAF is a systems standard developed by the Moving Picture Experts Group (MPEG) for enabling standardized omnidirectional media applications, focusing on 360 video, images and audio. OMAF Creator/Player Engine is an implementation of OMAF standard in order to demonstrate its powerful features and capabilities, and to help achieving interoperability between OMAF implementations.

## News:

## Features:
Nokia OMAF Creator/Player implements a subset of current OMAF version 1 standard, including the following main features:

### Convert 360-degree videos in equirectangular projection stored in conventional mp4 files in HEVC video format to OMAF HEVC viewport independent profile files or DASH streams.
* Input can be identified with filename suffix (_M for mono, _TB for top-bottom framepacked, and _LR for side-by-side framepacked; if no suffix mono is assumed). 
* In addition, mp4 files with [Google Spatial Media metadata](https://github.com/google/spatial-media) and HEVC video can be recognized and accepted as input.
* Output is either a single mp4 file, or DASH live profile MPD and segment files, in segment template mode.
* Both mono and framepacked stereo video in equirectangular projection are supported for file output, however, DASH currently supports only mono output.
* Both OMAF ISOBMFF boxes as well as HEVC omnidirectional video SEI messages are added to the output.
* AAC audio is passed through to the output file as is.

### Playback OMAF Viewport-Independent files and DASH streams (a sample application for Windows provided)
* For mp4 files with HEVC Viewport Independent profile video, both mono and framepacked stereo video are supported. However, DASH streaming supports only mono.
* Viewport dependent scheme with extractor tracks and sub-picture tracks with multiple qualities on same resolution is supported (OMAF spec Annex D.4.2). Viewport dependency is effective only with DASH streaming. Extractor track definition in MPD using both @dependencyId and Preselections descriptor is supported.
* Equirectangular and cubemap projection formats are supported on HEVC viewport dependent profile, however with some limitations.
* Spherical quality ranking is supported to the extend needed for the supported viewport dependent scheme, but 2D region-wise quality ranking is not supported.
* Initial viewing orientation and rotation are supported.
* Only video playback is available, audio track is not played back.
* DASH live profile streams are supported.
* All DASH modes other than segment template mode for OMAF viewport dependent delivery are experimental.
* Bitrate adaptation support is disabled.

All the components compile and run on Windows 10. 
OMAF Player requires in practice an HW accelerated HEVC decoder on Windows, available e.g. with NVidia 10xx display cards. With OpenGL, the playback rate is restricted to 4k@30 fps. With Direct3D, it is set as 6k@60 fps, which is ~8k@33 fps.
In addition to Windows 10, OMAF Creator can be compiled and run on Linux.
Android support for the OMAF Player engine is experimental.

## Contents of the Repository:
This repository contains the following items:
* OMAF Creator command-line application: omafvi.exe
* OMAF Player engine
* OMAF Player sample application
* OMAF MP4 reader and writer library, used by the Creator and Player.

In addition, a LibDASH fork is provided in a separate repository: [LibDASH](https://github.com/nokia/libdash).

## Supported operating systems
* Windows 10 for all components
* Android for the player engine, however there is no sample application for Android included. Linux build is supported for Android.
* Linux for the OMAF Creator.

## Building source:
Cmake-based build scripts are provided. 
A CMake installation is required, minimum v 3.5 and for android 3.6.3 is maximum and recommended
* For Linux gcc 6+ is required
* For Windows, Visual Studio 2015 or 2017 installation is expected
* For Android player build, currently Linux build is best supported. Further, Android NDK must be installed in addition to Android SDK.
* The Creator can be built on Linux, but the player does not support Linux.

First clone the [LibDASH](https://github.com/nokia/libdash) to the root of your work area.
Then clone this repository to the same place, i.e. root of your work area, so that the two projects are adjacent to each other.

Then, in command line shell, go to OMAF/Build
Execute `build-visualstudio.bat` or some other build script depending on your platform. It will echo the detailed usage allowing to select which parts to compile. 

The Creator binary should appear in `/Creator/bin/{OS}/{Debug/Release}/`

The Player libraries should appear in `/Player/Lib/{OS}/{Debug/Release}/`
The Player sample application Visual Studio solution can be opened in `\Player\VideoPlayback\Windows\Monitor_Sample`

When compiling the player to Windows with the provided OpenGL-based sample player application, the OMAF Player engine need to be compiled with OpenGL enabled, and OpenGL extension headers need to be downloaded and placed under OpenGLExt directory, see [Khronos](http://www.opengl.org/registry/)
The player engine as such supports Direct 3D too, and with a D3D player application it can be compiled to D3D without the OpenGL extension headers.
Note that with OpenGL, the max playback rate is restricted to 4k@30 fps.

## Usage:
### Convert a mono 360 mp4 to OMAF mp4:

    omafvi.exe -i file.mp4 -o omaf.mp4

See the Wiki for other converting cases:
https://github.com/nokiatech/omaf/wiki/Usage-instructions-for-OMAF-Creator

### Playback an OMAF file / DASH stream
`\Player\VideoPlayback\Windows\Monitor_Sample\x64\Release\Monitor_sample.exe` 
<br/>You can specify the input name as a command line argument, e.g. 'Monitor_sample.exe video.mp4' or 'Monitor_sample.exe http://myserver.com/omaf/video.mpd'
Default input file/stream is hardcoded in the `source.cpp`, search for // Start playback

## License:
Please see **[LICENSE.TXT](https://github.com/nokiatech/omaf/blob/master/LICENSE.txt)** file for the terms of use of the contents of this repository.

For more information, please contact: <omaf@nokia.com>

### Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies).
### **All rights reserved.** 
