
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "DashProvider/NVRDashAdaptationSet.h"
#include "VAS/NVRVASTileContainer.h"
#include "Foundation/NVRSpinlock.h"

OMAF_NS_BEGIN
    
    typedef FixedArray<VASTileContainer*, 50> VASTileSelection;    // selected tiles
    namespace MoveType
    {
        enum Enum
        {
            UNDEFINED = -1,
            STILL,
            SLOW,
            FAST,
            COUNT
        };
    }

    class VASTilePicker
    {
    public:
        VASTilePicker();
        virtual ~VASTilePicker();

        virtual void_t reset();

        virtual bool_t setRenderedViewPort(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height);
        virtual void_t setupTileRendering(VASTilesLayer& allTiles, float64_t width, float64_t height, uint32_t aBaseLayerDecoderPixelsInSec);
        virtual VASTileSelection& pickTiles(VASTilesLayer& allTiles, uint32_t aBaseLayerDecoderPixelsInSec);

        virtual VASTileSelection& getLatestTiles(bool_t& selectionUpdated, VASTileSelection& dropped, VASTileSelection& additional);
        virtual VASTileSelection& getLatestTiles();
        virtual VASTileSelection& getCurrentNonSelected(const VASTilesLayer& tiles);
        virtual VASTileSelection& getAll(const VASTilesLayer& tiles);

        virtual void_t setAllSelected(const VASTilesLayer& aTiles);

        virtual bool_t reduceMaxNrTiles(size_t count);

        // can be used e.g. when estimating total bandwidth for tiles layer; if not yet estimated (in renderer thread), returns 0
        virtual size_t getNrUsedTiles(bool_t aUseOnlyMono = false);
        // can be used to pre-allocate the decoders; perf issues not considered, but only the max number of decoders from the tile structure.
        virtual size_t getNrVisibleTiles(VASTilesLayer& aTiles, float64_t aViewportWidth, float64_t aViewportHeight);

        virtual void_t forceToMono(bool_t aMono);

    protected:
        void_t findStartTiles(const VASTilesLayer& tiles, VASTileSelection& selectedTiles);
        bool_t pickTilesFullSearch(const VASTilesLayer& tiles, StereoRole::Enum channel, VASTileSelection& selected);
        void_t addTilesInSortedOrder(VASTileSelection& allCandidates, const VASTileSelection& aCandidates);
        void_t removeExtraTiles(VASTileSelection& allCandidates, size_t maxTiles, uint32_t maxResolution);

    protected:
        VASRenderedViewport mRenderedViewport;
        VASTileSelection mSelectedTiles;    // tiles selected in rendering thread; L+R
        VASTileSelection mDownloadedTiles;    // tiles used in provider thread; L+R

        Spinlock mSpinlock;
        bool_t mSelectionUpdated;

        bool_t mTileCountRestricted;
    private:


        void_t findAllTiles(const VASTilesLayer& tiles);
        bool_t findSwitchingTiles(const VASTilesLayer& adaptationSets, VASTileSelection& selectedTiles, uint32_t aBaseLayerDecoderPixelsInSec);
        bool_t pickTilesDeltaSearch(const VASTilesLayer& tiles, StereoRole::Enum channel, VASTileSelection& selected);

        void_t doFullSearchRow(const VASTiles& tilesInRow, VASTileSelection& candidates);
        void_t doDeltaSearchRow(const VASTiles& tilesInRow, size_t first, VASTileSelection& candidates);


        bool_t estimateSupportedTileCount(VASTilesLayer& aTiles, float64_t aViewportWidth, float64_t aViewportHeight, uint32_t aBaseLayerDecoderPixelsInSec);
        size_t estimateTileCount(VASTilesLayer& aTiles, float64_t aViewportWidth, float64_t aViewportHeight, float64_t& aMaxTileCountRowCenter);
        size_t countNeededHorizontalTiles(const VASTiles& tilesInRow, float64_t aViewportWidth) const;
        void_t checkTileResolutions(VASTilesLayer& aTiles, float64_t aViewportTop, float64_t aViewportBottom, float64_t aViewportWidth, uint32_t aMaxPixelsInSec) const;

    private:
        struct MotionVector
        {
            MotionVector()
                : deltaLongitude(0.f)
                , deltaLatitude(0.f)
            {}

            float64_t deltaLongitude;
            float64_t deltaLatitude;

            void_t clear();
            inline bool_t isStill();
            inline bool_t isMoving();
            inline bool_t isFast();
        };
        MotionVector mTotalVectorSlow;
        MoveType::Enum mLastMove;

        // lists of references to VASTilesLayer, do not own anything
        VASTileSelection mSelectionAll;        // all
        VASTileSelection mCurrentNonSelected; // all except mCurrentSelection

        bool_t mUpdateNonSelected;
        bool_t mPendingTiles;
        bool_t mTriggerTileSelection;

        size_t mMaxTileCount;
        size_t mNeededTileCount;
        size_t mNeededTileCountMono;

        bool_t mForcedToMono;

        VASTileType::Enum mTileType;
    };
OMAF_NS_END
