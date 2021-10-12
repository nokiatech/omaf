
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

#include "VAS/NVRVASTilePicker.h"

OMAF_NS_BEGIN

/*
 * A simplified version of TilePicker; picks just one tile which is assumed to be a tile set, which covers more than a
 * viewport
 * Used with cases where extractor track has predefined tile sets
 */
class VASTileSetPicker : public VASTilePicker
{
public:
    VASTileSetPicker();
    virtual ~VASTileSetPicker();

    virtual VASTileSetContainer* getLatestTileSet(bool_t& selectionUpdated,
                                                  VASTileSetContainer** dropped,
                                                  VASTileSetContainer** additional);

public:  // from VASTilePicker
    virtual void_t setupTileRendering(VASTilesLayer& allTiles,
                                      float64_t aFovHorizontal,
                                      float64_t aFovVertical,
                                      uint32_t aBaseLayerDecoderPixelsInSec,
                                      VASLongitudeDirection::Enum aDirection);
    virtual VASTileSelection& pickTiles(VASTilesLayer& allTiles, uint32_t aBaseLayerDecoderPixelsInSec);

    virtual size_t getNrVisibleTiles(VASTilesLayer& aTiles,
                                     float64_t aViewportWidth,
                                     float64_t aViewportHeight,
                                     VASLongitudeDirection::Enum aDirection);

private:
    bool_t findBestTileSet(const VASTilesLayer& allTiles, VASTileSelection& selectedTiles);
    bool_t findExactTiles(VASTileSetContainer* aSelectedTileSet);


private:
    VASTileSetContainer* mSelectedTileSet;    // tiles selected in rendering thread; L+R
    VASTileSetContainer* mDownloadedTileSet;  // tiles used in provider thread; L+R
};
OMAF_NS_END
