
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

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
/// Non-thread safe timer to calculate elapsed times.
class ElapsedTimer
{
public:
    ElapsedTimer(uint64_t durationMs);
    ElapsedTimer(uint64_t durationMs, uint64_t startTimeMs);

    ~ElapsedTimer();

    uint64_t elapsedMs() const;
    uint64_t elapsedUs() const;

    void_t setDurationMs(uint64_t durationMs);
    uint64_t getDurationMs() const;

    void_t setDurationUs(uint64_t durationUs);
    uint64_t getDurationUs() const;

    bool_t hasExpired() const;

    void_t invalidate();
    bool_t isValid() const;

    void_t start();

    /// Restarts the timer and returns the time elapsed since the previous start in microseconds.
    uint64_t restart();

private:
    OMAF_NO_COPY(ElapsedTimer);
    OMAF_NO_ASSIGN(ElapsedTimer);

private:
    bool_t mIsValid;

    uint64_t mDurationUs;
    uint64_t mStartTimeUs;
};
OMAF_NS_END
