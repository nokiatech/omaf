
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
#include "NVRBandwidthMonitorAndroid.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRLogger.h"
#include "Math/OMAFMathFunctions.h"
#include "Foundation/NVRDeviceInfo.h"
#include <Foundation/Android/NVRAndroid.h>

OMAF_NS_BEGIN

OMAF_LOG_ZONE(BandwidthMonitorAndroid)

static const uint32_t MEASUREMENT_PERIOD_MS = 10 * 1000;
static const uint32_t MEASUREMENT_INTERVAL_MS = MEASUREMENT_PERIOD_MS / SAMPLE_ARRAY_SIZE; // this need to be rather low, e.g. 500 msec, to low-pass filter the bandwidth measurements
static const size_t INCLUDED_EVENT_COUNT = 3;           // # of largest events included in estimation
static const uint32_t MIN_PEAK_BPS = 1024*1024; // we need megabps for operation; filter out possible small peaks especially during pause

BandwidthMonitorAndroid::BandwidthMonitorAndroid()
    : mCurrentIndex(0)
    , mMeasurementEnabled(true, false)
    , mRunning(false)
    , mSupported(false)
    , mSamplesStored(0)
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

    uint64_t bytes = getBytesDownloaded();
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

void_t BandwidthMonitorAndroid::doSetMode(bool_t aEnable)
{
    if (!mSupported)
    {
        return BandwidthMonitor::doSetMode(aEnable);
    }
    if (aEnable)
    {
        // reset the array, as the old samples from previous bitrate should not be used any more
        Spinlock::ScopeLock lock(mListLock);

        mDownloadSamples.clear();
        mCurrentIndex = 0;

        for (size_t i = 0; i < mDownloadSamples.getCapacity() - 1; i++)
        {
            DownloadSample dlSample;
            dlSample.byteCountCumulative = 0;
            dlSample.bitsPerSecond = 0;
            dlSample.timestampInMS = 0;
            mDownloadSamples.add(dlSample);
        }

        mSamplesStored = 0;
        mMeasurementEnabled.signal();
    }
    else
    {
        mSamplesStored = 0;
        mMeasurementEnabled.reset();
    }
}

bool_t BandwidthMonitorAndroid::doCanMonitorParallelDownloads()
{
    return mSupported;
}

int64_t BandwidthMonitorAndroid::getBytesDownloaded()
{
    jlong value = mJEnv->CallStaticLongMethod(mJavaClass, mMethodId);

    return (int64_t)value;
}


void_t BandwidthMonitorAndroid::measure()
{
	int64_t bytesDownloaded = getBytesDownloaded();
    uint32_t now = Time::getClockTimeMs();

    Spinlock::ScopeLock lock(mListLock);

    int64_t bitsPerSecond = 0;
    if (mSamplesStored > 0)
    {
        int64_t delta = bytesDownloaded - mDownloadSamples.at(mCurrentIndex).byteCountCumulative;
        uint32_t period = now - mDownloadSamples.at(mCurrentIndex).timestampInMS;
        bitsPerSecond = (delta * 8) / period * 1000;
        OMAF_LOG_V("%d downloaded %lld bytes - computed bps %lld", now, delta,
                  bitsPerSecond);

        if (++mCurrentIndex == mDownloadSamples.getSize())
        {
            mCurrentIndex = 0;
        }
    }
    // else we are recording the reference sample

    mDownloadSamples.at(mCurrentIndex).byteCountCumulative = bytesDownloaded;
	mDownloadSamples.at(mCurrentIndex).bitsPerSecond = (uint32_t)bitsPerSecond;
    mDownloadSamples.at(mCurrentIndex).timestampInMS = now;

    mSamplesStored++;
}

uint32_t BandwidthMonitorAndroid::estimateBandWidth()
{
    if (!mSupported)
    {
        // use the parent class instead
        return BandwidthMonitor::estimateBandWidth();
    }
    Spinlock::ScopeLock lock(mListLock);

    if (mSamplesStored < 2)//tunable magic number
    {
        OMAF_LOG_V("Not enough data, skip");
        return 0;
    }
    // find N largest samples (peaks)
    uint32_t smallest = MIN_PEAK_BPS;
    DownloadSamples peaks;

    for (DownloadSamples::ConstIterator it = mDownloadSamples.begin(); it != mDownloadSamples.end(); ++it)
    {
        if ((*it).bitsPerSecond > smallest)
        {
            size_t count = min(INCLUDED_EVENT_COUNT, peaks.getSize());
            bool_t added = false;

            for (size_t i = 0; i < count; i++)
            {
                if (peaks.at(i).bitsPerSecond < (*it).bitsPerSecond)
                {
                    peaks.add(*it, i);
                    if (peaks.getSize() > INCLUDED_EVENT_COUNT)
                    {
                        peaks.removeAt(INCLUDED_EVENT_COUNT);
                    }
                    added = true;
                    break;
                }
            }
            if (peaks.getSize() < INCLUDED_EVENT_COUNT && !added)
            {
                // new small event, add to the end
                peaks.add(*it);
            }
            smallest = peaks.back().bitsPerSecond;
        }
    }

    for (DownloadSamples::ConstIterator it = peaks.begin(); it != peaks.end(); ++it)
    {
        OMAF_LOG_V("BW peak %d at %d", (*it).bitsPerSecond, (*it).timestampInMS);
    }

    if (peaks.getSize() > 0)
    {
        // take median of the bitrates - except if the largest one is newer than median, then average the two
        size_t median = min(peaks.getSize() / 2, INCLUDED_EVENT_COUNT / 2);
        uint32_t bps = 0;
        if (peaks.at(0).timestampInMS > peaks.at(median).timestampInMS)
        {
            // the largest value is newer than the median, and hence sounds relevant, average the two
            bps = (peaks.at(0).bitsPerSecond + peaks.at(median).bitsPerSecond)/2;
        }
        else if (median > 0 && peaks.getSize() % 2 == 0)
        {
            // even # of values, median is the average of the two middle ones
            bps = (peaks.at(median).bitsPerSecond + peaks.at(median-1).bitsPerSecond)/2;
        }
        else
        {
            // odd # of values, median is the middle one
            bps = peaks.at(median).bitsPerSecond;
        }
        return bps;
    }
    else
    {
        return 0;
    }
}

OMAF_NS_END
