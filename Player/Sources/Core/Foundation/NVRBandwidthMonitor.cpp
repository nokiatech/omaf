
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
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRLogger.h"
#include "Math/OMAFMathFunctions.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(BandwidthMonitor)

BandwidthMonitor* BandwidthMonitor::sInstance = OMAF_NULL;


BandwidthMonitor* BandwidthMonitor::getInstance()
{
    if (sInstance == OMAF_NULL)
    {
        sInstance = OMAF_NEW_HEAP(BandwidthMonitor);
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
    , mEnabled(false)
    , mEventsStored(0)
{

}

BandwidthMonitor::~BandwidthMonitor()
{

}

void_t BandwidthMonitor::setMode(bool_t aEnable)
{
    getInstance()->doSetMode(aEnable);
}

bool_t BandwidthMonitor::canMonitorParallelDownloads()
{
    return getInstance()->doCanMonitorParallelDownloads();
}

uint32_t BandwidthMonitor::getCurrentEstimatedBandwidthInBps()
{
    return getInstance()->estimateBandWidth();
}

void_t BandwidthMonitor::notifyDownloadCompleted(uint64_t aTimeTakenInMS, uint64_t aBytesDownloaded)
{
    getInstance()->addDownloadEvent(aTimeTakenInMS, aBytesDownloaded);
}

void_t BandwidthMonitor::doSetMode(bool_t aEnable)
{
}

bool_t BandwidthMonitor::doCanMonitorParallelDownloads()
{
    return false;
}

uint32_t BandwidthMonitor::estimateBandWidth()
{
    return 0;
}

void_t BandwidthMonitor::addDownloadEvent(uint64_t aTimeTakenInMS, uint64_t aBytesDownloaded)
{
}

OMAF_NS_END
