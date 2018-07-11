
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

#include <Foundation/NVREvent.h>
#include "NVREssentials.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

static const uint32_t SAMPLE_ARRAY_SIZE = 40;

class BandwidthMonitorAndroid : public BandwidthMonitor
{
public: 
    BandwidthMonitorAndroid();
    ~BandwidthMonitorAndroid();
protected:

    void_t doSetMode(bool_t aEnable);
	bool_t doCanMonitorParallelDownloads();
    uint32_t estimateBandWidth();

private:
    void_t measure();
    Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);
	int64_t getBytesDownloaded();
	
private:
    Spinlock mListLock;

    struct DownloadSample
    {
		uint64_t byteCountCumulative;
        uint32_t bitsPerSecond;
        uint32_t timestampInMS;
    };
    typedef FixedArray<DownloadSample, SAMPLE_ARRAY_SIZE> DownloadSamples;
    DownloadSamples mDownloadSamples;
    size_t mCurrentIndex;
    size_t mSamplesStored;

    Thread mThread;
    Event mMeasurementEnabled;
    bool_t mRunning;
    bool_t mSupported;

    JNIEnv* mJEnv;
	jclass	mJavaClass;
	jmethodID mMethodId;

	
};

OMAF_NS_END