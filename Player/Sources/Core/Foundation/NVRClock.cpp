
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
#include "Foundation/NVRClock.h"

#include "Platform/OMAFCompiler.h"
#include "Foundation/NVRDependencies.h"

OMAF_NS_BEGIN
    namespace Clock
    {
        static bool_t _initialized = false;
        
        static uint64_t _ticksPerSecond = 0;
        static float64_t _secondsPerTick = 0.0;
        
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        
        static uint64_t _startTicks = 0;
        
#elif OMAF_PLATFORM_ANDROID
        
        static timeval _startTime = { 0, 0 };
        
#else
        
    #error Unsupported platform
        
#endif
        
        static void_t initializeClock()
        {
            if(_initialized == true)
            {
                return;
            }
            
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            
            LARGE_INTEGER frequence;
            LARGE_INTEGER counter;
            
            ::QueryPerformanceFrequency(&frequence);
            ::QueryPerformanceCounter(&counter);
            
            _ticksPerSecond = frequence.QuadPart;
            _secondsPerTick = 1.0 / (float64_t)frequence.QuadPart;

            _startTicks = counter.QuadPart;
            
#elif OMAF_PLATFORM_ANDROID
            
            _ticksPerSecond = CLOCKS_PER_SEC;
            _secondsPerTick = 1.0 / (float64_t)CLOCKS_PER_SEC;
            
            ::gettimeofday(&_startTime, NULL);
            
#else
            
    #error Unsupported platform
            
#endif
            
            _initialized = true;
        }
        
        
        uint64_t getTicks()
        {
            initializeClock();
            
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            
            LARGE_INTEGER counter;
            ::QueryPerformanceCounter(&counter);
            
            uint64_t ticks = counter.QuadPart;
            
            return ticks;
            
#elif OMAF_PLATFORM_ANDROID
            
            return (CLOCKS_PER_SEC * getSeconds());
            
#else
            
    #error Unsupported platform
            
#endif
        }
        
        uint64_t getMicroseconds()
        {
            initializeClock();
            
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            
            LARGE_INTEGER currentCounter;
            ::QueryPerformanceCounter(&currentCounter);
            
            float64_t ms = (float64_t)((currentCounter.QuadPart - _startTicks) * (_secondsPerTick * 1000.0 * 1000.0));
            
            return (uint64_t)ms;
            
#elif OMAF_PLATFORM_ANDROID
            
            struct timeval currentTime;
            ::gettimeofday(&currentTime, NULL);
            
            uint64_t us = (currentTime.tv_sec - _startTime.tv_sec) * 1000 * 1000;
            us += (currentTime.tv_usec - _startTime.tv_usec);
            
            return us;
            
#else
            
    #error Unsupported platform
            
#endif
        }
        
        uint64_t getMilliseconds()
        {
            uint64_t us = getMicroseconds();
            
            return (us / 1000ll);
        }
        
        float64_t getSeconds()
        {
            uint64_t us = getMicroseconds();
            
            return (((float64_t)us) / 1000000.0);
        }
        
        uint64_t getTicksPerSecond()
        {
            initializeClock();
            
            return _ticksPerSecond;
        }
        
        uint64_t getTicksPerMs()
        {
            initializeClock();
            
            return _ticksPerSecond / 1000;
        }
        
        float64_t getSecondsPerTick()
        {
            initializeClock();
            
            return _secondsPerTick;
        }
        
        float64_t getMsPerTick()
        {
            initializeClock();
            
            return _secondsPerTick * 1000.0;
        }
        
        uint64_t getSecondsSinceReference()
        {
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            
            LARGE_INTEGER currentCounter;
            ::QueryPerformanceCounter(&currentCounter);
            
            uint64_t seconds = (uint64_t)((currentCounter.QuadPart) * _secondsPerTick);
            
            return seconds;
            
#elif OMAF_PLATFORM_ANDROID
            
            struct timeval currentTime;
            ::gettimeofday(&currentTime, NULL);
            
            uint64_t seconds = currentTime.tv_sec;
            
            return seconds;
            
#else
            
    #error Unsupported platform
            
#endif
        }

        time_t getEpochTime()
        {
            time_t elapsedSec;
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            // FILETIME of Jan 1 1970 00:00:00.
            static const uint64_t epoch = ((uint64_t)116444736000000000Ui64);

            FILETIME fileTime;
            ULARGE_INTEGER secondsSince1601;

            GetSystemTimeAsFileTime(&fileTime);
            secondsSince1601.LowPart = fileTime.dwLowDateTime;
            secondsSince1601.HighPart = fileTime.dwHighDateTime;
            elapsedSec = (time_t)((secondsSince1601.QuadPart - epoch) / 10000000L);

#elif OMAF_PLATFORM_ANDROID
            struct timeval tv;
            ::gettimeofday(&tv, NULL);
            elapsedSec = tv.tv_sec;
#else

    #error Unsupported platform

#endif
            return elapsedSec;
        }

        uint64_t getRandomSeed()
        {
            uint64_t seed = 0;
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
            FILETIME fileTime;
            GetSystemTimeAsFileTime(&fileTime);
            ULARGE_INTEGER secondsSince1601;
            secondsSince1601.LowPart = fileTime.dwLowDateTime;
            secondsSince1601.HighPart = fileTime.dwHighDateTime;

            seed = secondsSince1601.QuadPart/10;    // 1 unit == 100 nanosecond; use microseconds like in other platforms

#elif OMAF_PLATFORM_ANDROID
            struct timeval tv;
            ::gettimeofday(&tv, NULL);
            seed = tv.tv_usec;
#else

    #error Unsupported platform

#endif
            return seed;
        }

        bool_t getTimestampString(utf8_t buffer[64])
        {
            char_t fmt[64];
            time_t elapsedSec;
            struct tm* timeStruct;

            elapsedSec = getEpochTime();

            if ((timeStruct = ::gmtime(&elapsedSec)) != OMAF_NULL)
            {
                ::strftime(fmt, sizeof(fmt), "%Y-%m-%dT%H:%M:%SZ", timeStruct);
                ::sprintf(buffer, fmt, elapsedSec);
                
                return true;
            }
            
            return false;
        }
        
        bool_t getTimestampString(utf8_t buffer[64], time_t timestamp)
        {
            char_t fmt[64];
            struct tm* timeStruct;
            
            if ((timeStruct = ::gmtime(&timestamp)) != OMAF_NULL)
            {
                ::strftime(fmt, sizeof(fmt), "%Y-%m-%dT%H:%M:%SZ", timeStruct);
                ::sprintf(buffer, fmt, timestamp);
                
                return true;
            }
            
            return false;
        }
    }

OMAF_NS_END
