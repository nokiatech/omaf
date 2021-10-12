
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

#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "bbox.hpp"

#include "avccommondefs.hpp"

namespace StreamSegmenter
{
    namespace Utils
    {
        std::string stringOfAvcNalUnitType(AvcNalUnitType naluType);

        template <typename Container>
        std::string join(std::string aSeparator, const Container& aContainer)
        {
            std::string str;
            bool first = true;
            for (auto value : aContainer)
            {
                if (!first)
                {
                    str += aSeparator;
                }
                str += value;
                first = false;
            }
            return str;
        }

        template <typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args... args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        template <typename T, typename U, typename Deleter>
        std::unique_ptr<T, Deleter> static_cast_unique_ptr(std::unique_ptr<U, Deleter>&& aPtr)
        {
            return std::unique_ptr<T, Deleter>{static_cast<T*>(aPtr.release())};
        }

        template <typename Container>
        std::set<typename Container::value_type> setOfContainer(const Container& container)
        {
            return std::set<typename Container::value_type>(container.begin(), container.end());
        }

        template <typename Container, typename Function>
        auto setOfContainerMap(Function map, const Container& container) -> std::set<decltype(map(*container.begin()))>
        {
            std::set<decltype(map(*container.begin()))> set;
            for (const auto& x : container)
            {
                set.insert(map(x));
            }
            return set;
        }

        template <typename T>
        std::string to_string(T a)
        {
            std::ostringstream st;
            st << a;
            return st.str();
        }

        template <typename T>
        std::ostream& operator<<(std::ostream& aStream, std::list<T> aElements)
        {
            bool first = true;
            aStream << "[";
            for (auto& x : aElements)
            {
                if (!first)
                {
                    aStream << ", ";
                }
                aStream << x;
                first = false;
            }
            aStream << "]";
            return aStream;
        }

        template <typename Iterator>
        void hexDump(std::ostream& stream, Iterator begin, Iterator end)
        {
            Iterator frameAccessors = begin;
            bool first              = true;
            while (frameAccessors != end)
            {
                stream << (first ? "" : " ") << std::setw(2) << std::setfill('0') << std::hex
                       << unsigned(*frameAccessors);
                first = false;
                ++frameAccessors;
            }
        }

        // Useful for extracting track ids out of maps with track id as first element
        template <typename Container>
        auto vectorOfKeys(const Container& aContainer) -> std::vector<typename Container::key_type>
        {
            std::vector<typename Container::key_type> xs;

            for (const auto& x : aContainer)
            {
                xs.push_back(x.first);
            }

            return xs;
        }

        void dumpBox(std::ostream& stream, ::Box& box);

        class TrueOnce
        {
        public:
            bool operator()()
            {
                if (mFirst)
                {
                    mFirst = false;
                    return true;
                }
                else
                {
                    return false;
                }
            }

        private:
            bool mFirst = true;
        };
    }  // Utils
}  // StreamSegmenter
