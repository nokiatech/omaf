
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
#include <list>
#include <cctype>

#if defined(WIN32)
#include <windows.h>
#include <direct.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#else
#endif

#include "utils.h"

namespace VDD
{
    namespace Utils
    {
        std::string replace(std::string aStr,
                            const std::string& aPattern,
                            const std::string& aReplace)
        {
            size_t pos = 0;
            while ((pos = aStr.find(aPattern, pos)) != std::string::npos) {
                aStr.replace(pos, aPattern.size(), aReplace);
                pos += aReplace.size();
            }
            return aStr;
        }

        std::list<std::string> split(std::string aStr, char aSeparator)
        {
            std::list<std::string> ret;
            size_t wordBeginOfs = 0;
            for (size_t n = 0; n < aStr.size(); ++n)
            {
                if (aStr[n] == aSeparator)
                {
                    ret.push_back(aStr.substr(wordBeginOfs, n - wordBeginOfs));
                    wordBeginOfs = n + 1;
                }
            }
            ret.push_back(aStr.substr(wordBeginOfs, aStr.size() - wordBeginOfs));
            return ret;
        }

        // Works only for ASCII
        std::string lowercaseString(const std::string& aString)
        {
            std::string str;
            str.reserve(aString.size());
            for (auto x : aString)
            {
                str.push_back(std::tolower(x));
            }
            return str;
        }

        std::string baseName(std::string aFilename)
        {
            auto index = aFilename.rfind('.');
            if (index == std::string::npos)
            {
                return aFilename;
            }
            else
            {
                return aFilename.substr(0, index);
            }
        }

        double getSeconds()
        {
#if defined(WIN32)
            return double(GetTickCount()) / 1000.0;
#elif defined(__unix__) || defined(__APPLE__)
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return tv.tv_sec + tv.tv_usec / 1000000.0;
#else
#error ("no getSeconds available for platform")
#endif
        }

        uint32_t reverse(register uint32_t x)
        {
            x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
            x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
            x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
            x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
            return((x >> 16) | (x << 16));
        }

        bool makeDirectory(const std::string& aDirectory)
        {
#if defined _WIN32
            return _mkdir(aDirectory.c_str()) == 0;
#else
            return mkdir(aDirectory.c_str(), 0777) == 0;
#endif
        }

        FilenameComponents filenameComponents(std::string aFilename)
        {
            FilenameComponents components {};
#if defined _WIN32
            std::set<char> separators { '/', '\\' };
#else
            std::set<char> separators { '/' };
#endif
            size_t basenameIndex = 0;
            // find the rightmost separator
            for (size_t index = 0; index < aFilename.size() && basenameIndex == 0; ++index)
            {
                size_t revIndex = aFilename.size() - 1 - index;
                char ch = aFilename[revIndex];
                if (separators.count(ch))
                {
                    basenameIndex = revIndex + 1;
                }
            }

            components.path = aFilename.substr(0, basenameIndex);
            components.basename = aFilename.substr(basenameIndex);
            return components;
        }
    }
}
