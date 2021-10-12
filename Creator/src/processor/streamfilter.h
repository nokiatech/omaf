
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

#include <cstdint>
#include <limits>
#include <set>

#include "streamsegmenter/id.hpp"

namespace VDD
{
    using StreamId = StreamSegmenter::IdBase<std::uint32_t, class StreamIdTag>;
    static const StreamId StreamIdUninitialized = StreamId(0xffffffff);

    /** @brief Allocate a new stream id. The symbolic name is used for debugging purposes. */
    StreamId newStreamId(std::string aLabel);

    std::string to_string(const StreamId&);

    /** @brief StreamFilter is attached to connections from one node to another and indicates which stream ids should be
        passed over that edge.

        StreamFilter can either match all streams or just the listed set of stream ids. */
    class StreamFilter
    {
    public:
        /** @brief Construct a StreamFilter matching all streams */
        StreamFilter() {}
        ~StreamFilter() = default;

        /** @brief Construct a StreamFilter matching listed streams. Empty set is not permitted. */
        StreamFilter(const std::set<StreamId>& aStreams);

        StreamFilter(const StreamFilter&) = default;
        StreamFilter(StreamFilter&&) = default;
        StreamFilter& operator=(const StreamFilter&) = default;
        StreamFilter& operator=(StreamFilter&&) = default;

        /** @brief Add one stream to the filter. Note that if before the StreamFilter matched all streams (ie. it was
            default-constructed), then after adding the first stream if will match only exactly that new stream. */
        void add(StreamId aStreamId);

        /** @brief Does this filter match all streams? If so, return true. If this returns true, asSet must not be
            called. */
        bool allStreams() const;

        /** @brief Return the set of stream ids accepted by this stream filter. This can only be used to stream filters
            that don't match all streams, ie. assert(!allStreams()). */
        const std::set<StreamId>& asSet() const;

        /** @brief Return a filter that has streams both filters have */
        StreamFilter operator&(const StreamFilter& aOther) const;

        /** @brief Return a filter that has streams both filters have */
        StreamFilter& operator&=(const StreamFilter& aOther);

    private:
        // memory manamgement
        std::set<StreamId> mStreams;
    };

    extern const StreamFilter allStreams;
}
