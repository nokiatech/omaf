
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
#include "NVRBandwidthMonitorWindows.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRTime.h"
#include "Math/OMAFMathFunctions.h"
#pragma comment(lib, "IPHLPAPI.lib")
#include <IPHlpApi.h>

OMAF_NS_BEGIN

OMAF_LOG_ZONE(BandwidthMonitorWindows)

static const uint32_t MEASUREMENT_PERIOD_MS = 5 * 1000;
static const uint32_t MEASUREMENT_INTERVAL_MS = MEASUREMENT_PERIOD_MS / SAMPLE_ARRAY_SIZE;

BandwidthMonitorWindows::BandwidthMonitorWindows()
    : BandwidthMonitor()
    , mTableSize(0)
{
    mThread.setPriority(Thread::Priority::LOWEST);
    mThread.setName("BandwidthMonitorWindows");

    Thread::EntryFunction function;
    function.bind<BandwidthMonitorWindows, &BandwidthMonitorWindows::threadEntry>(this);

    mThread.start(function);
}

BandwidthMonitorWindows::~BandwidthMonitorWindows()
{
    if (mThread.isValid() && mThread.isRunning())
    {
        mRunning = false;
        mMeasurementEnabled.signal();
        mThread.stop();
        mThread.join();
    }
}

Thread::ReturnValue BandwidthMonitorWindows::threadEntry(const Thread& thread, void_t* userData)
{
    mIfTable = (MIB_IFTABLE*) OMAF_ALLOC(*Private::MemorySystem::DefaultHeapAllocator(), sizeof(MIB_IFTABLE));
    mTableSize = sizeof(MIB_IFTABLE);
    if (GetIfTable(mIfTable, &mTableSize, 0) == ERROR_INSUFFICIENT_BUFFER)
    {
        OMAF_FREE(*Private::MemorySystem::DefaultHeapAllocator(), mIfTable);
        mIfTable = (MIB_IFTABLE*) OMAF_ALLOC(*Private::MemorySystem::DefaultHeapAllocator(), mTableSize);
    }

    // take only nonzero entries, from rows that are operational or connected (seems like connected state is not used
    // though) take only entries that have unique octet count
    if (GetIfTable(mIfTable, &mTableSize, 0) == NO_ERROR)
    {
        if (mIfTable->dwNumEntries > 0)
        {
            MIB_IFROW* row;

            for (int i = 0; i < mIfTable->dwNumEntries; i++)
            {
                row = (MIB_IFROW*) &mIfTable->table[i];
                OMAF_LOG_V("IF: %s %lld", row->bDescr, row->dwInOctets);
                if (row->dwInOctets > 0 &&
                    (row->dwOperStatus == INTERNAL_IF_OPER_STATUS::IF_OPER_STATUS_CONNECTED ||
                     row->dwOperStatus == INTERNAL_IF_OPER_STATUS::IF_OPER_STATUS_OPERATIONAL))
                {
                    bool dupe = false;
                    for (FixedArray<Connection*, 8>::Iterator it = mConnections.begin(); it != mConnections.end(); ++it)
                    {
                        if ((*it)->octetsIn == row->dwInOctets)
                        {
                            // skip
                            dupe = true;
                            break;
                        }
                    }
                    if (!dupe)
                    {
                        Connection* c = OMAF_NEW_HEAP(Connection);
                        c->octetsIn = row->dwInOctets;
                        c->rowIndex = row->dwIndex;
                        mConnections.add(c);
                    }
                }
            }
        }
    }

    if (!mConnections.isEmpty())
    {
        mRunning = true;
        mSupported = true;
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


    for (size_t i = 0; i < mConnections.getSize(); i++)
    {
        OMAF_DELETE_HEAP(mConnections[i]);
    }
    mConnections.clear();
    OMAF_FREE(*Private::MemorySystem::DefaultHeapAllocator(), mIfTable);

    return 0;
}

int64_t BandwidthMonitorWindows::getBytesDownloaded()
{
    DWORD largest = 0;
    DWORD status = GetIfTable(mIfTable, &mTableSize, 0);

    if (status == NO_ERROR)
    {
        if (mIfTable->dwNumEntries > 0)
        {
            MIB_IFROW* row;
            for (int i = 0; i < mIfTable->dwNumEntries; i++)
            {
                row = (MIB_IFROW*) &mIfTable->table[i];
                if (row->dwInOctets > 0 &&
                    (row->dwOperStatus == INTERNAL_IF_OPER_STATUS::IF_OPER_STATUS_CONNECTED ||
                     row->dwOperStatus == INTERNAL_IF_OPER_STATUS::IF_OPER_STATUS_OPERATIONAL))
                {
                    for (FixedArray<Connection*, 8>::Iterator it = mConnections.begin(); it != mConnections.end(); ++it)
                    {
                        if ((*it)->rowIndex == row->dwIndex)
                        {
                            if (row->dwInOctets - (*it)->octetsIn > largest)
                            {
                                largest = row->dwInOctets - (*it)->octetsIn;
                            }
                        }
                    }
                }
            }
        }
    }

    return (int64_t) largest;
}


OMAF_NS_END
