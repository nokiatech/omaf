
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
#include "NVREssentials.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRAtomicBoolean.h"

OMAF_NS_BEGIN

class BandwidthMonitor
{
public:
    static BandwidthMonitor* getInstance();
    static void_t destroyInstance();

    static void_t setMode(bool_t aEnable);
    static bool_t canMonitorParallelDownloads();
    static void_t notifyDownloadCompleted(uint64_t aTimeTakenInMS, uint64_t aBytesDownloaded);
    static uint32_t getCurrentEstimatedBandwidthInBps();

protected:
    friend void_t OMAF::Private::Destruct<BandwidthMonitor>(void_t *);
    BandwidthMonitor();
    virtual ~BandwidthMonitor();

    virtual void_t doSetMode(bool_t aEnable);
    virtual bool_t doCanMonitorParallelDownloads();
    virtual uint32_t estimateBandWidth();
    virtual void_t addDownloadEvent(uint64_t aTimeTakenInMS, uint64_t aBytesDownloaded);

private:
    static BandwidthMonitor* sInstance;

    Spinlock mListLock;

    struct DownloadSample
    {
        uint32_t bitsPerSecond;
        uint32_t timestampInMS;
    };
    typedef FixedArray<DownloadSample, 100> DownloadSamples;
    DownloadSamples mDownloadSamples;
    size_t mCurrentIndex;
    size_t mEventsStored;
    AtomicBoolean mEnabled;
};

OMAF_NS_END