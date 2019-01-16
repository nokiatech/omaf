
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

cmake_minimum_required(VERSION 3.2 FATAL_ERROR)

if(${CMAKE_EXTRA_GENERATOR} MATCHES "Eclipse CDT4")
    set(CMAKE_ECLIPSE_VERSION "4.4" CACHE STRING "Eclipse version" FORCE)
    set(CMAKE_CXX_COMPILER_ARG1 "-std=c++11" CACHE STRING "C++ version for eclipse" FORCE)
endif()

project( common LANGUAGES CXX )

set(COMMON_SRCS
    customallocator.cpp
    avcparser.cpp
    audiosampleentrybox.cpp
    avcconfigurationbox.cpp
    avcdecoderconfigrecord.cpp
    avcsampleentry.cpp
    bbox.cpp
    bitstream.cpp
    brandandcompatiblebrandsbasebox.cpp
    channellayoutbox.cpp
    chunkoffsetbox.cpp
    cleanaperturebox.cpp
    compatibleschemetypebox.cpp
    compositionoffsetbox.cpp
    compositiontodecodebox.cpp
    coverageinformationbox.cpp
    datainformationbox.cpp
    datareferencebox.cpp
    decodepts.cpp
    editbox.cpp
    elementarystreamdescriptorbox.cpp
    filetypebox.cpp
    fourccint.cpp
    fullbox.cpp
    googlecubemapprojectionbox.cpp
    googleequirectangularprojectionbox.cpp
    googlenondiegeticaudiobox.cpp
    googleprojectiondatabox.cpp
    googleprojectionbox.cpp
    googleprojectionheaderbox.cpp
    googlespatialaudiobox.cpp
    googlesphericalvideov1box.cpp
    googlesphericalvideov2box.cpp
    googlesphericalvideoheaderbox.cpp
    googlestereoscopic3dbox.cpp
    handlerbox.cpp
    hevcconfigurationbox.cpp
    hevcdecoderconfigrecord.cpp
    hevcsampleentry.cpp
    initialviewingorientationsampleentry.cpp
    itemdatabox.cpp
    iteminfobox.cpp
    itemlocationbox.cpp
    itemprotectionbox.cpp
    itemreferencebox.cpp
    log.cpp
    mediabox.cpp
    mediadatabox.cpp
    mediaheaderbox.cpp
    mediainformationbox.cpp
    metabox.cpp
    metadatasampleentrybox.cpp
    moviebox.cpp
    movieextendsbox.cpp
    movieextendsheaderbox.cpp
    moviefragmentbox.cpp
    moviefragmentheaderbox.cpp
    movieheaderbox.cpp
    mp4audiodecoderconfigrecord.cpp
    mp4audiosampleentrybox.cpp
    mp4visualsampleentrybox.cpp
    nalutil.cpp
    nullmediaheaderbox.cpp
    originalformatbox.cpp
    primaryitembox.cpp
    projectedomnivideobox.cpp
    projectionformatbox.cpp
    protectionschemeinfobox.cpp
    regionwisepackingbox.cpp
    restrictedschemeinfobox.cpp
    rotationbox.cpp
    sampledescriptionbox.cpp
    sampleentrybox.cpp
    samplesizebox.cpp
    sampletablebox.cpp
    sampletochunkbox.cpp
    sampletogroupbox.cpp
    samplegroupdescriptionbox.cpp
    samplegroupentry.cpp
    samplingratebox.cpp
    schemetypebox.cpp
    segmentindexbox.cpp
    segmenttypebox.cpp
    soundmediaheaderbox.cpp
    sphereregionconfigbox.cpp
    sphereregionsampleentrybox.cpp
    stereovideobox.cpp
    syncsamplebox.cpp
    timetosamplebox.cpp
    trackbox.cpp
    trackextendsbox.cpp
    trackfragmentbasemediadecodetimebox.cpp
    trackfragmentbox.cpp
    trackfragmentheaderbox.cpp
    trackgroupbox.cpp
    trackgrouptypebox.cpp
    trackheaderbox.cpp
    trackreferencebox.cpp
    trackreferencetypebox.cpp
    trackrunbox.cpp
    tracktypebox.cpp
    uribox.cpp
    uriinitbox.cpp
    urimetasampleentrybox.cpp
    userdatabox.cpp
    videomediaheaderbox.cpp
    visualsampleentrybox.cpp
    xmlbox.cpp
    )

set(COMMON_HDRS
    customallocator.hpp
    avcparser.hpp
    androidhacks.hpp
    audiosampleentrybox.hpp
    avccommondefs.hpp
    avcconfigurationbox.hpp
    avcdecoderconfigrecord.hpp
    avcsampleentry.hpp
    bbox.hpp
    bitstream.hpp
    brandandcompatiblebrandsbasebox.hpp
    channellayoutbox.hpp
    chunkoffsetbox.hpp
    cleanaperturebox.hpp
    commontypes.hpp
    compatibleschemetypebox.hpp
    compositionoffsetbox.hpp
    compositiontodecodebox.hpp
    coverageinformationbox.hpp
    datainformationbox.hpp
    datareferencebox.hpp
    decodepts.hpp
    decoderconfigrecord.hpp
    editbox.hpp
    elementarystreamdescriptorbox.hpp
    filetypebox.hpp
    fourccint.hpp
    fullbox.hpp
    googlecubemapprojectionbox.hpp
    googleequirectangularprojectionbox.hpp
    googlenondiegeticaudiobox.hpp
    googleprojectiondatabox.hpp
    googleprojectionbox.hpp
    googleprojectionheaderbox.hpp
    googlespatialaudiobox.hpp
    googlesphericalvideov1box.hpp
    googlesphericalvideov2box.hpp
    googlesphericalvideoheaderbox.hpp
    googlestereoscopic3dbox.hpp
    handlerbox.hpp
    hevccommondefs.hpp
    hevcconfigurationbox.hpp
    hevcdecoderconfigrecord.hpp
    hevcsampleentry.hpp
    initialviewingorientationsampleentry.hpp
    itemdatabox.hpp
    iteminfobox.hpp
    itemlocationbox.hpp
    itemprotectionbox.hpp
    itemreferencebox.hpp
    log.hpp
    mediabox.hpp
    mediadatabox.hpp
    mediaheaderbox.hpp
    mediainformationbox.hpp
    metabox.hpp
    metadatasampleentrybox.hpp
    moviebox.hpp
    movieextendsbox.hpp
    movieextendsheaderbox.hpp
    moviefragmentbox.hpp
    moviefragmentheaderbox.hpp
    moviefragmentsdatatypes.hpp
    movieheaderbox.hpp
    mp4audiodecoderconfigrecord.hpp
    mp4audiosampleentrybox.hpp
    mp4visualsampleentrybox.hpp
    nullmediaheaderbox.hpp
    originalformatbox.hpp
    primaryitembox.hpp
    projectedomnivideobox.hpp
    projectionformatbox.hpp
    protectionschemeinfobox.hpp
    regionwisepackingbox.hpp
    restrictedschemeinfobox.hpp
    rotationbox.hpp
    sampledescriptionbox.hpp
    sampleentrybox.hpp
    samplegroupdescriptionbox.hpp
    samplegroupentry.hpp
    samplesizebox.hpp
    sampletablebox.hpp
    sampletochunkbox.hpp
    sampletogroupbox.hpp
    samplingratebox.hpp
    schemetypebox.hpp
    segmentindexbox.hpp
    segmenttypebox.hpp
    smallvector.hpp
    soundmediaheaderbox.hpp
    sphereregionconfigbox.hpp
    sphereregionsampleentrybox.hpp
    stereovideobox.hpp
    syncsamplebox.hpp
    timetosamplebox.hpp
    trackbox.hpp
    trackextendsbox.hpp
    trackfragmentbasemediadecodetimebox.hpp
    trackfragmentbox.hpp
    trackfragmentheaderbox.hpp
    trackgroupbox.hpp
    trackgrouptypebox.hpp
    trackheaderbox.hpp
    trackreferencebox.hpp
    trackreferencetypebox.hpp
    trackrunbox.hpp
    tracktypebox.hpp
    uribox.hpp
    uriinitbox.hpp
    urimetasampleentrybox.hpp
    userdatabox.hpp
    videomediaheaderbox.hpp
    visualsampleentrybox.hpp
    writeoncemap.hpp
    xmlbox.hpp
    )

add_library(common_for_lib OBJECT ${COMMON_SRCS} ${COMMON_HDRS})
set_property(TARGET common_for_lib PROPERTY CXX_STANDARD 11)
target_compile_definitions(common_for_lib PRIVATE "MP4VR_BUILDING_LIB")
# target_compile_definitions(common_for_lib PUBLIC "MP4VR_USE_STATIC_LIB")
if (IOS)
    set_xcode_property(common_for_lib IPHONEOS_DEPLOYMENT_TARGET "10.0")
endif(IOS)

# Use the same objects for the shared library
set_property(TARGET common_for_lib PROPERTY POSITION_INDEPENDENT_CODE 1)

if (NOT IOS)
  add_library(common_for_dll OBJECT ${COMMON_SRCS} ${COMMON_HDRS})
  set_property(TARGET common_for_dll PROPERTY CXX_STANDARD 11)
  target_compile_definitions(common_for_dll PRIVATE "MP4VR_BUILDING_LIB")
#  target_compile_definitions(common_for_dll PUBLIC "MP4VR_USE_STATIC_LIB")
  # target_compile_definitions(common_for_dll PRIVATE "MP4VR_BUILDING_DLL")
  # Use the same objects for the shared library
  set_property(TARGET common_for_dll PROPERTY POSITION_INDEPENDENT_CODE 1)
endif(NOT IOS)

