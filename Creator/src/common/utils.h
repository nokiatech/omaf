
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
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <set>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <memory>

namespace VDD
{
    namespace Utils
    {
        template <typename T>
        std::string to_string(T a)
        {
            std::ostringstream st;
            st << a;
            return st.str();
        }

        template <typename T>
        T from_string(std::string a)
        {
            std::istringstream st(a);
            T x;
            st >> x;
            return x;

        }

        template <typename T>
        std::string joinStrings(const T& aValues, std::string aSeparator)
        {
            std::string result;
            bool first = true;
            for (const auto& value : aValues)
            {
                if (!first)
                {
                    result += aSeparator;
                }
                result += value;
                first = false;
            }
            return result;
        }

        // Works only for ASCII
        std::string lowercaseString(const std::string& aString);

        template <typename T, typename... Args>
        std::unique_ptr<T> make_unique(Args&&... args)
        {
            return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

        template <typename T, typename U>
        std::unique_ptr<T> static_cast_unique_ptr(std::unique_ptr<U>&& a_ptr)
        {
            return std::unique_ptr<T>{static_cast<T*>(a_ptr.release())};
        }

        std::string replace(std::string aStr,
                            const std::string& aPattern,
                            const std::string& aReplace);

        std::list<std::string> split(std::string aStr, char aSeparator);

        std::string baseName(std::string aFilename);

        // Given a reference type, return its value version
        // Useful only in templates (in conjunction with decltype
        template <typename T> T valuefy(const T& reference);

        template <typename T>
        auto setify(const T& container) -> std::set<decltype(valuefy(*container.begin()))>
        {
            return { container.begin(), container.end() };
        }

        // Get some time stamp usable for time comparisons
        double getSeconds();

        // reverse bit order of unsigned 32 bit int
        uint32_t reverse(register uint32_t x);

        template <typename T, typename U>
        std::map<U, T> reverseMapping(const std::map<T, U>& aMap)
        {
            std::map<U, T> map;
            for (const auto& kv : aMap)
            {
                map[kv.second] = kv.first;
            }
            return map;
        }

        bool makeDirectory(const std::string& directory);

        struct FilenameComponents
        {
            std::string path; // including possible trailing separator
            std::string basename; // name of the file
        };

        FilenameComponents filenameComponents(std::string aFilename);
    }
}  // namespace VDD::Utils

#endif  // COMMON_UTILS_H
