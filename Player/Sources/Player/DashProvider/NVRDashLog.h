
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
#pragma once

#include "Foundation/NVRLogger.h"

#if 0 //TODO enable only when needed as can cause frame drops
OMAF_NS_BEGIN
    namespace DASH_LOG
    {
        static FileLogger sFileLog("downloadlog.txt");
        inline void_t logDebug(const char_t* zone, const char_t* format, ...)
        {
            va_list args;
            va_start(args, format);

            sFileLog.logDebug(zone, format, args);

            va_end(args);
        }
        static FileLogger sFileSummary("downloadsummary.txt");
        inline void_t logDebugSum(const char_t* zone, const char_t* format, ...)
        {
            va_list args;
            va_start(args, format);

            sFileSummary.logDebug(zone, format, args);

            va_end(args);
        }
}

#define DASH_LOG_D(...) DASH_LOG::logDebug("DASH", ## __VA_ARGS__)
#define DASH_LOG_SUM_D(...) DASH_LOG::logDebugSum("DASH", ## __VA_ARGS__)



OMAF_NS_END
#else
#define DASH_LOG_D(...)
#define DASH_LOG_SUM_D(...)
#endif
