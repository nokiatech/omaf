
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
#include "NVRBandwidthMonitorAndroid.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRLogger.h"
#include "Math/OMAFMathFunctions.h"
#include "Foundation/NVRDeviceInfo.h"
#include <Foundation/Android/NVRAndroid.h>

OMAF_NS_BEGIN

OMAF_LOG_ZONE(BandwidthMonitorAndroid)

static const uint32_t MEASUREMENT_PERIOD_MS = 5 * 1000;
static const uint32_t MEASUREMENT_INTERVAL_MS = MEASUREMENT_PERIOD_MS / SAMPLE_ARRAY_SIZE;

BandwidthMonitorAndroid::BandwidthMonitorAndroid()
    : BandwidthMonitor()
{
    mThread.setPriority(Thread::Priority::LOWEST);
    mThread.setName("BandwidthMonitorAndroid");

    Thread::EntryFunction function;
    function.bind<BandwidthMonitorAndroid, &BandwidthMonitorAndroid::threadEntry>(this);

    mThread.start(function);
}

BandwidthMonitorAndroid::~BandwidthMonitorAndroid()
{
    if (mThread.isValid() && mThread.isRunning())
    {
        mRunning = false;
        mMeasurementEnabled.signal();
        mThread.stop();
        mThread.join();
    }
}

Thread::ReturnValue BandwidthMonitorAndroid::threadEntry(const Thread& thread, void_t* userData)
{
    mJEnv = Android::getJNIEnv();
    mJavaClass = mJEnv->FindClass("android/net/TrafficStats");
    if (mJavaClass == 0)
    {
        OMAF_ASSERT(false, "Can't find the TrafficStats class");
    }
    mMethodId = mJEnv->GetStaticMethodID(mJavaClass, "getTotalRxBytes", "()J");
    if (mMethodId == 0)
    {
        OMAF_ASSERT(false, "Can't find the method in the TrafficStats class");
    }

    int64_t bytes = getBytesDownloaded();
    if (bytes < 0)
    {
        OMAF_LOG_W("TrafficStats not supported on this device, fallbacking to basic method");
        // fallback to the basic way of monitoring, implemented in the base class
        mSupported = false;
        mRunning = false;
    }
    else
    {
        mSupported = true;
        mRunning = true;
    }

    // app-level main loop, exited only if the thread must exit, and never entered if not supported
    while (mRunning)
    {
        // enabled when there is a clip open; do bandwidth estimation once in MEASUREMENT_INTERVAL_MS
        mMeasurementEnabled.wait();
        if (!mRunning)
        {
            break;
        }
        measure();
        Thread::sleep(MEASUREMENT_INTERVAL_MS);
    }

    // Delete local refs
    mJEnv->DeleteLocalRef(mJavaClass);

    return 0;
}

int64_t BandwidthMonitorAndroid::getBytesDownloaded()
{
    jlong value = mJEnv->CallStaticLongMethod(mJavaClass, mMethodId);

    return (int64_t)value;
}



OMAF_NS_END
