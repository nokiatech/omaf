
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

#include "async/future.h"
#include "processor/source.h"
#include "processor/data.h"
#include "controller/common.h"
#include "./omaf/parser/h265datastructs.hpp"

namespace VDD {

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
         * Mp4 track id. Not the same as stream id, as stream Id is unique, track id is overlapping (e.g. for different qualities)
         */
        TrackId                trackId;
        /*
         * The tile index
         */
        int64_t                tileIndex;
    };

    struct Projection
    {
        OmafProjectionType projection;
        // in future could have custom faceorder and transform here
    };

    struct TileGrid
    {
        std::vector<uint16_t> columnWidths;  // In pixels; note PPS needs in CTBs
        std::vector<uint16_t> rowHeights;    // In pixels; note PPS needs in CTBs
    };

    struct TileSingle
    {
        StreamAndTrack ids;

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
