
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
#include "NVRDashSpeedFactorMonitor.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRTime.h"
#include "Math/OMAFMath.h"

OMAF_NS_BEGIN

OMAF_LOG_ZONE(DashSpeedFactorMonitor)

DashSpeedFactorMonitor* DashSpeedFactorMonitor::sInstance = OMAF_NULL;

DashSpeedFactorMonitor::DashSpeedFactorMonitor()
    : mDownloadSpeedFactors()
    , mOrderedDownloadSpeedFactors()
{
}


DashSpeedFactorMonitor::~DashSpeedFactorMonitor()
{
}

DashSpeedFactorMonitor* DashSpeedFactorMonitor::getInstance()
{
    if (sInstance == OMAF_NULL)
    {
        sInstance = OMAF_NEW_HEAP(DashSpeedFactorMonitor);
    }
    return sInstance;
}

void_t DashSpeedFactorMonitor::destroyInstance()
{
    if (sInstance != OMAF_NULL)
    {
        OMAF_DELETE_HEAP(sInstance);
        sInstance = OMAF_NULL;
    }
}

void_t DashSpeedFactorMonitor::start(size_t aNrAdaptationSetsToMonitor)
{
    // Value 0.0f would mean infite speed, so let's use a more realistic one as default value (2 ^= 1 sec segment
    // downloaded in 0.5 sec).
    getInstance()->mDownloadSpeedFactors.clear();
    getInstance()->mOrderedDownloadSpeedFactors.clear();
    getInstance()->mNrDownloadsToMonitor =
        min(aNrAdaptationSetsToMonitor * 2 + 1, getInstance()->mDownloadSpeedFactors.getCapacity());
}

void_t DashSpeedFactorMonitor::stop()
{
}

void_t DashSpeedFactorMonitor::addSpeedFactor(float32_t aFactor)
{
    getInstance()->addFactor(aFactor);
}

float32_t DashSpeedFactorMonitor::getDownloadSpeed()
{
    return getInstance()->getSpeed();
}

float32_t DashSpeedFactorMonitor::getWorstDownloadSpeed()
{
    return getInstance()->getWorstSpeed();
}

void_t DashSpeedFactorMonitor::reportDownloadProblem()
{
}

void_t DashSpeedFactorMonitor::addFactor(float32_t aFactor)
{
    if (mDownloadSpeedFactors.getSize() == mNrDownloadsToMonitor)
    {
        float32_t oldest = mDownloadSpeedFactors.at(0);
        mDownloadSpeedFactors.removeAt(0, 1);
        mOrderedDownloadSpeedFactors.remove(oldest);
    }
    OMAF_LOG_V("%d\tAdd factor\t%f", Time::getClockTimeMs(), aFactor);
    mDownloadSpeedFactors.add(aFactor);
    if (mOrderedDownloadSpeedFactors.isEmpty() || aFactor > mOrderedDownloadSpeedFactors.back())
    {
        // add to the end
        mOrderedDownloadSpeedFactors.add(aFactor);
    }
    else
    {
        for (size_t i = 0; i < mOrderedDownloadSpeedFactors.getSize(); ++i)
        {
            if (aFactor < mOrderedDownloadSpeedFactors.at(i))
            {
                mOrderedDownloadSpeedFactors.add(aFactor, i);
                break;
            }
        }
    }
}

// take median of the factors
float32_t DashSpeedFactorMonitor::getSpeed()
{
    if (mOrderedDownloadSpeedFactors.isEmpty())
    {
        return 1.f;  //??
    }
    float32_t median = mOrderedDownloadSpeedFactors.at((size_t) floor(mOrderedDownloadSpeedFactors.getSize() / 2));
    OMAF_LOG_V("Median downloadSpeedFactor: %f", median);
    return median;
}

float32_t DashSpeedFactorMonitor::getWorstSpeed()
{
    if (mOrderedDownloadSpeedFactors.isEmpty())
    {
        return 1.f;  //??
    }
    float32_t smallest = mOrderedDownloadSpeedFactors.at(0);
    OMAF_LOG_V("Worst downloadSpeedFactor: %f", smallest);
    return smallest;
}

OMAF_NS_END
