
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

/* This file contains configuration parsers for various controller-specific needs that are shared by
 * multiple compilation units. */

#include <map>
#include <string>

#include "config/config.h"
#include "tiler/tiler.h"
#include "medialoader/mp4loader.h"
#include "mediainput.h"

namespace VDD
{
    TileConfiguration readTileConfiguration(const VDD::ConfigValue& aTilerConfig);

    std::map<std::string, TileConfiguration> readTilerConfigs(const VDD::ConfigValue& aTilerConfig);

    template <typename T> std::istream& operator>>(std::istream& aStream, Rational<T>& aRational);

    /** @brief Load MP4 loader configuration from JSON
     *
     *  aMP4LoaderConfig should point to the object that contains "filename" or "segmented". If it
     * contains only the former then a configuration for reading a single MP4 file is generated, if
     * it ontains only the latter then a configuration for reading segmented MP4 data is generated.
     *
     * If neither or if both then an error is thrown.
     */
    MediaInputConfig readMediaInputConfig(const VDD::ConfigValue& aMP4LoaderConfig);

    std::tuple<FrameTime, FrameDuration> readTimeDuration(const ConfigValue& aValue);
}

#include "controllerparsers.icpp"
