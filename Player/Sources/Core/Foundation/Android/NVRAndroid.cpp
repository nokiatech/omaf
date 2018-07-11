
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Foundation/Android/NVRAndroid.h"

#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(NVRAndroid)

    namespace Android
    {
        static JavaVM* mJavaVM = OMAF_NULL;

        static jobject mActivity = OMAF_NULL;
        static ANativeActivity* mNativeActivity = OMAF_NULL;

        static AAssetManager* mAssetManager = OMAF_NULL;

        static PathName mExternalStoragePath = OMAF_NULL;
        static PathName mCachePath = OMAF_NULL;

        void_t initialize(OMAF::PlatformParameters* platformParameters)
        {
            mJavaVM = platformParameters->javaVM;

            mActivity = platformParameters->activity;
            mNativeActivity = platformParameters->nativeActivity;

            if (mNativeActivity)
            {
                //Yes, mNativeActivity is a normal Activity.
                mActivity = mNativeActivity->clazz;
            }

            mAssetManager = platformParameters->assetManager;

            mExternalStoragePath = platformParameters->externalStoragePath;
            mExternalStoragePath.append("/");

            mCachePath = platformParameters->cachePath;
            mCachePath.append("/");
        }

        jobject getActivity()
        {
            OMAF_ASSERT(mActivity != NULL, "Activity has not been initialized");

            return mActivity;
        }

        ANativeActivity* getNativeActivity()
        {
            OMAF_ASSERT(mNativeActivity != NULL, "Native activity has not been initialized");

            return mNativeActivity;
        }

        JavaVM* getJavaVM()
        {
            OMAF_ASSERT(mJavaVM != NULL, "JavaVM has not been initialized");

            return mJavaVM;
        }

        void_t attachThread()
        {
            OMAF_ASSERT_NOT_NULL(mJavaVM);

            JNIEnv* jniEnv;

            int result = mJavaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);

            if (result != JNI_OK)
            {
                result = mJavaVM->AttachCurrentThread(&jniEnv, NULL);
            }

            if (result != JNI_OK)
            {
                OMAF_LOG_E("Failed to attach current thread.");
                OMAF_ASSERT(false, "");
            }
        }

        JNIEnv* getJNIEnv()
        {
            OMAF_ASSERT_NOT_NULL(mJavaVM);

            JNIEnv* jniEnv;

            int result = mJavaVM->GetEnv((void**)&jniEnv, JNI_VERSION_1_6);

            if (result != JNI_OK)
            {
                OMAF_LOG_E("JNIEnv is not valid.");
                OMAF_ASSERT(false, "");
            }

            return jniEnv;
        }

        void_t detachThread()
        {
            OMAF_ASSERT_NOT_NULL(mJavaVM);

            mJavaVM->DetachCurrentThread();
        }

        AAssetManager* getAssetManager()
        {
            OMAF_ASSERT(mAssetManager != NULL, "Asset manager has not been initialized");

            return mAssetManager;
        }

        const PathName getExternalStoragePath()
        {
            OMAF_ASSERT(!mExternalStoragePath.isEmpty(), "Storage path has not been initialized");

            return mExternalStoragePath;
        }

        const PathName getInternalStoragePath()
        {
            OMAF_ASSERT(!mCachePath.isEmpty(), "Storage path has not been initialized");

            return mCachePath;
        }
    }
OMAF_NS_END
