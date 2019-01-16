
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
#include "api/streamsegmenter/exceptions.hpp"

namespace StreamSegmenter
{
    ConfigValueError::ConfigValueError(std::string str, std::string path)
        : ConfigError(str)
        , mPath(path)
    {
        // nothing
    }

    std::string ConfigValueError::getPath() const
    {
        return mPath;
    }
}
