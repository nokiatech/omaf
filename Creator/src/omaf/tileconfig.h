
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
#include "tilefilter.h"

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
         * List of tiles to include
         */
        std::vector<uint64_t>  tileId;
        /*
         * Tile span (columns, rows)
         */
        struct {
            uint64_t columns;
            uint64_t rows;
        } tileSpan;

        /*
         * Extracted subpicture coordinates and resolution (left, top, width, height)
         */
        struct {
            uint64_t top;
            uint64_t left;
            uint64_t width;
            uint64_t height;
        } pixelSpan;
    };

    struct Projection
    {
        OmafProjectionType projection;
        // in future could have custom faceorder and transform here
    };
}
