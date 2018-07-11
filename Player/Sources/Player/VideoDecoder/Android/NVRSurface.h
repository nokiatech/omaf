
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
#pragma once

#include <android/native_window_jni.h>

#include "NVREssentials.h"

#include "VideoDecoder/Android/NVRSurfaceTexture.h"
OMAF_NS_BEGIN
class Surface
{
public:

	Surface();
	~Surface();

	SurfaceTexture* getSurfaceTexture();

	jobject	getJavaObject();

private:

	JNIEnv* mJEnv;
	jobject	mJavaObject;

	SurfaceTexture* mSurfaceTexture;
};
OMAF_NS_END
