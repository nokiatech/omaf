
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

#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRThread.h"
#include "NVREssentials.h"

// @note for some reason compilation fails if this is included before anything else
//       this comment prevents you and clang-format from reordering this one.
#include <ifmib.h>

OMAF_NS_BEGIN

class BandwidthMonitorWindows : public BandwidthMonitor
{
public:
    BandwidthMonitorWindows();
    ~BandwidthMonitorWindows();

protected:
    int64_t getBytesDownloaded();

private:
    Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);

private:
    MIB_IFTABLE* mIfTable;
    struct Connection
    {
        IF_INDEX rowIndex;
        DWORD octetsIn;
    };
    DWORD mTableSize;
    FixedArray<Connection*, 8> mConnections;
};

OMAF_NS_END
