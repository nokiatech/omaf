
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
#pragma once

#include <memory>
#include <vector>

#include "tileconfig.h"
#include "tilefilter.h"


namespace VDD {
    namespace TileConfigurations {
        struct TiledInputVideo
        {
            int width;
            int height;
            uint8_t qualityRank;
            std::vector<OmafTileSetConfiguration> tiles;
            int ctuSize;
        };

        size_t createERP5KConfig(TileMergeConfig& aMergedConfig, const TiledInputVideo& aFullResVideo, const TiledInputVideo& aLowResVideo, StreamAndTrackIds& aExtractorIds);
        size_t createERP6KConfig(TileMergeConfig& aMergedConfig, const TiledInputVideo& aFullResVideo, const TiledInputVideo& aMediumResVideo, const TiledInputVideo& aMediumResPolarVideo, const TiledInputVideo& aLowResPolarVideo, StreamAndTrackIds& aExtractorIds);
        void crop6KForegroundVideo(TileFilter::OmafTileSets& aTileConfig);
        void crop6KBackgroundVideo(TileFilter::OmafTileSets& aTileConfig);
        void crop6KPolarVideo(TileFilter::OmafTileSets& aTileConfig);
        void crop6KPolarBackgroundVideo(TileFilter::OmafTileSets& aTileConfig);
    }
}
