
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
#include "processornodebase.h"

#include "log/log.h"

namespace VDD
{
    LoggerNotSetException::LoggerNotSetException()
        : Exception("LoggerNotSetException")
    {
        // nothing
    }

    ProcessorNodeBase::ProcessorNodeBase() = default;
    ProcessorNodeBase::~ProcessorNodeBase() = default;

    void ProcessorNodeBase::setLog(std::shared_ptr<Log> aLog)
    {
        mLog = aLog;
    }

    Log& ProcessorNodeBase::getLog() const
    {
        if (!mLog)
        {
            throw LoggerNotSetException();
        }
        return *mLog;
    }

    LogStream& ProcessorNodeBase::log(LogLevel aLog) const
    {
        return getLog().log(aLog);
    }

    void ProcessorNodeBase::setId(unsigned int aId)
    {
        mId = aId;
    }

    void ProcessorNodeBase::ready()
    {
        // nothing
    }

    unsigned int ProcessorNodeBase::getId() const
    {
        return mId;
    }
}
