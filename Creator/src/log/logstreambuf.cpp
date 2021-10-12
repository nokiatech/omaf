
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
#include "logstreambuf.h"

#include "logstream.h"

namespace VDD
{
    LogStreamBuf::LogStreamBuf(LogStream& aLogStream)
        : mLogStream(aLogStream)
    {
        mLogStream.clear();
    }

    int LogStreamBuf::sync()
    {
        return 0;
    }

    int LogStreamBuf::overflow(int ch)
    {
        if (ch == '\n')
        {
            mLogStream.writeLine(mLineBuffer);
            mLineBuffer.clear();
        }
        else
        {
            mLineBuffer += ch;
        }
        return 0;
    }

    LogStreamBuf::~LogStreamBuf()
    {
        if (mLineBuffer.size())
        {
            mLogStream.writeLine(mLineBuffer);
        }
    }
}
