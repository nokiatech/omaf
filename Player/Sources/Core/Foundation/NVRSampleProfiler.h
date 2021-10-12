
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

#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
template <size_t N>
class SampleProfiler
{
public:
    SampleProfiler(const char_t* name, bool_t autoReset = true, bool_t autoLog = true);

    ~SampleProfiler();

    void_t setAutoReset(bool_t enabled = true);
    void_t setAutoLog(bool_t enabled = true);

    void_t start();
    void_t stop();

    void_t reset();

    void_t printLog();

private:
    const char_t* mName;

    bool_t mAutoReset;
    bool_t mAutoLog;

    int64_t mStartTime;
    int64_t mEndTime;

    int64_t mSamples[N];
    int32_t mSampleIndex;
};
OMAF_NS_END

#include "Foundation/NVRSampleProfilerImpl.h"
