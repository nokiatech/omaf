
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

#if 0 // enable only when needed as can cause frame drops
OMAF_NS_BEGIN
    namespace LATENCY_LOG
    {
        static FileLogger sFileLog("latencylog.txt");
        inline void_t logDebug(const char_t* zone, const char_t* format, ...)
        {
            va_list args;
            va_start(args, format);

            sFileLog.logDebug(zone, format, args);

            va_end(args);
        }

    }

#define LATENCY_LOG_D(...) LATENCY_LOG::logDebug("Latency", ## __VA_ARGS__)


OMAF_NS_END
#else
#define LATENCY_LOG_D(...)

#endif
