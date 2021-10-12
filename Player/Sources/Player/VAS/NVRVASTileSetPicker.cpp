
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
#include "VAS/NVRVASTileSetPicker.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "VAS/NVRVASLog.h"

OMAF_LOG_ZONE(VASTileSetPicker)

OMAF_NS_BEGIN


VASTileSetPicker::VASTileSetPicker()
    : VASTilePicker()
    , mSelectedTileSet(OMAF_NULL)
    , mDownloadedTileSet(OMAF_NULL)
{
    mTileCountRestricted = true;
}

VASTileSetPicker::~VASTileSetPicker()
{
}

// called from renderer thread
void_t VASTileSetPicker::setupTileRendering(VASTilesLayer& allTiles,
                                            float64_t aFovHorizontal,
                                            float64_t aFovVertical,
                                            uint32_t aBaseLayerDecoderPixelsInSec,
                                            VASLongitudeDirection::Enum aDirection)
{
    mTileCountRestricted = true;
}

// called from renderer thread
VASTileSelection& VASTileSetPicker::pickTiles(VASTilesLayer& allTiles, uint32_t aBaseLayerDecoderPixelsInSec)
{
    OMAF_ASSERT(allTiles.hasSeparateStereoTiles() == false, "support for separate track stereo missing");
    VASTileSelection selectedTiles;
    if (findBestTileSet(allTiles, selectedTiles))
    {
        Spinlock::ScopeLock lock(mSpinlock);
        // store the selection to wait for provider to read it
        mSelectedTileSet = (VASTileSetContainer*) selectedTiles.at(0);
        mSelectedTiles = selectedTiles;
        if (!mSelectedTileSet)
        {
            OMAF_LOG_D("EMPTY");
        }
        mSelectionUpdated = true;
        OMAF_LOG_D("Tile for (%f, %f) updated: repr %s", mRenderedViewport.getCenterLongitude(),
                   mRenderedViewport.getCenterLatitude(),
                   mSelectedTileSet->getAdaptationSet()->getCurrentRepresentationId().getData());

        mSelectedTileSet->startNew();
        findExactTiles(mSelectedTileSet);
    }
    return mSelectedTiles;
}

// called from provider thread
VASTileSetContainer* VASTileSetPicker::getLatestTileSet(bool_t& selectionUpdated,
                                                        VASTileSetContainer** droppedTile,
                                                        VASTileSetContainer** newTile)
{
    Spinlock::ScopeLock lock(mSpinlock);
    if (mSelectionUpdated)
    {
        *droppedTile = mDownloadedTileSet;
        *newTile = mSelectedTileSet;
        if (*newTile != *droppedTile)
        {
            OMAF_LOG_D("Tiles updated; old tile (%f,%f) new tile lon (%f,%f)",
                       ((*droppedTile) ? (*droppedTile)->getCoveredViewport().getCenterLongitude() : NAN),
                       ((*droppedTile) ? (*droppedTile)->getCoveredViewport().getCenterLatitude() : NAN),
                       ((*newTile) ? (*newTile)->getCoveredViewport().getCenterLongitude() : NAN),
                       ((*newTile) ? (*newTile)->getCoveredViewport().getCenterLatitude() : NAN));
            selectionUpdated = true;
        }
        else
        {
            *newTile = *droppedTile = OMAF_NULL;
            selectionUpdated = false;
        }
    }
    else
    {
        selectionUpdated = false;
    }
    mSelectionUpdated = false;
    mDownloadedTileSet = mSelectedTileSet;
    if (!mDownloadedTileSet)
    {
        OMAF_LOG_D("EMPTY");
    }

    return mDownloadedTileSet;
}

bool_t VASTileSetPicker::findBestTileSet(const VASTilesLayer& allTiles, VASTileSelection& selectedTiles)
{
    VASTileSetContainer* newTileSet = OMAF_NULL;
    VASTileSetContainer* droppedTileSet = OMAF_NULL;

    VASTileSelection selectedRight;
    if (pickTilesFullSearch(allTiles, StereoRole::RIGHT, selectedRight))
    {
        VAS_LOG_TILES_D("Selected right channel tiles:", selectedTiles);
        addTilesInSortedOrder(selectedTiles, selectedRight);
    }
    VASTileSelection selectedLeft;
    if (pickTilesFullSearch(allTiles, StereoRole::LEFT, selectedLeft))
    {
        VAS_LOG_TILES_D("Selected left channel tiles:", selectedLeft);

        addTilesInSortedOrder(selectedTiles, selectedLeft);
    }
    removeExtraTiles(selectedTiles, 1, 0);
    droppedTileSet = mSelectedTileSet;
    if (selectedTiles.isEmpty())
    {
        OMAF_LOG_W("No tiles for lon %f, nr of tiles %zd", mRenderedViewport.getCenterLongitude(),
                   allTiles.getNrAdaptationSets());
    }
    else
    {
        newTileSet = (VASTileSetContainer*) selectedTiles.at(0);
    }

    if (droppedTileSet || newTileSet)
    {
        if (droppedTileSet)
        {
            droppedTileSet->setSelected(false);
        }
        if (newTileSet)
        {
            newTileSet->setSelected(true);
        }
        return true;
    }
    return false;
}

bool_t VASTileSetPicker::findExactTiles(VASTileSetContainer* aSelectedTileSet)
{
    // currently supported. This function operates on foreground center tiles only.
    for (DashAdaptationSetSubPicture* set : aSelectedTileSet->getFgSets())
    {
        float64_t area = mRenderedViewport.intersect(*set->getCoveredViewport());
        if (area > 0)
        {
            OMAF_LOG_V("findExactTiles set %d as active, area %f", set->getId(), area);
            aSelectedTileSet->setAsFgActive(set);
        }
        else
        {
            OMAF_LOG_V("findExactTiles set %d as margin, area %f", set->getId(), area);
            aSelectedTileSet->setAsFgMargin(set);
        }
    }
    return true;
}

size_t VASTileSetPicker::getNrVisibleTiles(VASTilesLayer& aTiles,
                                           float64_t aViewportWidth,
                                           float64_t aViewportHeight,
                                           VASLongitudeDirection::Enum aDirection)
{
    return ((DashAdaptationSetExtractor*) aTiles.getAdaptationSetAt(0))->getNrForegroundTiles();
}

OMAF_NS_END
