
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

#include <Foundation/NVREvent.h>
#include "NVREssentials.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

class BandwidthMonitorAndroid : public BandwidthMonitor
{
public: 
    BandwidthMonitorAndroid();
    ~BandwidthMonitorAndroid();
protected:

	int64_t getBytesDownloaded();

private:
    Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);
	
private:

    JNIEnv* mJEnv;
	jclass	mJavaClass;
	jmethodID mMethodId;

};

OMAF_NS_END