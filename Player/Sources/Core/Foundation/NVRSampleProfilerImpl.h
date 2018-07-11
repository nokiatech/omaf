
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
#pragma once

#include "Foundation/NVRSampleProfiler.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN
    template <size_t N>
    SampleProfiler<N>::SampleProfiler(const char_t* name, bool_t autoReset, bool_t autoLog)
    : mName(name)
    , mAutoReset(autoReset)
    , mAutoLog(autoLog)
    , mStartTime(0)
    , mEndTime(0)
    , mSampleIndex(0)
    {
        OMAF_ASSERT(N > 0, "");
        
        MemoryZero(mSamples, N * OMAF_SIZE_OF(int64_t));
    }
    
    template <size_t N>
    SampleProfiler<N>::~SampleProfiler()
    {
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::setAutoReset(bool_t enabled)
    {
        mAutoReset = enabled;
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::setAutoLog(bool_t enabled)
    {
        mAutoLog = enabled;
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::start()
    {
        mStartTime = Time::getThreadTimeUs();
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::stop()
    {
        mEndTime = Time::getThreadTimeUs();
        
        int32_t index = (mSampleIndex++) % N;
        mSamples[index] = mEndTime - mStartTime;
        
        if (mAutoLog && index == N - 1)
        {
            printLog();
        }
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::reset()
    {
        mSampleIndex = 0;
    }
    
    template <size_t N>
    void_t SampleProfiler<N>::printLog()
    {
        if (mSampleIndex == 0)
        {
            return;
        }
        
        int64_t minUs = -1;
        int64_t maxUs = -1;
        int64_t meanUs = 0;
        
        for (int32_t n = 0; n < mSampleIndex && n < N; ++n)
        {
            meanUs += mSamples[n];
            
            if (minUs == -1 || mSamples[n] < minUs)
            {
                minUs = mSamples[n];
            }
            if (maxUs == -1 || mSamples[n] > maxUs)
            {
                maxUs = mSamples[n];
            }
        }
        
        meanUs /= (mSampleIndex < N ? mSampleIndex : N);
        
        int64_t residualSqrSum = 0, residual;
        
        for (int32_t n = 0; n < mSampleIndex && n < N; ++n)
        {
            residual = mSamples[n] - meanUs;
            residualSqrSum += residual * residual;
        }
        
        int64_t varianceUs = residualSqrSum / (mSampleIndex < N ? mSampleIndex : N);
        
        OMAF_LOG_ZONE(SampleProfiler)
        
        OMAF_UNUSED_VARIABLE(varianceUs);
        OMAF_LOG_D("%s: Average time %.3f ms, min = %.3f ms, max = %.3f ms, stddev = %.3f, variance = %.3f",
                  mName,
                  meanUs / 1000.0f,
                  minUs / 1000.0f,
                  maxUs / 1000.0f,
                  sqrtf(varianceUs / 1000000.0f),
                  varianceUs / 1000000.0f);
        
        if (mAutoReset)
        {
            reset();
        }
    }
OMAF_NS_END
