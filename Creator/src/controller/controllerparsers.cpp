
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
#include "controllerparsers.h"

#include <cassert>

#include "common/rational.h"
#include "mediainput.h"

namespace VDD
{
    TileConfiguration readTileConfiguration(const VDD::ConfigValue& aConfig)
    {
        TileConfiguration tile;
        try
        {
            tile.center.latitude.num = readInt32(aConfig["center_latitude"]);
            tile.center.latitude.den = 1;
        }
        catch (ConfigValueTypeMismatches&)
        {
            tile.center.latitude =
                readGeneric<Rational<int32_t>>("Tile center latitude")(aConfig["center_latitude"]);
        }
        try
        {
            tile.center.longitude.num = readInt32(aConfig["center_longitude"]);
            tile.center.longitude.den = 1;
        }
        catch (ConfigValueTypeMismatches&)
        {
            tile.center.longitude = readGeneric<Rational<int32_t>>("Tile center longitude")(
                aConfig["center_longitude"]);
        }
        try
        {
            tile.span.horizontal.num = readUint32(aConfig["span_horizontal"]);
            tile.span.horizontal.den = 1;
        }
        catch (ConfigValueTypeMismatches&)
        {
            tile.span.horizontal =
                readGeneric<Rational<uint32_t>>("Horizontal tile span")(aConfig["span_horizontal"]);
        }

        try
        {
            tile.span.vertical.num = readUint32(aConfig["span_vertical"]);
            tile.span.vertical.den = 1;
        }
        catch (ConfigValueTypeMismatches&)
        {
            tile.span.vertical =
                readGeneric<Rational<uint32_t>>("Vertical tile span")(aConfig["span_vertical"]);
        }
        return tile;
    }

    std::map<std::string, TileConfiguration> readTilerConfigs(const VDD::ConfigValue& aTilerConfig)
    {
        std::map<std::string, TileConfiguration> config;
        for (const VDD::ConfigValue& value : aTilerConfig.childValues())
        {
            if (value.getName() != "DEFAULTS")
            {
                config[value.getName()] = readTileConfiguration(value);
            }
        }

        return config;
    }

    MediaInputConfig readMediaInputConfig(const VDD::ConfigValue& aMP4LoaderConfig)
    {
        auto filename = aMP4LoaderConfig["filename"];
        auto segmented = aMP4LoaderConfig["segmented"];
        MediaInputConfig config{};
        if (filename && segmented)
        {
            throw ConfigError(
                "Needs to have one of \"filename\" or \"segmented\" for MP4 loader, "
                "not both");
        }
        else if (!filename && !segmented)
        {
            throw ConfigError("Needs to have either \"filename\" or \"segmented\" for MP4 loader");
        }
        else if (filename)
        {
            config.filename = readFilename(filename);
        }
        else if (segmented)
        {
            config.filename = readString(segmented["template"]);
            config.initFilename = readOptional(readString)(segmented["initialization"]);
            config.startNumber = readOptional(readUInt)(segmented["start_number"]);
        }
        else
        {
            assert(false);
        }
        config.gopLength = readOptional(readInt)(aMP4LoaderConfig["gop_length"]);
        return config;
    }

    std::tuple<FrameTime, FrameDuration> readTimeDuration(const ConfigValue& aValue)
    {
        std::tuple<FrameTime, FrameDuration> r{};
        std::get<0>(r) = readGeneric<FrameTime>("sample time")(aValue["timestamp"]);
        std::get<1>(r) = optionWithDefault(
            aValue, "duration", readGeneric<FrameDuration>("sample duration"), FrameDuration());

        return r;
    }
}  // namespace VDD
