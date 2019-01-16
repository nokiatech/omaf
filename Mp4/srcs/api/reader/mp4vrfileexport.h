
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#ifndef MP4VRFILEEXPORT_H
#define MP4VRFILEEXPORT_H

#if (defined(MP4VR_BUILDING_LIB) && !defined(MP4VR_BUILDING_DLL)) || defined(MP4VR_USE_STATIC_LIB)
#define MP4VR_DLL_PUBLIC
#define MP4VR_DLL_LOCAL
#else
#if defined _WIN32 || defined __CYGWIN__
#ifdef MP4VR_BUILDING_DLL
#ifdef __GNUC__
#define MP4VR_DLL_PUBLIC __attribute__((dllexport))
#else
#define MP4VR_DLL_PUBLIC __declspec(dllexport)  // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define MP4VR_DLL_PUBLIC __attribute__((dllimport))
#else
#define MP4VR_DLL_PUBLIC __declspec(dllimport)  // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define MP4VR_DLL_LOCAL
#else
#if __GNUC__ >= 4
#define MP4VR_DLL_PUBLIC __attribute__((visibility("default")))
#define MP4VR_DLL_LOCAL __attribute__((visibility("hidden")))
#else
#define MP4VR_DLL_PUBLIC
#define MP4VR_DLL_LOCAL
#endif
#endif
#endif

#endif  // MP4VRFILEEXPORT_H
