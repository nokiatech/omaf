
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
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRTime.h"
#include "Math/OMAFMathFunctions.h"
#if OMAF_PLATFORM_ANDROID
#include "Foundation/Android/NVRBandwidthMonitorAndroid.h"
#else
#include "Foundation/Windows/NVRBandwidthMonitorWindows.h"
#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(BandwidthMonitor)

BandwidthMonitor* BandwidthMonitor::sInstance = OMAF_NULL;

BandwidthMonitor* BandwidthMonitor::getInstance()
{
    if (sInstance == OMAF_NULL)
    {
#if OMAF_PLATFORM_ANDROID
        sInstance = OMAF_NEW_HEAP(BandwidthMonitorAndroid);
#else
        sInstance = OMAF_NEW_HEAP(BandwidthMonitorWindows);
#endif
    }
    return sInstance;
}

void_t BandwidthMonitor::destroyInstance()
{
    if (sInstance != OMAF_NULL)
    {
        OMAF_DELETE_HEAP(sInstance);
        sInstance = OMAF_NULL;
    }
}

BandwidthMonitor::BandwidthMonitor()
    : mCurrentIndex(0)
    , mMeasurementEnabled(true, false)
    , mRunning(false)
    , mSupported(false)
    , mSamplesStored(0)
{
}

bool_t BandwidthMonitor::setMode(bool_t aEnable)
{
    return getInstance()->doSetMode(aEnable);
}

uint32_t BandwidthMonitor::getInstantaneousThroughputInBps(bool_t& aFunctional)
{
    return getInstance()->calculateInstantaneousThroughput(aFunctional);
}

uint32_t BandwidthMonitor::getCurrentThroughputInBps(bool_t& aFunctional)
{
    return getInstance()->calculateThroughput(aFunctional);
}

bool_t BandwidthMonitor::doSetMode(bool_t aEnable)
{
    if (!mSupported)
    {
        return false;
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
    return true;
}


/*Based on last 5 samples only*/
uint32_t BandwidthMonitor::calculateInstantaneousThroughput(bool_t& aFunctional)
{
    if (!mSupported)
    {
        aFunctional = false;
        return 0;
    }
    Spinlock::ScopeLock lock(mListLock);

    if (mSamplesStored < 5)  // tunable magic number
    {
        OMAF_LOG_V("Not enough data, skip");
        aFunctional = true;
        return 0;
    }
    aFunctional = true;

    FixedArray<uint32_t, SAMPLE_ARRAY_SIZE> orderedbps;
    for (size_t index = 0; index < mDownloadSamples.getSize(); index++)
    {
        if (mDownloadSamples[index].bitsPerSecond == 0)
            continue;
        if (orderedbps.getSize() == 0 || mDownloadSamples[index].bitsPerSecond > orderedbps.back())
            orderedbps.add(mDownloadSamples[index].bitsPerSecond);
        else
        {
            for (size_t i = 0; i < orderedbps.getSize(); ++i)
            {
                if (mDownloadSamples[index].bitsPerSecond < orderedbps.at(i))
                {
                    orderedbps.add(mDownloadSamples[index].bitsPerSecond, i);
                    break;
                }
            }
        }
    }

    uint32_t throughput = 0;
    if (orderedbps.getSize() > 0)
        throughput = orderedbps.at((size_t) floor(orderedbps.getSize() / 2));
    return throughput;
}

uint32_t BandwidthMonitor::calculateThroughput(bool_t& aFunctional)
{
    if (!mSupported)
    {
        aFunctional = false;
        return 0;
    }
    Spinlock::ScopeLock lock(mListLock);

    if (mSamplesStored < 5)  // tunable magic number
    {
        OMAF_LOG_V("Not enough data, skip");
        aFunctional = true;
        return 0;
    }
    aFunctional = true;
    size_t oldestSample = 0, newestSample = 0;
    uint32_t oldestTimeStampMs = OMAF_UINT32_MAX, newestTimeStampMs = 0;
    for (size_t index = 0; index < mDownloadSamples.getSize(); index++)
    {
        if (mDownloadSamples[index].timestampInMS > 0 && mDownloadSamples[index].timestampInMS < oldestTimeStampMs)
        {
            oldestTimeStampMs = mDownloadSamples[index].timestampInMS;
            oldestSample = index;
        }
        else if (mDownloadSamples[index].timestampInMS > newestTimeStampMs)
        {
            newestTimeStampMs = mDownloadSamples[index].timestampInMS;
            newestSample = index;
        }
    }
    uint32_t throughput = (uint32_t)(
        (mDownloadSamples[newestSample].byteCountCumulative - mDownloadSamples[oldestSample].byteCountCumulative) *
        1000ll * 8.f / (mDownloadSamples[newestSample].timestampInMS - mDownloadSamples[oldestSample].timestampInMS));
    OMAF_LOG_V("Calculated throughput %u", throughput);
    return throughput;
}

void_t BandwidthMonitor::measure()
{
    uint32_t now = Time::getClockTimeMs();
    int64_t bytesDownloaded = getBytesDownloaded();

    Spinlock::ScopeLock lock(mListLock);

    int64_t bitsPerSecond = 0;
    if (mSamplesStored > 0)
    {
        int64_t delta = bytesDownloaded - mDownloadSamples.at(mCurrentIndex).byteCountCumulative;
        uint32_t period = now - mDownloadSamples.at(mCurrentIndex).timestampInMS;
        bitsPerSecond = (delta * 8) / period * 1000;
        OMAF_LOG_V("%d downloaded %lld bytes - computed bps %lld", now, delta, bitsPerSecond);

        if (++mCurrentIndex == mDownloadSamples.getSize())
        {
            mCurrentIndex = 0;
        }
    }
    // else we are recording the reference sample

    mDownloadSamples.at(mCurrentIndex).byteCountCumulative = bytesDownloaded;
    mDownloadSamples.at(mCurrentIndex).bitsPerSecond = (uint32_t) bitsPerSecond;
    mDownloadSamples.at(mCurrentIndex).timestampInMS = now;
    OMAF_LOG_V("Downloaded at %u ms %lld bytes", now, bytesDownloaded);

    mSamplesStored++;
}


OMAF_NS_END
