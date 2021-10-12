
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
#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <streamsegmenter/optional.hpp>

namespace VDD
{
    template <typename T>
    using Optional = StreamSegmenter::Utils::Optional<T>;

    const auto none = StreamSegmenter::Utils::none;

    template <typename T>
    Optional<T> makeOptional(const T& value)
    {
        return StreamSegmenter::Utils::makeOptional(value);
    }

    template <typename T>
    Optional<T> makeOptional(T&& value)
    {
        return StreamSegmenter::Utils::makeOptional(std::move(value));
    }

    template <typename First, typename... Rest>
    First coalesce(First value, Rest... rest)
    {
        return StreamSegmenter::Utils::coalesce(value, rest...);
    }

    /** Generic "find value from associative container, or don't" function to use with maps.
     *
     * findOptional(key, map) returns map[key] if it exists, otherwise it returns a None
     */
    template <typename Container, typename Key>
    auto findOptional(const Key& aKey, const Container& aContainer)
        -> Optional<typename Container::mapped_type>
    {
        auto value = aContainer.find(aKey);
        if (value != aContainer.end())
        {
            return {value->second};
        }
        else
        {
            return {};
        }
    }
}  // namespace VDD

#endif  // OPTIONAL_H
