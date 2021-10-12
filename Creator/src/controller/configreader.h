
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

#include <sstream>
#include "config/config.h"
#include "processor/meta.h"

namespace VDD
{
    // This function is a bit more cumbersome than it needs to be due to IdBase not containing
    // some helpful typedefs nor having specializations for std::numeric_limits
    template <typename T>
    auto readId(std::string aName) -> std::function<T(const ConfigValue&)>
    {
        return [=](const ConfigValue& aValue)
        {
            T idValue;
            auto value = readIntegral<decltype(idValue.get())>(aName)(aValue);
            return T(value);
        };
    }

    const auto readTrackId = readId<TrackId>("track id");

    const auto readDimensions = VDD::readPair<std::uint32_t, std::uint32_t>("dimensions", 'x');

    // Container is a map-like container
    template <typename Container>
    auto readMapping(std::string aName, const Container& aContainer,
                     std::function<std::string(const ConfigValue&)> aReadValue = readString)
        -> std::function<typename Container::mapped_type(const ConfigValue&)>
    {
        return [=](const ConfigValue& aNode)
        {
            std::string value = aReadValue(aNode);
            auto it = aContainer.find(value);
            if (it != aContainer.end())
            {
                return it->second;
            }
            else
            {
                std::ostringstream validValues;
                bool first = true;
                for (auto x : aContainer)
                {
                    if (!first)
                    {
                        validValues << ", ";
                    }
                    first = false;
                    validValues << x.first;
                }
                throw ConfigValueInvalid(value + " isn't a valid " + aName +
                                             " (valid values: " + validValues.str() + ")",
                                         aNode);
            }
        };
    }

    // Container is a map-like container
    template <typename Container>
    std::string mappingToString(const Container& aContainer, typename Container::mapped_type aValue)
    {
        for (auto x: aContainer)
        {
            if (x.second == aValue)
            {
                return x.first;
            }
        }
        // The maps must always contain all the alternatives
        assert(false);
        return {};
    }

    const auto readSegmentDuration = readGeneric<StreamSegmenter::Segmenter::Duration>("segment duration");

}
