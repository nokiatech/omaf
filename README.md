[![Build Status](https://travis-ci.org/nokiatech/omaf.svg?branch=master)](https://travis-ci.org/nokiatech/omaf)

# Omnidirectional MediA Format (OMAF)
OMAF is a systems standard developed by the Moving Picture Experts Group (MPEG) for enabling standardized omnidirectional media applications, focusing on 360 video, images and audio. OMAF Creator/Player Engine is an implementation of OMAF standard in order to demonstrate its powerful features and capabilities, and to help achieving interoperability between OMAF implementations.

## News:
11-Oct-2018: Version 2.0.0 released! Main changes include:
* OMAF Creator supports creating OMAF HEVC Viewport Dependent profile DASH streams
* OMAF Creator supports creating OMAF Still Image profile files 
* OMAF Player supports OMAF HEVC Viewport Dependent profile DASH streams with more modes than in version 1
* OMAF Player supports playing back OMAF Still Image profile files, including one or more images, and with and without grid.
* Android sample application for OMAF Player is included, operating in handheld mode.

## Features:
Nokia OMAF Creator/Player implements a subset of current OMAF version 1 standard, including the following main features:

### Convert 360-degree HEVC videos in equirectangular projection stored in conventional mp4 files to OMAF HEVC viewport independent profile files or DASH streams.
* Input can be identified with filename suffix (_M for mono, _TB for top-bottom framepacked, and _LR for side-by-side framepacked; if no suffix mono is assumed). 
* In addition, mp4 files with [Google Spatial Media metadata](https://github.com/google/spatial-media) and HEVC video can be recognized and accepted as input.
* Output is either a single mp4 file, or DASH live profile MPD and segment files in segment template mode.
* Both mono and framepacked stereo video in equirectangular projection are supported for file and DASH output.
* Both OMAF ISOBMFF boxes as well as HEVC omnidirectional video SEI messages are added to the output.
* AAC audio is passed through to the output file as is.

### Create 360-degree OMAF HEVC Viewport Dependent profile DASH streams, from HEVC files that have HEVC tiles.
* As this requires rather complicated configuration, json configuration file is used instead of command line. See example-omaf-configuration.json under Creator/doc folder, and examples in https://github.com/nokiatech/omaf/wiki/Usage-instructor-for-OMAF-Creator---Viewport-dependent-mode
* Three viewport dependent modes are supported: Equal-resolution streams with extractor (OMAF Annex D.4.2), Effective 5K ERP (Annex D.6.2/second example), and Effective 6K ERP (Annex D.6.3).
* Output is DASH live profile MPD and segment files, in segment template mode. The Creator can be configured also to output a single mp4 file, but it does not contain all the tracks that the DASH has. E.g. in single resolution mode, it has video tracks only for the best quality case, and in unequal resolution case it has only 1 extractor track (one viewing direction).
* Both mono and framepacked stereo video in equirectangular projection are supported for file and DASH output.
* OMAF signalling in MPD, OMAF ISOBMFF boxes as well as HEVC omnidirectional video SEI messages are supported.
* Multiple representations to achieve bitrate adaptation is not supported.

### Create 360-degree OMAF Still Image profile files, from HEVC files.
* Input is HEVC bitstream, or multiple HEVC bitstreams if image sequence output is generated. 
* Output is a single heic file. 
* Both mono and framepacked stereo in equirectangular projection are supported.

### Playback OMAF HEVC Viewport-Independent and Viewport Dependent files and DASH streams (a sample application for Windows and Android provided)
* Both mono and framepacked stereo video are supported.
* The same viewport dependent schemes as produced by the Creator are supported, in both Equirectangular and Cubemap projection formats.
* Viewport dependency is effective only with DASH streaming. Note! Tile switching latency optimizations can be further optimized.
* Extractor track definition in MPD using Preselections descriptor is supported, @dependencyId is supported also in single resolution case.
* Spherical quality ranking is supported to the extend needed for the supported viewport dependent schemes, but 2D region-wise quality ranking is not supported.
* Initial viewing orientation and rotation are supported on local playback, but on DASH streaming the initial viewing orientation is not yet supported.
* DASH live profile streams are supported. All DASH modes other than segment template mode for OMAF viewport dependent delivery are experimental.
* Bitrate adaptation support is disabled.

### Playback OMAF Still Image files (a sample application for Windows and Android provided)
* Both mono and framepacked stereo images are supported.
* Images in image sequences can be browsed by clicking space (Windows) or touching the screen (Android).
* There are known issues in playing back image sequences with different image sizes on Windows OpenGL mode, and on Android large image grids can cause issues.


All the components compile and run on Windows 10. 
OMAF Player requires in practice an HW accelerated HEVC decoder on Windows, available e.g. with NVidia 10xx display cards. With OpenGL, the playback rate is restricted to 4k@30 fps. With Direct3D, it is set as 6k@60 fps, which is ~8k@33 fps.
In addition to Windows 10, OMAF Creator can be compiled and run on Linux.

## Contents of the Repository:
This repository contains the following items:
* OMAF Creator with the following command-line applications: omafvi.exe, omafvd.exe, and omafimage.exe 
* OMAF Player engine
* OMAF Player sample applications for Windows and Android
* OMAF MP4 reader and writer library, used by the Creator and Player.

In addition, a LibDASH fork is provided in a separate repository: [LibDASH](https://github.com/nokia/libdash), and OMAF branch of the HEIF repository [heif](https://github.com/nokiatech/heif) is utilized as well. 

## Supported operating systems
### OMAF Player
* Windows 10
* Android. Both Windows and Linux builds are supported for Android.

### OMAF Creator
* Windows 10
* Linux

## Building source:
Cmake-based build scripts are provided. 
A CMake installation is required, minimum v 3.5 and for android 3.6.3 is maximum and recommended
* For Linux gcc 6+ is required
* For Windows, Visual Studio 2015 or 2017 installation is expected
* For Android player build, both Windows and Linux can be used. Further, Android NDK (r17) must be installed in addition to Android SDK (28). 
* If building Android on Windows, Ninja.exe build tool is also needed. Download the tool here: https://github.com/ninja-build/ninja/releases and add it to the %PATH% variable.
* The Creator can be built on Linux, and Linux can also be used to build player for Android.

First clone the [LibDASH](https://github.com/nokia/libdash) to the root of your work area.
Then clone the OMAF branch of [HEIF](https://github.com/nokiatech/heif) to the root of your work area.
And finally clone this OMAF repository to the same place, i.e. root of your work area, so that the three cloned repositories are adjacent to each other.
For example:

    `mkdir omaf-build`
    `cd omaf-build`
    `git clone https://github.com/nokia/libdash`
    `git clone --single-branch -b OMAF https://github.com/nokiatech/heif.git`
    `git clone https://github.com/nokiatech/omaf.git`

Build libdash as instructed in its README.md and then go to `OMAF/Build`.
Execute `build-visualstudio.bat` or some other build script depending on your platform. It will echo the detailed usage allowing to select which parts to compile. 

The Creator binary should appear in `/Creator/bin/{OS}/{Debug/Release}/`

The Player libraries should appear in `/Player/Lib/{OS}/{Debug/Release}/`
The Player sample application Visual Studio solution can be opened in `\Player\VideoPlayback\Windows\Monitor_Sample` and the sample for Android from
`Player\VideoPlayback\Android\Handheld\`.

When compiling the player to Windows with the provided OpenGL-based sample player application, the OMAF Player engine need to be compiled with OpenGL enabled, and OpenGL extension headers need to be downloaded and placed under OpenGLExt directory, see [Khronos](http://www.opengl.org/registry/)
The player engine supports Direct 3D too, and with a D3D player application it can be compiled to D3D without the OpenGL extension headers.
Note that with OpenGL, the max playback rate is restricted to 4k@30 fps.

## Usage:
### Convert a mono 360 mp4 to OMAF mp4:

    omafvi.exe -i file.mp4 -o omaf.mp4

See the Wiki for other viewport independent conversion cases:
https://github.com/nokiatech/omaf/wiki/Usage-instructions-for-OMAF-Creator

### Create a Viewport dependent profile DASH stream:

    omafvd.exe config.json

See the Wiki for explanation of the json configuration for viewport dependent stream generation:
https://github.com/nokiatech/omaf/wiki/Usage-instructor-for-OMAF-Creator---Viewport-dependent-mode

### Create OMAF Still image profile heic files:

    omafimage.exe -s image.265 -o image.heic

See the Wiki for more detailed explanation of OMAF Still Image creation:
https://github.com/nokiatech/omaf/wiki/Usage-instructions-for-OMAF-Creator-Still-Image-file-creation

### Playback an OMAF file / DASH stream on Windows
`\Player\VideoPlayback\Windows\Monitor_Sample\x64\Release\Monitor_sample.exe` 
<br/>You can specify the input name as a command line argument, e.g. `Monitor_sample.exe storage://video.mp4` or `Monitor_sample.exe http://myserver.com/omaf/video.mpd`
<br/>Default input file/stream is hardcoded in the `source.cpp`, search for `// Replace the path`

### Playback an OMAF file / DASH stream on Android
Open the following project in Android Studio:
`\Player\VideoPlayback\Android\Handheld\` 
<br/>Default input file/stream is hardcoded in the `ApplicationJNI.cpp`, search for `FILE_URI`
If you want to play a local mp4 file, start the URI with 'asset://', and place the file in the assets directory (`\Player\VideoPlayback\Android\Handheld\Assets\`)
To play a DASH stream, just assign the http URL to the 'FILE_URI'

## License:
Please see **[LICENSE.TXT](https://github.com/nokiatech/omaf/blob/master/LICENSE.txt)** file for the terms of use of the contents of this repository.

For more information, please contact: <omaf@nokia.com>

### Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies).
### **All rights reserved.** 
