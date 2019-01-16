
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
#include "Foundation/NVRTime.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
#if (OMAF_PLATFORM_ANDROID)
    static int64_t getNs(const timespec& ts)
    {
        return (ts.tv_sec * 1000000000ll + ts.tv_nsec);
    }
#endif

    int32_t Time::getClockTimeMs()
    {
        return (int32_t)(getClockTimeUs() / 1000ll);
    }

    int64_t Time::getClockTimeUs()
    {
#if (OMAF_PLATFORM_ANDROID)

        timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        static int64_t initial = getNs(ts);
        return (getNs(ts) - initial) / 1000ll;

#elif OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

        return Clock::getMicroseconds();

#else

#error Unsupported platform

#endif
    }

    int32_t Time::getProcessTimeMs()
    {
        return (int32_t)(getProcessTimeUs() / 1000ll);
    }

    int64_t Time::getProcessTimeUs()
    {
#if (OMAF_PLATFORM_ANDROID)

        timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        static int64_t initial = getNs(ts);
        return (getNs(ts) - initial) / 1000ll;

#elif OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

        // TODO: Add getProcessTimeUs implementation
        OMAF_ASSERT_NOT_IMPLEMENTED();
        return 0;

#else

#error Unsupported platform

#endif
    }

    int32_t Time::getThreadTimeMs()
    {
        return (int32_t)(getThreadTimeUs() / 1000ll);
    }

    int64_t Time::getThreadTimeUs()
    {
#if (OMAF_PLATFORM_ANDROID)

        timespec ts;
        ::clock_gettime(CLOCK_REALTIME, &ts);
        static int64_t initial = getNs(ts);
        return (getNs(ts) - initial) / 1000ll;

#elif OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

        // TODO: Add getThreadTimeUs implementation
        OMAF_ASSERT_NOT_IMPLEMENTED();
        return 0;

#else

#error Unsupported platform

#endif
    }

    time_t Time::fromUTCString(const char_t* timeStr)
    {
#if (OMAF_PLATFORM_ANDROID)

        const char_t *timeFormat = "%Y-%m-%dT%H:%M:%S"; //yyyy-MM-ddThh:mm:ss
        struct tm time;
        ::strptime(timeStr, timeFormat, &time);
        time_t t_time = ::timegm(&time);
        return t_time;

#else
        const char_t *timeFormat = "%Y-%m-%dT%H:%M:%S"; //yyyy-MM-ddThh:mm:ss
        uint32_t year, month, day, hour, minute, second;
        sscanf(timeStr, "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
        struct tm time;
        time.tm_hour = hour;
        time.tm_min = minute;
        time.tm_sec = second;
        time.tm_year = year - 1900;
        time.tm_mon = month - 1;
        time.tm_mday = day;
        time_t t_time = ::_mkgmtime32(&time);
        return t_time;
#endif
    }
OMAF_NS_END
