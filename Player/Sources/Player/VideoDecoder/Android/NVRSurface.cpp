
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
#include "VideoDecoder/Android/NVRSurface.h"
#include "VideoDecoder/Android/NVRSurfaceTexture.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"
#include "Foundation/Android/NVRAndroid.h"

OMAF_NS_BEGIN
Surface::Surface()
: mJEnv(NULL)
, mJavaObject(NULL)
{
    mJEnv = OMAF::Private::Android::getJNIEnv();
    mSurfaceTexture = OMAF_NEW_HEAP(SurfaceTexture);
    // Find class
    const char *className = "android/view/Surface";
    const jclass surfaceClass = mJEnv->FindClass(className);

    if (surfaceClass == 0)
    {
        OMAF_ASSERT(false, "Class cannot be found");
    }

    // Find constructor
    const jmethodID constructor = mJEnv->GetMethodID(surfaceClass, "<init>", "(Landroid/graphics/SurfaceTexture;)V");

    if (constructor == 0)
    {
        OMAF_ASSERT(false, "Constructor cannot be found");
    }

    // Create object
    jobject surfaceTextureObject = mSurfaceTexture->getJavaObject();
    jobject object = mJEnv->NewObject(surfaceClass, constructor, surfaceTextureObject);

    if (object == 0)
    {
        OMAF_ASSERT(false, "Object creation failed");
    }

    // Create global ref
    jobject javaObject = mJEnv->NewGlobalRef(object);

    if (javaObject == 0)
    {
        OMAF_ASSERT(false, "Object ref failed");
    }

    mJavaObject = javaObject;

    // Delete local ref
    mJEnv->DeleteLocalRef(object);

    // Delete local refs
    mJEnv->DeleteLocalRef(surfaceClass);
}

Surface::~Surface()
{
    OMAF_DELETE_HEAP(mSurfaceTexture);
    mSurfaceTexture = NULL;

    if (mJavaObject != NULL)
    {
        OMAF::Private::Android::attachThread();
        mJEnv = OMAF::Private::Android::getJNIEnv();
        mJEnv->DeleteGlobalRef(mJavaObject);
        mJavaObject = NULL;
    }
}

SurfaceTexture* Surface::getSurfaceTexture()
{
    return mSurfaceTexture;
}

jobject Surface::getJavaObject()
{
    return mJavaObject;
}
OMAF_NS_END