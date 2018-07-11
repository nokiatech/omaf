
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

#include "log.h"

namespace VDD
{
    class ConsoleLog : public Log
    {
    public:
        ConsoleLog();
        ~ConsoleLog();

    protected:
        void writeLineImpl(LogLevel, const std::string&) override;
        ConsoleLog* sharedClone() override;
    };
}
