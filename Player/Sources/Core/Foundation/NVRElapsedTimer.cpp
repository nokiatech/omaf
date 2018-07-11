
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
#include "Foundation/NVRElapsedTimer.h"

#include "Platform/OMAFPlatformDetection.h"
#include "Foundation/NVRClock.h"

OMAF_NS_BEGIN
    ElapsedTimer::ElapsedTimer(uint64_t durationMs)
    : mIsValid(false)
    , mDurationUs(durationMs * 1000)
    , mStartTimeUs(0)
    {
    }
    
    ElapsedTimer::ElapsedTimer(uint64_t durationMs, uint64_t startTimeMs)
    : mIsValid(false)
    , mDurationUs(durationMs * 1000)
    , mStartTimeUs(startTimeMs * 1000)
    {
        
    }

    ElapsedTimer::~ElapsedTimer()
    {
        invalidate();
    }
    
    uint64_t ElapsedTimer::elapsedMs() const
    {
        uint64_t delta = Clock::getMicroseconds() - mStartTimeUs;
        
        return (delta / 1000);
    }
    
    uint64_t ElapsedTimer::elapsedUs() const
    {
        uint64_t delta = Clock::getMicroseconds() - mStartTimeUs;
        
        return delta;
    }
    
    void_t ElapsedTimer::setDurationMs(uint64_t durationMs)
    {
        mDurationUs = durationMs * 1000;
    }
    
    uint64_t ElapsedTimer::getDurationMs() const
    {
        return mDurationUs / 1000;
    }
    
    void_t ElapsedTimer::setDurationUs(uint64_t durationUs)
    {
        mDurationUs = durationUs;
    }
    
    uint64_t ElapsedTimer::getDurationUs() const
    {
        return mDurationUs;
    }
    
    bool_t ElapsedTimer::hasExpired() const
    {
        if (!this->isValid())
        {
            return false;
        }
        
        uint64_t elapsedUs = this->elapsedUs();
        
        if (elapsedUs > mDurationUs)
        {
            return true;
        }
        
        return false;
    }
    
    void_t ElapsedTimer::invalidate()
    {
        mIsValid = false;
        mStartTimeUs = 0;
    }
    
    bool_t ElapsedTimer::isValid() const
    {
        return mIsValid;
    }
    
    void_t ElapsedTimer::start()
    {
        mStartTimeUs = Clock::getMicroseconds();
        
        mIsValid = true;
    }
    
    uint64_t ElapsedTimer::restart()
    {
        if (!this->isValid())
        {
            return 0;
        }
        
        uint64_t elapsedUs = this->elapsedUs();
        
        mStartTimeUs = Clock::getMicroseconds();
        
        return elapsedUs;
    }
OMAF_NS_END
