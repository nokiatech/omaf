
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
#include "streamfilter.h"

#include <cassert>
#include <cstdlib>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <utility>

namespace VDD
{
    namespace
    {
        constexpr StreamId sStreamIdNamesRangeBegin(1000000000);
        constexpr StreamId sStreamIdNamesRangeEnd(0xffffffffu);

        struct StreamIdGenerator
        {
            std::uint32_t streamIdNext;
            std::map<StreamId, std::string> streamIdNames;
            std::shared_mutex streamIdNamesMutex;
            StreamIdGenerator() : streamIdNext(sStreamIdNamesRangeBegin.get())
            {
            }
        };


        std::once_flag sStreamIdGeneratorFlag;
        StreamIdGenerator* sStreamIdGenerator = nullptr;

        static void destructStreamIdGenerator()
        {
            delete sStreamIdGenerator;
            sStreamIdGenerator = nullptr;
        }

        StreamIdGenerator& getStreamIdGenerator()
        {
            // _assume_ the first stream is allocated in a context where thread safety doesn't
            // matter

            // use this awkwards structure to ignore the problems in initialization order by using
            // only basic types in the global scope
            std::call_once(sStreamIdGeneratorFlag, [] {
                sStreamIdGenerator = new StreamIdGenerator();
                std::atexit(destructStreamIdGenerator);
            });
            return *sStreamIdGenerator;
        }
    };  // namespace

    std::string to_string(const StreamId& aStreamId)
    {
        if (aStreamId == StreamIdUninitialized)
        {
            return "<none>";
        }
        else if (aStreamId < sStreamIdNamesRangeBegin || aStreamId >= sStreamIdNamesRangeEnd)
        {
            return std::to_string(aStreamId.get());
        }
        else
        {
            auto& generator = getStreamIdGenerator();
            std::shared_lock<std::shared_mutex> lock(generator.streamIdNamesMutex);
            auto name = generator.streamIdNames.find(aStreamId);
            if (name != generator.streamIdNames.end())
            {
                return "<" + name->second + ">";
            }
            else
            {
                return std::to_string(aStreamId.get());
            }
        }
    }

    StreamId newStreamId(std::string aLabel)
    {
        auto& generator = getStreamIdGenerator();
        std::unique_lock<std::shared_mutex> lock(generator.streamIdNamesMutex);
        auto id = generator.streamIdNext;
        ++generator.streamIdNext;
        generator.streamIdNames.insert({StreamId(id), aLabel});
        return id;
    }

    StreamFilter::StreamFilter(const std::set<StreamId>& aStreams)
        : mStreams(aStreams)
    {
        assert(aStreams.size());
    }

    void StreamFilter::add(StreamId aStreamId)
    {
        mStreams.insert(aStreamId);
    }

    bool StreamFilter::allStreams() const
    {
        return mStreams.size() == 0;
    }

    const std::set<StreamId>& StreamFilter::asSet() const
    {
        assert(mStreams.size());
        return mStreams;
    }

    StreamFilter& StreamFilter::operator&=(const StreamFilter& aOther)
    {
        if (mStreams.size() == 0) {
            mStreams = aOther.mStreams;
        } else if (aOther.mStreams.size() == 0) {
            // nop
        } else {
            std::set<StreamId> toRemove;
            for (auto id: mStreams) {
                if (!aOther.mStreams.count(id)) {
                    toRemove.insert(id);
                }
            }
            for (auto id: toRemove) {
                mStreams.erase(id);
            }
        }
        return *this;
    }

    StreamFilter StreamFilter::operator&(const StreamFilter& aOther) const
    {
        if (mStreams.size() == 0) {
            return aOther;
        } else if (aOther.mStreams.size() == 0) {
            return *this;
        } else {
            StreamFilter filter;
            for (auto id: filter.mStreams) {
                if (aOther.mStreams.count(id)) {
                    filter.mStreams.insert(id);
                }
            }
            return filter;
        }
    }

    const StreamFilter allStreams;
}
