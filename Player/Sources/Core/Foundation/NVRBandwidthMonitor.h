
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
#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRSpinlock.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

static const uint32_t SAMPLE_ARRAY_SIZE = 30;


class BandwidthMonitor
{
public:
    static BandwidthMonitor* getInstance();
    static void_t destroyInstance();

    static bool_t setMode(bool_t aEnable);
    static uint32_t getInstantaneousThroughputInBps(bool_t& aFunctional);
    static uint32_t getCurrentThroughputInBps(bool_t& aFunctional);

protected:
    friend void_t OMAF::Private::Destruct<BandwidthMonitor>(void_t*);
    BandwidthMonitor();
    virtual ~BandwidthMonitor(){};

    virtual bool_t doSetMode(bool_t aEnable);
    virtual void_t measure();
    virtual uint32_t calculateInstantaneousThroughput(bool_t& aFunctional);
    virtual uint32_t calculateThroughput(bool_t& aFunctional);
    virtual int64_t getBytesDownloaded() = 0;

protected:
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

private:
    static BandwidthMonitor* sInstance;
};

OMAF_NS_END
