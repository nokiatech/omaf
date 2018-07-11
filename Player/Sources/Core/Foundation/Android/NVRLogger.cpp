
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
#include "Foundation/NVRLogger.h"

#include "Foundation/NVRDependencies.h"

OMAF_NS_BEGIN
    ConsoleLogger::ConsoleLogger()
    {
    }

    ConsoleLogger::~ConsoleLogger()
    {
    }

    void_t ConsoleLogger::logVerbose(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        __android_log_vprint(ANDROID_LOG_VERBOSE, zone, format, copy);

        va_end(copy);
    }

    void_t ConsoleLogger::logDebug(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        __android_log_vprint(ANDROID_LOG_DEBUG, zone, format, copy);

        va_end(copy);
    }

    void_t ConsoleLogger::logInfo(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        __android_log_vprint(ANDROID_LOG_INFO, zone, format, copy);

        va_end(copy);
    }

    void_t ConsoleLogger::logWarning(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        __android_log_vprint(ANDROID_LOG_WARN, zone, format, copy);

        va_end(copy);
    }

    void_t ConsoleLogger::logError(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

        __android_log_vprint(ANDROID_LOG_ERROR, zone, format, copy);

        va_end(copy);
    }
OMAF_NS_END
