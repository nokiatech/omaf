
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
#include "Foundation/NVRPerformanceLogger.h"
#include "Foundation/NVRTime.h"
OMAF_NS_BEGIN
OMAF_LOG_ZONE(PerformanceLogger)
#if OMAF_ENABLE_PERFORMANCE_LOGGING
PerformanceLogger::PerformanceLogger(const char_t* logName, uint32_t minFilterMS, bool_t startOnCreate)
: mName(logName)
, mMinFilterMS(minFilterMS)
, mStartTimeInMS(OMAF_UINT32_MAX)
, mLastIntervalTimeMS(0)
{
    if (startOnCreate)
    {
        restart();
    }
}

PerformanceLogger::~PerformanceLogger()
{
    stop();
}

void_t PerformanceLogger::restart()
{
    mStartTimeInMS = Time::getClockTimeMs();
    mLastIntervalTimeMS = mStartTimeInMS;
}

void_t PerformanceLogger::printIntervalTime(const char_t* logTag)
{
    if (mStartTimeInMS != OMAF_UINT32_MAX)
    {
        uint32_t now = Time::getClockTimeMs();
        uint32_t timeSince = now - mLastIntervalTimeMS;
        mLastIntervalTimeMS = now;
        if (timeSince >= mMinFilterMS)
        {
            OMAF_LOG_W("%s %s  %d ms", mName, logTag, timeSince);
        }
    }
}

void_t PerformanceLogger::stop()
{
    if (mStartTimeInMS != OMAF_UINT32_MAX)
    {
        uint32_t timeSince = Time::getClockTimeMs() - mStartTimeInMS;
        if (timeSince >= mMinFilterMS)
        {
            OMAF_LOG_W("%s took %d ms", mName, timeSince);
        }
    }
    mStartTimeInMS = OMAF_UINT32_MAX;
}
#endif
OMAF_NS_END