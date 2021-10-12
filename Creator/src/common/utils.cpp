
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
#include <algorithm>
#include <cctype>
#include <list>

#if defined(_WIN32)
#include <direct.h>
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#else
#endif

#include <mutex>
#include <regex>

// Nice, it seems with MSVC something defines some macros..
#undef min
#undef max

#include "utils.h"

namespace VDD
{
    namespace Utils
    {
        std::string twoLastElementsAndNumber(const char* aFile, int aLine)
        {
            int count = 0;
            bool found = false;
            int sepIndex;
            for (sepIndex = int(std::strlen(aFile)) - 1; !found && sepIndex >= 0;
                 !found && --sepIndex)
            {
                if (aFile[sepIndex] == '/' || aFile[sepIndex] == '\\')
                {
                    ++count;
                    if (count == 2)
                    {
                        found = true;
                    }
                }
            }
            std::string str;
            str.reserve(5u + (found                                         //
                              ? std::strlen(aFile) + size_t(sepIndex) + 1u  //
                              : std::strlen(aFile)));
            str += found ? std::string(aFile + sepIndex + 1) : std::string(aFile);
            str += ':';
            str += std::to_string(aLine);
            ;
            return str;
        }

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
#if defined(_WIN32)
            return double(GetTickCount()) / 1000.0;
#elif defined(__unix__) || defined(__APPLE__)
            struct timeval tv;
            gettimeofday(&tv, nullptr);
            return tv.tv_sec + tv.tv_usec / 1000000.0;
#else
#error ("no getSeconds available for platform")
#endif
        }

        uint32_t reverse(uint32_t x)
        {
            x = (((x & 0xaaaaaaaa) >> 1) | ((x & 0x55555555) << 1));
            x = (((x & 0xcccccccc) >> 2) | ((x & 0x33333333) << 2));
            x = (((x & 0xf0f0f0f0) >> 4) | ((x & 0x0f0f0f0f) << 4));
            x = (((x & 0xff00ff00) >> 8) | ((x & 0x00ff00ff) << 8));
            return((x >> 16) | (x << 16));
        }

        // thanks https://stackoverflow.com/questions/6218325/how-do-you-check-if-a-directory-exists-on-windows-in-c
        bool directoryExists(std::string aPath)
        {
#if defined _WIN32
            DWORD dwAttrib = GetFileAttributes(aPath.c_str());

            return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
            if (access(aPath.c_str(), 0) == 0)
            {
                struct stat status;
                stat(aPath.c_str(), &status);

                return (status.st_mode & S_IFDIR) != 0;
            }
            else
            {
                return false;
            }
#endif
        }

        bool makeDirectory(const std::string& aDirectory)
        {
#if defined _WIN32

            return _mkdir(aDirectory.c_str()) == 0;
#else
            return mkdir(aDirectory.c_str(), 0777) == 0;
#endif
        }

        const FilesystemConfiguration fsWindows = {
            {'/', '\\'}, {':'}, {"\\\\"}
        };

        const FilesystemConfiguration fsOther = {
            {'/'}, {}, {}
        };

#if defined _WIN32
        const FilesystemConfiguration& fsDefault = fsWindows;
#else
        const FilesystemConfiguration& fsDefault = fsOther;
#endif

        PathComponents pathComponents(std::string aPath, const FilesystemConfiguration& aFs)
        {
            PathComponents pcs;
            std::string component;
            size_t index = 0u;
            bool hasVolume = aFs.volumePrefix.size() && aPath.size() >= aFs.volumePrefix.size() &&
                            aPath.substr(0, aFs.volumePrefix.size()) == aFs.volumePrefix;
            if (hasVolume)
            {
                pcs.absolute = true;
                pcs.volume = aFs.volumePrefix;
                index = aFs.volumePrefix.size();
                for (bool inVolume = true; inVolume && index <= aPath.size(); ++index)
                {
                    bool end = index == aPath.size();
                    char ch = end ? 0 : aPath[index];
                    if (end)
                    {
                        // hackish perhaps. perhaps we're faking too hard to be general while really
                        // we just work for windows volumes :).
                        pcs.volume += aFs.volumePrefix[0];
                    }
                    else
                    {
                        pcs.volume += ch;
                    }
                    bool isSeparator = aFs.pathSeparators.count(ch);
                    inVolume = !isSeparator;
                }
            }
            else if (pcs.volume.size() == 0u && aPath.size() &&
                     aFs.pathSeparators.count(aPath.at(0)))
            {
                pcs.absolute = true;
                ++index;
            }
            bool expectVolume = aFs.volumeSeparators.size() && pcs.volume.size() == 0;
            bool consideringAbsolute = false;
            for (; index <= aPath.size(); ++index)
            {
                bool end = index == aPath.size();
                char ch = end ? 0 : aPath[index];
                bool isVolumeSeparator = expectVolume && aFs.volumeSeparators.count(ch);
                if (isVolumeSeparator)
                {
                    component += ch;
                    expectVolume = false;
                    pcs.volume = std::move(component);
                    component.clear();
                    consideringAbsolute = true;
                }
                else
                {
                    bool isSeparator = aFs.pathSeparators.count(ch);
                    if (isSeparator || end)
                    {
                        if (consideringAbsolute)
                        {
                            pcs.absolute = true;
                        }
                        if (component.size())
                        {
                            pcs.components.push_back(std::move(component));
                            component.clear();
                        }
                    }
                    else
                    {
                        component += ch;
                    }
                    consideringAbsolute = false;
                }
            }
            return pcs;
        }

        std::string joinPathComponents(std::string aParent, std::string aChild, const FilesystemConfiguration& aFs)
        {
            if (aParent.size())
            {
                return aParent + *aFs.pathSeparators.begin() + aChild;
            }
            else
            {
                return aChild;
            }
        }

        FilenameComponents filenameComponents(std::string aFilename, const FilesystemConfiguration& aFs)
        {
            FilenameComponents components {};
            size_t basenameIndex = 0;
            // find the rightmost separator
            for (size_t index = 0; index < aFilename.size() && basenameIndex == 0; ++index)
            {
                size_t revIndex = aFilename.size() - 1 - index;
                char ch = aFilename[revIndex];
                if (aFs.pathSeparators.count(ch))
                {
                    basenameIndex = revIndex + 1;
                }
            }

            components.path = aFilename.substr(0, basenameIndex);
            components.basename = aFilename.substr(basenameIndex);
            return components;
        }

        void ensurePathForFilename(std::string aPath, const FilesystemConfiguration& aFs)
        {
            static std::mutex mutex;
            std::unique_lock<std::mutex> lock(mutex);

            auto components = pathComponents(filenameComponents(aPath).path, aFs).components;
            std::string path;
            for (auto component: components)
            {
                path += component;
                if (!directoryExists(path))
                {
                    makeDirectory(path);
                    if (!directoryExists(path))
                    {
                        throw CannotOpenFile(path);
                    }
                }

                path = joinPathComponents(path, "", aFs);
            }
        }

        std::string eliminateCommonPrefix(std::string aPrefix, std::string aFilename,
                                          const FilesystemConfiguration& aFs)
        {
            PathComponents prefixComponents = pathComponents(aPrefix, aFs);
            FilenameComponents fnComponents = filenameComponents(aFilename, aFs);
            PathComponents fnPathComponents = pathComponents(fnComponents.path, aFs);
            std::string filename;

            // difficult to do anything in this case, so let's do nothing
            if (prefixComponents.absolute != fnPathComponents.absolute ||
                prefixComponents.volume != fnPathComponents.volume)
            {
                filename = aFilename;
            }
            else
            {
                size_t filenamePathIndex = 0u;

                while (filenamePathIndex < std::min(prefixComponents.components.size(),
                                                    fnPathComponents.components.size()) &&
                       prefixComponents.components[filenamePathIndex] ==
                           fnPathComponents.components[filenamePathIndex])
                {
                    ++filenamePathIndex;
                }

                while (filenamePathIndex < fnPathComponents.components.size())
                {
                    auto& pathComponent = fnPathComponents.components[filenamePathIndex];
                    filename = joinPathComponents(filename, pathComponent, aFs);
                    ++filenamePathIndex;
                }

                filename = joinPathComponents(filename, fnComponents.basename, aFs);
            }

            return filename;
        }

#if defined(_WIN32) || defined(_WIN64)
        std::vector<std::string> listDirectoryContents(std::string directory,
                                                       bool withLeadingDirectory, std::string regex)
        {
            std::vector<std::string> files;

            TCHAR dir[MAX_PATH];
            WIN32_FIND_DATA ffd;

            StringCchCopy(dir, MAX_PATH, directory.c_str());
            StringCchCat(dir, MAX_PATH, TEXT("\\*"));
            HANDLE hFind = FindFirstFile(dir, &ffd);

            std::regex matcher(regex,
                               std::regex_constants::ECMAScript | std::regex_constants::icase);

            if (hFind != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (ffd.cFileName[0] != '.' && std::regex_search(ffd.cFileName, matcher))
                    {
                        if (withLeadingDirectory)
                        {
                            files.push_back(std::string(directory + "\\" + ffd.cFileName));
                        }
                        else
                        {
                            files.push_back(std::string(ffd.cFileName));
                        }
                    }
                } while (FindNextFile(hFind, &ffd));
                FindClose(hFind);
            }

            return files;
        }
#else
        std::vector<std::string> listDirectoryContents(std::string directory,
                                                       bool withLeadingDirectory, std::string regex)
        {
            std::vector<std::string> files;
            DIR* dir = opendir(directory.c_str());

            std::regex matcher(regex,
                               std::regex_constants::ECMAScript | std::regex_constants::icase);

            if (dir)
            {
                struct dirent* entry;
                while ((entry = readdir(dir)))
                {
                    if (entry->d_name[0] != '.' && std::regex_search(entry->d_name, matcher))
                    {
                        if (withLeadingDirectory)
                        {
                            files.push_back(std::string(directory + "/" + entry->d_name));
                        }
                        else
                        {
                            files.push_back(std::string(entry->d_name));
                        }
                    }
                }
                closedir(dir);
            }
            return files;
        }
#endif

    }  // namespace Utils
}
