
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
#include "VideoDecoder/Android/NVRSurfaceTexture.h"
#include "Foundation/Android/NVRAndroid.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRPerformanceLogger.h"
#include "NVRGraphics.h"
OMAF_NS_BEGIN
OMAF_LOG_ZONE(SurfaceTexture)

SurfaceTexture::SurfaceTexture()
    : mJEnv(NULL)
    , mJavaObject(NULL)
    , mTexture(0)
    , mNanoTimestamp(0)
    , mUpdateTexImageMethodId(NULL)
    , mGetTimestampMethodId(NULL)
    , mSetDefaultBufferSizeMethodId(NULL)
    , mGetTransformMatrixId(NULL)
{
    mJEnv = OMAF::Private::Android::getJNIEnv();

    GLint currentTexture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_EXTERNAL_OES, &currentTexture);

    // Create texture
    glGenTextures(1, &mTexture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, mTexture);

    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, currentTexture);

    // Find class
    const char* className = "android/graphics/SurfaceTexture";
    const jclass surfaceTextureClass = mJEnv->FindClass(className);

    if (surfaceTextureClass == 0)
    {
        OMAF_ASSERT(false, "Class cannot be found");
    }

    // Find constructor
    const jmethodID constructor = mJEnv->GetMethodID(surfaceTextureClass, "<init>", "(I)V");

    if (constructor == 0)
    {
        OMAF_ASSERT(false, "Constructor cannot be found");
    }

    // Create object
    jobject object = mJEnv->NewObject(surfaceTextureClass, constructor, mTexture);

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

    // Get updateTexImage method id
    jmethodID updateTexImageMethodId = mJEnv->GetMethodID(surfaceTextureClass, "updateTexImage", "()V");

    if (!updateTexImageMethodId)
    {
        OMAF_ASSERT(false, "Couldn't get method id for \"updateTexImage\"");
    }

    mUpdateTexImageMethodId = updateTexImageMethodId;

    // Get getTimestamp method id
    jmethodID getTimestampMethodId = mJEnv->GetMethodID(surfaceTextureClass, "getTimestamp", "()J");

    if (!getTimestampMethodId)
    {
        OMAF_ASSERT(false, "Couldn't get method id for \"getTimestamp\"");
    }

    mGetTimestampMethodId = getTimestampMethodId;

    // Create setDefaultBufferSize method id
    jmethodID setDefaultBufferSizeMethodId = mJEnv->GetMethodID(surfaceTextureClass, "setDefaultBufferSize", "(II)V");

    if (!setDefaultBufferSizeMethodId)
    {
        OMAF_ASSERT(false, "Couldn't get method id for \"setDefaultBufferSize\"");
    }

    mSetDefaultBufferSizeMethodId = setDefaultBufferSizeMethodId;

    // Create getTransformMatrix method id
    jmethodID getTransformMatrixId = mJEnv->GetMethodID(surfaceTextureClass, "getTransformMatrix", "([F)V");

    if (!getTransformMatrixId)
    {
        OMAF_ASSERT(false, "Couldn't get method id for \"getTransformMatrix\"");
    }

    mGetTransformMatrixId = getTransformMatrixId;

    // Delete local refs
    mJEnv->DeleteLocalRef(surfaceTextureClass);
}

SurfaceTexture::~SurfaceTexture()
{
    if (mTexture != 0)
    {
        glDeleteTextures(1, &mTexture);
        mTexture = 0;
    }

    if (mJavaObject != NULL)
    {
        JavaVM* javaVM = Android::getJavaVM();
        JNIEnv* jniEnv = NULL;

        int result = javaVM->GetEnv((void**) &jniEnv, JNI_VERSION_1_6);

        if (result != JNI_OK)
        {
            result = javaVM->AttachCurrentThread(&jniEnv, NULL);
        }

        if (result == JNI_OK)
        {
            jniEnv->DeleteGlobalRef(mJavaObject);
        }
        mJavaObject = NULL;
    }

    mJEnv = NULL;
}

void SurfaceTexture::setDefaultBufferSize(int width, int height)
{
    OMAF_ASSERT_NOT_NULL(mJavaObject);

    mJEnv->CallVoidMethod(mJavaObject, mSetDefaultBufferSizeMethodId, width, height);
}

bool_t SurfaceTexture::update()
{
    PerformanceLogger logger("SurfaceTexture::update", 5);
    OMAF_ASSERT_NOT_NULL(mJavaObject);
    OMAF_ASSERT_NOT_NULL(mJEnv);

    // Call updateTextImage
    mJEnv->CallVoidMethod(mJavaObject, mUpdateTexImageMethodId);
    logger.printIntervalTime("updateTexImage");
    // Call getTimestamp
    mNanoTimestamp = mJEnv->CallLongMethod(mJavaObject, mGetTimestampMethodId);

    // Call getTransformMatrix
    jfloatArray jarray = mJEnv->NewFloatArray(16);

    mJEnv->CallVoidMethod(mJavaObject, mGetTransformMatrixId, jarray);

    jfloat* array = mJEnv->GetFloatArrayElements(jarray, NULL);

    mTransformMatrix[0] = array[0];
    mTransformMatrix[1] = array[1];
    mTransformMatrix[2] = array[2];
    mTransformMatrix[3] = array[3];

    mTransformMatrix[4] = array[4];
    mTransformMatrix[5] = array[5];
    mTransformMatrix[6] = array[6];
    mTransformMatrix[7] = array[7];

    mTransformMatrix[8] = array[8];
    mTransformMatrix[9] = array[9];
    mTransformMatrix[10] = array[10];
    mTransformMatrix[11] = array[11];

    mTransformMatrix[12] = array[12];
    mTransformMatrix[13] = array[13];
    mTransformMatrix[14] = array[14];
    mTransformMatrix[15] = array[15];

    mJEnv->ReleaseFloatArrayElements(jarray, array, JNI_COMMIT);

    mJEnv->DeleteLocalRef(jarray);

    return true;
}

unsigned int SurfaceTexture::getTexture()
{
    return mTexture;
}

const float* SurfaceTexture::getTransformMatrix()
{
    return mTransformMatrix;
}

long long SurfaceTexture::getNanoTimestamp()
{
    return mNanoTimestamp;
}

jobject SurfaceTexture::getJavaObject()
{
    return mJavaObject;
}
OMAF_NS_END