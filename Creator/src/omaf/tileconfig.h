
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

#include "async/future.h"
#include "processor/source.h"
#include "processor/data.h"
#include "controller/common.h"
#include "./omaf/parser/h265datastructs.hpp"

namespace VDD {
    static const unsigned TileIDCLevel51 = 51;

    struct OmafTileSetConfiguration
    {
        /*
         * Label for the config in json; used in mapping the config internally
         */
        std::string            label;
        /*
         * Internal unique identifier for the subpicture
         */
        StreamId               streamId;
        /*
         * Track group id. This is unique for each tile index.
         */
        TrackGroupId           trackGroupId;
        /*
         * The tile index
         */
        TileIndex              tileIndex;
        /*
         * Track id. This is unique for each tile, much like StreamId.
         */
        TrackId                trackId;
    };

    typedef std::vector<OmafTileSetConfiguration> OmafTileSets;

    using TileIndexTrackGroupIdMap = std::map<TileIndex, TrackGroupId>;

    using TileXY = std::pair<size_t, size_t>;

    struct Projection
    {
        Optional<OmafProjectionType> projection;
        // in future could have custom faceorder and transform here
    };

    struct TileGrid
    {
        std::vector<uint16_t> columnWidths;  // In pixels; note PPS needs in CTBs
        std::vector<uint16_t> rowHeights;    // In pixels; note PPS needs in CTBs
    };

    struct TileSingle
    {
        StreamAndTrackGroup ids;

        int ctuIndex;
    };

    struct TileInGrid
    {
        Region regionPacking;
        std::vector<TileSingle> tile;
    };

    using TileRow = std::vector<TileInGrid>;
    // one for each direction
    struct TileDirectionConfig
    {
        std::string label;
        std::vector<TileRow> tiles;

        StreamAndTrack extractorId;
        Quality3d qualityRank;
    };

    struct TileMergeConfig
    {
        uint32_t packedWidth;
        uint32_t packedHeight;
        uint32_t projectedWidth;
        uint32_t projectedHeight;
        TileGrid grid;
        std::vector<TileDirectionConfig> directions;
        // these are configured when SPS/PPS from original stream is available
        H265::SequenceParameterSet extractorSPS;
        H265::PictureParameterSet extractorPPS;
    };

}
