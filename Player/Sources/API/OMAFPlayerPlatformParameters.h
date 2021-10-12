
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#pragma once

#include <cstdlib>  // import NULL

#if defined(__ANDROID__)
#include <android/native_activity.h>
#include <EGL/egl.h>
//#include "../../SDK/ApplicationFramework/Android/EGLContext.h"

#elif defined(_WIN64) || defined(_WIN32)

#include <winapifamily.h>
#endif
namespace OMAF
{
#if defined(__ANDROID__)

    struct PlatformParameters
    {
        PlatformParameters()
            : javaVM(NULL)
            , activity(NULL)
            , nativeActivity(NULL)
            , assetManager(NULL)
            , externalStoragePath(NULL)
            , cachePath(NULL)
            , gvrContext(0)
        {
        }

        JavaVM* javaVM;                   ///< Pointer to the JavaVM instance
        jobject activity;                 ///< Pointer to the activity
        ANativeActivity* nativeActivity;  ///< Pointer to the native activity
        ANativeWindow* nativeWindow;        ///< Pointer to the native window
        AAssetManager* assetManager;      ///< Asset manager

        const char* externalStoragePath;  ///< Path to the external storage folder
        const char* cachePath;            ///< Path to the cache

        // Needed only for HMDDeviceGVR
        jlong gvrContext;  ///< GVR context
        EGLDisplay display;
        EGLContext context;
        EGLSurface surface;
    };

#elif defined(_WIN64) || defined(_WIN32)

#if WINAPI_PARTITION_DESKTOP
    struct PlatformParameters
    {
        PlatformParameters()
            : hWnd(NULL)
            , hInstance(NULL)
            , d3dDevice(NULL)
            , storagePath(NULL)
            , assetPath(NULL)
        {
        }

        void* hWnd;       ///< Handle to the window
        void* hInstance;  ///< Application instance

        void* d3dDevice;

        const char* storagePath;  ///< Path to the storage folder
        const char* assetPath;    ///< Path to the assets folder
    };
#elif WINAPI_PARTITION_APP
    struct PlatformParameters
    {
        PlatformParameters()
            : d3dDevice(NULL)
        {
        }
        void* d3dDevice;

        const char* storagePath;  ///< Path to the storage folder
        const char* assetPath;    ///< Path to the assets folder
    };
#elif WINAPI_PARTITION_PC_APP
#error Unsupported platform
#elif WINAPI_PARTITION_PHONE_APP
#error Unsupported platform
#endif

#elif defined(__APPLE__) && defined(__MACH__)

    struct PlatformParameters
    {
        PlatformParameters()
            : storagePath(NULL)
            , gvrContext(NULL)
            , logger(NULL)
        {
        }

        const char* storagePath;  ///< Path to the storage folder

        // Needed only for HMDDeviceGVR
        void* gvrContext;  ///< GVR context
        void* logger;
    };

#else

#error Unsupported platform

#endif
}  // namespace OMAF
