
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#if defined(ANDROID) || defined(__ANDROID__)

#ifdef ANDROID_TO_STRING_HACK
#include <sstream>
#include "customallocator.hpp"
namespace std
{
    static string to_string(int num)
    {
        ostringstream convert;
        convert << num;
        return convert.str();
    }
}
#endif  // ANDROID_TO_STRING_HACK

#ifdef ANDROID_STOI_HACK
#include <cstdlib>
namespace std
{
    static int stoi(const String& str)
    {
        return std::atoi(str.c_str());
    }
}
#endif  // ANDROID_STOI_HACK

#endif
