
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
#include "debugsink.h"

#include <sstream>

#include "log/logstream.h"

namespace VDD
{
    struct DebugSink::Info
    {
        std::map<StreamId, size_t> countPerStream;
        std::map<StreamId, size_t> numEndOfStreamPerStream;
    };

    DebugSink::DebugSink(Config aConfig) : mConfig(aConfig), mInfo(std::make_unique<Info>())
    {
        // nothing
    }

    DebugSink::~DebugSink()
    {
        // nothing
    }

    std::string DebugSink::getGraphVizDescription()
    {
        std::ostringstream st;

        st << "counts:";
        for (auto& [stream, count] : mInfo->countPerStream)
        {
            st << " " << to_string(stream) << ":" << count << "("
               << mInfo->numEndOfStreamPerStream[stream] << ")";
        }

        return st.str();
    }

    void DebugSink::consume(const Streams& aStreams)
    {
        std::unique_ptr<LogStream> logStream;
        if (mConfig.log)
        {
            logStream = std::make_unique<LogStream>(std::move(log(LogLevel::Info)));
        }
        if (logStream)
        {
            *logStream << getId () << " Streams ("
                       << (aStreams.isEndOfStream() ? "end of stream" : "not end of stream") << ")"
                       << ":";
        }
        for (auto& data : aStreams)
        {
            if (logStream)
            {
                *logStream << " " << to_string(data.getStreamId());
            }
            ++mInfo->countPerStream[data.getStreamId()];
            if (aStreams.isEndOfStream())
            {
                ++mInfo->numEndOfStreamPerStream[data.getStreamId()];
            }
        }
        if (logStream)
        {
            *logStream << std::endl;
        }
    }

}  // namespace VDD
