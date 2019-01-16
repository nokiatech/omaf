
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
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
#define MAX_LOG_LINE_LENGTH 1024
#define OMAF_LOG_TO_VS_CONSOLE 1
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

#ifdef OMAF_LOG_TO_VS_CONSOLE
        char buf2[MAX_LOG_LINE_LENGTH];
        char buf[sizeof(buf2)];
        ::vsnprintf(buf, sizeof(buf), format, copy);
        _snprintf(buf2, sizeof(buf2), "V/%s: %s\n", zone, buf);
        OutputDebugStringA(buf2);
#else
        ::fprintf(stderr, "V/%s: ", zone);
        ::vfprintf(stderr, format, copy);
        ::fprintf(stderr, "\n");
        ::fflush(stderr);
#endif

        va_end(copy);
    }

    void_t ConsoleLogger::logDebug(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

#ifdef OMAF_LOG_TO_VS_CONSOLE
        char buf2[MAX_LOG_LINE_LENGTH];
        char buf[sizeof(buf2)];
        ::vsnprintf(buf, sizeof(buf), format, copy);
        _snprintf(buf2, sizeof(buf2), "D/%s: %s\n", zone, buf);
        OutputDebugStringA(buf2);
#else
        ::fprintf(stderr, "D/%s: ", zone);
        ::vfprintf(stderr, format, copy);
        ::fprintf(stderr, "\n");
        ::fflush(stderr);
#endif

        va_end(copy);
    }

    void_t ConsoleLogger::logInfo(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

#ifdef OMAF_LOG_TO_VS_CONSOLE
        char buf2[MAX_LOG_LINE_LENGTH];
        char buf[sizeof(buf2)];
        ::vsnprintf(buf, sizeof(buf), format, copy);
        _snprintf(buf2, sizeof(buf2), "I/%s: %s\n", zone, buf);
        OutputDebugStringA(buf2);
#else
        ::fprintf(stderr, "I/%s: ", zone);
        ::vfprintf(stderr, format, copy);
        ::fprintf(stderr, "\n");
        ::fflush(stderr);
#endif

        va_end(copy);
    }

    void_t ConsoleLogger::logWarning(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

#ifdef OMAF_LOG_TO_VS_CONSOLE
        char buf2[MAX_LOG_LINE_LENGTH];
        char buf[sizeof(buf2)];
        ::vsnprintf(buf, sizeof(buf), format, copy);
        _snprintf(buf2, sizeof(buf2), "W/%s: %s\n", zone, buf);
        OutputDebugStringA(buf2);
#else
        ::fprintf(stderr, "W/%s: ", zone);
        ::vfprintf(stderr, format, copy);
        ::fprintf(stderr, "\n");
        ::fflush(stderr);
#endif

        va_end(copy);
    }

    void_t ConsoleLogger::logError(const char_t* zone, const char_t* format, va_list args)
    {
        va_list copy;
        va_copy(copy, args);

#ifdef OMAF_LOG_TO_VS_CONSOLE
        char buf[MAX_LOG_LINE_LENGTH];
        char buf2[sizeof(buf)];
        ::vsnprintf(buf, sizeof(buf), format, copy);
        _snprintf(buf2, sizeof(buf2), "E/%s: %s\n", zone, buf);
        OutputDebugStringA(buf2);
#else
        ::fprintf(stderr, "E/%s: ", zone);
        ::vfprintf(stderr, format, copy);
        ::fprintf(stderr, "\n");
        ::fflush(stderr);
#endif

        va_end(copy);
    }
OMAF_NS_END
