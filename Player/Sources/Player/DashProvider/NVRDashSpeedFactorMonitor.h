
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

#include "Foundation/NVRFixedArray.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

typedef FixedArray<float32_t, 65> DownloadSpeedFactors;


class DashSpeedFactorMonitor
{
public:
    static void_t destroyInstance();

    static void_t start(size_t aNrAdaptationSetsToMonitor);
    static void_t stop();
    static void_t addSpeedFactor(float32_t aFactor);
    static float32_t getDownloadSpeed();
    static float32_t getWorstDownloadSpeed();
    static void_t reportDownloadProblem();

private:
    friend void_t OMAF::Private::Destruct<DashSpeedFactorMonitor>(void_t*);
    static DashSpeedFactorMonitor* getInstance();
    DashSpeedFactorMonitor();
    ~DashSpeedFactorMonitor();

    void_t addFactor(float32_t aFactor);
    float32_t getSpeed();
    float32_t getWorstSpeed();

private:
    static DashSpeedFactorMonitor* sInstance;

    DownloadSpeedFactors mDownloadSpeedFactors;
    DownloadSpeedFactors mOrderedDownloadSpeedFactors;
    size_t mNrDownloadsToMonitor;
};

OMAF_NS_END
