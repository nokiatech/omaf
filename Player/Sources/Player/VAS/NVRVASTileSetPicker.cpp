
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
#include "VAS/NVRVASTileSetPicker.h"
#include "VAS/NVRVASLog.h"

OMAF_LOG_ZONE(VASTileSetPicker)

OMAF_NS_BEGIN


    VASTileSetPicker::VASTileSetPicker()
        : VASTilePicker()
    {
        mTileCountRestricted = true;
    }

    VASTileSetPicker::~VASTileSetPicker()
    {
    }

    // called from renderer thread
    void_t VASTileSetPicker::setupTileRendering(VASTilesLayer& allTiles, float64_t aWidth, float64_t aHeight, uint32_t aBaseLayerDecoderPixelsInSec)
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
            mSelectedTiles = selectedTiles;
            if (mSelectedTiles.isEmpty())
            {
                OMAF_LOG_D("EMPTY");
            }
            mSelectionUpdated = true;
            OMAF_LOG_D("Tiles for (%f, %f) updated, now %zd", mRenderedViewport.getCenterLongitude(), mRenderedViewport.getCenterLatitude(), mSelectedTiles.getSize());
        }
        return mSelectedTiles;
    }

    // called from provider thread
    VASTileSelection& VASTileSetPicker::getLatestTiles(bool_t& selectionUpdated, VASTileSelection& droppedTiles, VASTileSelection& newTiles)
    {
        Spinlock::ScopeLock lock(mSpinlock);
        if (mSelectionUpdated)
        {
            droppedTiles.add(mDownloadedTiles.difference(mSelectedTiles));
            if (!droppedTiles.isEmpty())
            {
                OMAF_LOG_D("dropped a tile");
            }
            newTiles.add(mSelectedTiles.difference(mDownloadedTiles));
            if (!newTiles.isEmpty())
            {
                OMAF_LOG_D("added a tile");
            }
            if (mDownloadedTiles.getSize() > 0 && newTiles.getSize() > 0)
            {
                OMAF_LOG_D("Tiles updated; old tile lon %f new tile lon %f", mDownloadedTiles.front()->getCoveredViewport().getCenterLongitude(), newTiles.front()->getCoveredViewport().getCenterLongitude());
            }

            selectionUpdated = true;
        }
        else
        {
            selectionUpdated = false;
        }
        mSelectionUpdated = false;
        mDownloadedTiles = mSelectedTiles;
        if (mDownloadedTiles.isEmpty())
        {
            OMAF_LOG_D("EMPTY");
        }

        return mDownloadedTiles;
    }

    bool_t VASTileSetPicker::findBestTileSet(const VASTilesLayer& allTiles, VASTileSelection& selectedTiles)
    {
        VASTileSelection newTiles;
        VASTileSelection droppedTiles;

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
        droppedTiles.add(mSelectedTiles.difference(selectedTiles));
        newTiles.add(selectedTiles.difference(mSelectedTiles));

        if (!droppedTiles.isEmpty() || !newTiles.isEmpty())
        {
            for (VASTileSelection::Iterator it = droppedTiles.begin(); it != droppedTiles.end(); ++it)
            {
                (*it)->setSelected(false);
            }
            for (VASTileSelection::Iterator it = newTiles.begin(); it != newTiles.end(); ++it)
            {
                (*it)->setSelected(true);
            }
            return true;
        }
        return false;


    }
OMAF_NS_END
