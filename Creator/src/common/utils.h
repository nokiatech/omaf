
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
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <cstring>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#include <Strsafe.h>
#include <direct.h>
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "exceptions.h"

#define STRINGIFY(x) #x
#define STRIGIFY2(x) STRINGIFY(x)
#define STRING__LINE__ STRIGIFY2(__LINE__)
#define VDD_SHORT_HERE VDD::Utils::twoLastElementsAndNumber(__FILE__, __LINE__)

namespace VDD
{
    namespace Utils
    {
        // for use with VDD_SHORT_FILE
        std::string twoLastElementsAndNumber(const char* aFile, int aLine);

        template <typename T>
        std::string to_string(const T& a)
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

        template <typename T, typename U>
        T append(const T& aContainer, const U& aValue)
        {
            T ret(aContainer);
            ret.push_back(aValue);
            return ret;
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
        uint32_t reverse(uint32_t x);

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

        // Exposed for testability
        struct FilesystemConfiguration
        {
            const std::set<char> pathSeparators;
            const std::set<char> volumeSeparators;
            const std::string volumePrefix;
        };

        extern const FilesystemConfiguration fsWindows;
        extern const FilesystemConfiguration fsOther;
        extern const FilesystemConfiguration& fsDefault;

        struct PathComponents
        {
            bool absolute = false;
            // ie. "VOLUME:\" or "\\SERVER\" on windows; empty on other platforms.
            // the trailing \ exists only if the path was absolute or missing
            std::string volume;
            std::vector<std::string> components;
        };

        PathComponents pathComponents(std::string aPath, const FilesystemConfiguration& aFs = fsDefault);

        struct FilenameComponents
        {
            std::string path; // including possible trailing separator
            std::string basename; // name of the file
        };

        FilenameComponents filenameComponents(std::string aFilename, const FilesystemConfiguration& aFs = fsDefault);

        /** @brief Concatenate two file name components, ie "aParent/aChild"
         *
         * if aParent is empty, no separator is added.
         *
         * Note that on Windows there can be two kinds of path separators and this just uses one of
         * them.
         **/
        std::string joinPathComponents(std::string aParent, std::string aChild, const FilesystemConfiguration& aFs = fsDefault);

        /** @brief ensure there is a path to write the given filename by creating each missing
         * directory to it
         *
         * Throws CannotOpenFile(partial or full path) on error.
         *
         * @note Uses an internal mutex to ensure at least this same process isn't racing against
         * itself; it could race against other consumers of the file system
         */
        void ensurePathForFilename(std::string aFilename, const FilesystemConfiguration& aFs = fsDefault);

        /** @brief Elimiante common prefix from the given file name
         *
         * ie. eliminateCommonPrefix("foo/bar", "foo/bar/baz/bal") becomes "baz/bal"
         *
         * Used when generating media file names for the MPD
         */
        std::string eliminateCommonPrefix(std::string aPrefix, std::string aFilename,
                                          const FilesystemConfiguration& aFs = fsDefault);

        /** List directory contents.
         *
         * @param directory The directory to list the contents of
         * @param withLeadingDirectory If true, prefix each returned file name with the directory */
        std::vector<std::string> listDirectoryContents(std::string directory,
                                                       bool withLeadingDirectory,
                                                       std::string regex = ".");

    }
}  // namespace VDD::Utils

#endif  // COMMON_UTILS_H
