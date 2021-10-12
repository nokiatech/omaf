
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
#pragma once

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include "loglevel.h"
#include "logstream.h"
#include "logstreambuf.h"

namespace VDD
{
    class Log
    {
    public:
        Log();
        virtual ~Log();

        /** For convenience, same as log(LogLevel) */
        LogStream& operator()(LogLevel);

        LogStream& log(LogLevel);

        LogLevel getLogLevel() const;
        void setLogLevel(LogLevel aLevel);

        void writeLine(LogLevel, const std::string&);
        virtual void progress(size_t aFrameIndex);
        virtual void progress(size_t aFrameIndex, std::string aFileName);

        // Create another instance that can be used from another thread at the same time
        std::shared_ptr<Log> instance();

    protected:
        friend class LogStream;

        virtual Log* sharedClone() = 0;
        virtual void writeLineImpl(LogLevel, const std::string&) = 0;

    private:
        LogLevel mLogLevel = LogLevel::Info;
        std::map<LogLevel, std::unique_ptr<LogStream>> mLogStreams;
    };
}
