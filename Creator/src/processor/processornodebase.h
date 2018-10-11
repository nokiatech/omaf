
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

#include <list>
#include <vector>
#include <memory>

#include "common/exceptions.h"
#include "log/loglevel.h"

namespace VDD
{
    class LoggerNotSetException : public Exception
    {
    public:
        LoggerNotSetException();
    };

    class Log;
    class LogStream;

    class ProcessorNodeBase
    {
    public:
        ProcessorNodeBase();
        ~ProcessorNodeBase();

        void setLog(std::shared_ptr<Log> aLog);

        Log& getLog();
        LogStream& log(LogLevel aLog);

        virtual std::string getGraphVizDescription() { return ""; }

    private:
        std::shared_ptr<Log> mLog;
    };
}
