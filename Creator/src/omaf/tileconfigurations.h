
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

#include <memory>
#include <vector>

#include "tileconfig.h"
#include "tilefilter.h"
#include "common/optional.h"
#include "controller/videoinput.h"

namespace VDD {
    class ConfigValue;
    struct VideoFileProperties;

    enum class VDMode
    {
        Invalid = -1,
        MultiQ,
        MultiRes5K,
        MultiRes6K,
        COUNT
    };

    enum class VideoRole
    {
        FullRes,        /* for 5K and 6K */
        MediumRes,      /* for 6K */
        MediumResPolar, /* for 6K */
        LowRes,         /* for 5K */
        LowResPolar     /* for 6K */
    };

    std::string stringOfVideoRole(VideoRole aVideoRole);

    VideoRole readRole(const ConfigValue& aConfig);

    Optional<VideoRole> detectVideoRole(const VideoFileProperties& aVideoFile, VDMode aVDMode);

    namespace TileConfigurations {
        struct TiledInputVideo
        {
            uint32_t width;
            uint32_t height;
            uint8_t qualityRank;
            std::vector<OmafTileSetConfiguration> tiles;
            int ctuSize;
        };

        size_t createERP5KConfig(TileMergeConfig& aMergedConfig, const TiledInputVideo& aFullResVideo, const TiledInputVideo& aLowResVideo, StreamAndTrackIds& aExtractorIds);
        size_t createERP6KConfig(TileMergeConfig& aMergedConfig,
                                 const TiledInputVideo& aFullResVideo,
                                 const TiledInputVideo& aMediumResVideo,
                                 const TiledInputVideo& aMediumResPolarVideo,
                                 const TiledInputVideo& aLowResPolarVideo,
                                 StreamAndTrackIds& aExtractorIds, TrackIdProvider aTrackIdProvider,
                                 StreamIdProvider aStreamIdProvider);

        // Returns the removed tiles
        OmafTileSets crop6KMainVideo(OmafTileSets& aTileConfig, VideoInputMode aInputVideoMode);
        OmafTileSets crop6KPolarVideo(OmafTileSets& aTileConfig, VideoInputMode aInputVideoMode);
    }  // namespace TileConfigurations
}
