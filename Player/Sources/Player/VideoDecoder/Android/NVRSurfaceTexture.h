
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
#include "Graphics/NVRDependencies.h"

OMAF_NS_BEGIN
class SurfaceTexture
{
	public:

		SurfaceTexture();
		~SurfaceTexture();

		void setDefaultBufferSize(int width, int height);
		bool_t update();
		const float* getTransformMatrix();
		long long getNanoTimestamp();
		unsigned int getTexture();
		jobject	getJavaObject();

	private:

		JNIEnv* mJEnv;
		jobject	mJavaObject;

		GLuint mTexture;
		long long mNanoTimestamp;
		float mTransformMatrix[16];

		jmethodID mUpdateTexImageMethodId;
		jmethodID mGetTimestampMethodId;
		jmethodID mSetDefaultBufferSizeMethodId;
		jmethodID mGetTransformMatrixId;
};
OMAF_NS_END