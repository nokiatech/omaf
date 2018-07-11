
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

#include "VAS/NVRVASTilePicker.h"

OMAF_NS_BEGIN
    
    /*
     * A simplified version of TilePicker; picks just one tile which is assumed to be a tile set, which covers more than a viewport
     * Used with cases where extractor track has predefined tile sets
     */
    class VASTileSetPicker : public VASTilePicker
    {
    public:
        VASTileSetPicker();
        virtual ~VASTileSetPicker();

        virtual void_t setupTileRendering(VASTilesLayer& allTiles, float64_t width, float64_t height, uint32_t aBaseLayerDecoderPixelsInSec);
        virtual VASTileSelection& pickTiles(VASTilesLayer& allTiles, uint32_t aBaseLayerDecoderPixelsInSec);

        virtual VASTileSelection& getLatestTiles(bool_t& selectionUpdated, VASTileSelection& dropped, VASTileSelection& additional);

    private:
        bool_t findBestTileSet(const VASTilesLayer& allTiles, VASTileSelection& selectedTiles);

    };
OMAF_NS_END
