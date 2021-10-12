
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
#include "logstream.h"

#include "log.h"

namespace VDD
{
    LogStream::LogStream(LogLevel aLogLevel, Log& aLog)
        : std::ostream(&mLogStreamBuf)
        , mLogStreamBuf(*this)
        , mLogLevel(aLogLevel)
        , mLog(aLog)
    {
        // nothing
    }

    LogStream::LogStream(LogStream&& aOther)
        : std::ostream(&mLogStreamBuf)
        , mLogStreamBuf(std::move(aOther.mLogStreamBuf))
        , mLogLevel(aOther.mLogLevel)
        , mLog(aOther.mLog)
    {
        // maybe the move above is not sufficient in theory..
        rdbuf(&mLogStreamBuf);
    }

    void LogStream::writeLine(const std::string& aLine)
    {
        mLog.writeLine(mLogLevel, aLine);
    }

    LogStream::~LogStream() = default;
}
