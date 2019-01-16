
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
#include "consolelog.h"

namespace VDD
{
    ConsoleLog::ConsoleLog() = default;

    ConsoleLog::~ConsoleLog() = default;

    void ConsoleLog::writeLineImpl(LogLevel aLevel, const std::string& aMessage)
    {
        auto* stream = &std::cout;
        switch (aLevel)
        {
            case Assert: {
                stream = &std::cerr;
                *stream << "Assert: ";
                break;
            }
            case Error: {
                stream = &std::cerr;
                *stream << "Error: ";
                break;
            }
            case Info: {
                *stream << "Info: ";
                break;
            }
            case Debug: {
                *stream << "Debug: ";
                break;
            }
        }
        *stream << aMessage << std::endl;
    }

    ConsoleLog* ConsoleLog::sharedClone()
    {
        ConsoleLog* instance = new ConsoleLog();
        instance->setLogLevel(getLogLevel());
        return instance;
    }
}
