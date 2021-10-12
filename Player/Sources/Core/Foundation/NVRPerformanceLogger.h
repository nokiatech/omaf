
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
#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRLogger.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

#define OMAF_ENABLE_PERFORMANCE_LOGGING 0

#if OMAF_ENABLE_PERFORMANCE_LOGGING
class PerformanceLogger
{
public:
    PerformanceLogger(const char_t* logName, uint32_t minFilterMS = 0, bool_t startOnCreate = true);
    ~PerformanceLogger();

    void_t restart();
    void_t printIntervalTime(const char_t* logTag);
    void_t stop();

private:
    OMAF_NO_DEFAULT(PerformanceLogger);

private:
    const char_t* mName;
    uint32_t mStartTimeInMS;
    uint32_t mMinFilterMS;
    uint32_t mLastIntervalTimeMS;
};
#else
class PerformanceLogger
{
public:
    PerformanceLogger(const char_t* logName, uint32_t minFilterMS = 0, bool_t startOnCreate = true)
    {
    }
    ~PerformanceLogger()
    {
    }

    void_t restart()
    {
    }
    void_t printIntervalTime(const char_t* logTag)
    {
    }
    void_t stop()
    {
    }

private:
    OMAF_NO_DEFAULT(PerformanceLogger);

private:
};
#endif

OMAF_NS_END
