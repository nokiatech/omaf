
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

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <set>

#include "processor/data.h"

namespace VDD
{
    /** @brief A Streams-object represents frames (or segments or other pieces of data) at a point in time, where
        multiple Data entries are transmitted from one node to the next.

        As such, all Data entries must either represent actual data, or they all must reprepsent end-of-stream. This
        end-of-stream can be retrieved from the Streams object itself.
    */
    class Streams {
        using Container = std::vector<Data>;
    public:
        using iterator = Container::iterator;
        using const_iterator = Container::const_iterator;

        /** @brief Construct an empty Streams object.

            Streams object are considered invalid at some points, so you should fill Data with .add before passing this
            object onwards. */
        Streams() = default;

        /** @brief Construct a Streams object from given Data objects. There must be at least one Data in the
            initializer list. */
        Streams(std::initializer_list<Data> aStreams);

        /** @brief Construct a Streams object from given Data objects. There must be at least one Data in the
            range. */
        template <typename T> Streams(T aBegin, T aEnd)
            : mStreams(aBegin, aEnd)
        {
            assert(mStreams.size());
            mEndOfStream = mStreams[0].isEndOfStream();
#if defined(_DEBUG)
            for (auto& view: mStreams)
            {
                assert(view.isEndOfStream() == mEndOfStream);
            }
#endif
        }

        ~Streams() = default;

        /** @brief Return the first data. In principle this is arbitrary, but currently it's the element that was first
            .added to this Streams */
        const Data& front() const;

        /** @brief Determine the number of Data in this stream */
        size_t size() const;

        /** @brief Add a new Data. To make more sense out of multiple Data Streams, you should arrange them to have an
            asisgned StreamId. **/
        void add(const Data& aData);

        const_iterator begin() const  { return mStreams.begin(); }
        const_iterator end() const { return mStreams.end(); }

        const_iterator cbegin() const  { return mStreams.begin(); }
        const_iterator cend() const { return mStreams.end(); }

        /** @brief Is this the end of the stream? If this is true, then all Data indicate the end of stream and vice
            versa for false. */
        bool isEndOfStream() const;

        /** @brief Random-access */
        const Data& operator[](size_t aIndex) const
        {
            return mStreams[aIndex];
        }

        /** @brief Random-access */
        Data& operator[](size_t aIndex)
        {
            return mStreams[aIndex];
        }

        /** @brief Random-access */
        const Data& operator[](StreamId aStreamId) const
        {
            auto it = std::find_if(mStreams.begin(), mStreams.end(), [=](const Data& data) {
                return data.getStreamId() == aStreamId;
            });
            assert(it != mStreams.end());
            return *it;
        }

        /** @brief Random-access */
        Data& operator[](StreamId aStreamId)
        {
            auto it = std::find_if(mStreams.begin(), mStreams.end(), [=](const Data& data) {
                return data.getStreamId() == aStreamId;
            });
            assert(it != mStreams.end());
            return *it;
        }

    private:
        std::vector<Data> mStreams;
        bool mEndOfStream = false;
    };
}
