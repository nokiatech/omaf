
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
#include "log.h"

#include "logstream.h"
#include "common/utils.h"

namespace VDD
{
    Log::Log() = default;

    LogStream& Log::operator()(LogLevel aLevel)
    {
        return log(aLevel);
    }

    void Log::progress(size_t aFrameIndex)
    {
        log(Info) << "Processing frame " << aFrameIndex << std::endl;
    }

    void Log::progress(size_t aFrameIndex, std::string aFileName)
    {
        log(Info) << "Processing frame " << aFrameIndex << " of " << aFileName << std::endl;
    }

    LogLevel Log::getLogLevel() const
    {
        return mLogLevel;
    }

    void Log::setLogLevel(LogLevel aLevel)
    {
        mLogLevel = aLevel;
    }

    void Log::writeLine(LogLevel aLevel, const std::string& aMessage)
    {
        if (aLevel <= mLogLevel)
        {
            writeLineImpl(aLevel, aMessage);
        }
    }

    LogStream& Log::log(LogLevel aLevel)
    {
        if (!mLogStreams.count(aLevel))
        {
            mLogStreams.insert(std::make_pair(aLevel, Utils::make_unique<LogStream>(aLevel, *this)));
        }
        return *mLogStreams.at(aLevel);
    }

    std::shared_ptr<Log> Log::instance()
    {
        return std::shared_ptr<Log>(sharedClone());
    }

    Log::~Log() = default;
}
