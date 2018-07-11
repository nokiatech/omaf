
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

#include <iostream>
#include <string>

#include "loglevel.h"
#include "logstreambuf.h"

namespace VDD
{
    class Log;

    class LogStream: public std::ostream
    {
    public:
        LogStream(LogLevel, Log&);

        LogStream(const LogStream&) = delete;
        void operator=(const LogStream&) = delete;

        ~LogStream();

        void writeLine(const std::string&);

    private:
        LogStreamBuf mLogStreamBuf;
        LogLevel mLogLevel;
        Log& mLog;
    };
}
