
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

#include "DashProvider/NVRDashAdaptationSetSubPicture.h"

OMAF_NS_BEGIN

class VASTileRow;
class VASTileContainer;
class DashAdaptationSetExtractorMR;

typedef FixedArray<VASTileContainer*, 60> VASTiles;  // tiles in row
typedef FixedArray<VASTileRow*, 20> VASTileRows;     // tile rows, containing tiles

/*
 * Represents one tile. In DASH it is wrapper for adaptation set, but in potential other VAS systems may be something
 * else
 */
class VASTileContainer
{
public:
    VASTileContainer(DashAdaptationSetSubPicture* aSet, const VASTileViewport& aTileViewport);
    virtual ~VASTileContainer();

    const VASTileViewport& getCoveredViewport() const;
    DashAdaptationSetSubPicture* getAdaptationSet() const;
    MP4VideoStreams getVideoStreams() const;

    void_t setSelected(bool_t selected);
    bool_t getSelected() const;

    void_t setIntersectionArea(float64_t area);
    float64_t getIntersectionArea() const;

private:
    DashAdaptationSetSubPicture* mAdaptationSet;
    const VASTileViewport& mTileViewport;
    bool_t mIsSelected;
    float64_t mIntersectionArea;
};

class VASTileSetContainer : public VASTileContainer
{
public:
    VASTileSetContainer(DashAdaptationSetSubPicture* aSet,
                        const VASTileViewport& aTileViewport,
                        TileAdaptationSets aForegroundTiles);
    virtual ~VASTileSetContainer();

    const TileAdaptationSets& getFgSets() const;
    const TileAdaptationSets& getFgActiveSets() const;
    const TileAdaptationSets& getFgMarginSets() const;

    void_t startNew();
    void_t setAsFgActive(DashAdaptationSetSubPicture* aSet);
    void_t setAsFgMargin(DashAdaptationSetSubPicture* aSet);

private:
    TileAdaptationSets mFgSets;
    TileAdaptationSets mFgActiveSets;
    TileAdaptationSets mFgMarginSets;
};

/*
 * One horizontal row of tiles.
 * Tiles can be overlapping, but the assumption is that the overlap of left & right of the current tile together
 * completely cover the current tile always,
 * i.e. there are two parallel set of tiles that cover the whole row, but the sets are shifted
 */
class VASTileRow
{
public:
    VASTileRow(float64_t latitudeCenter, float64_t latitudeTop, float64_t latitudeBottom);
    virtual ~VASTileRow();

    const VASTiles& getTiles() const;
    const VASTiles& getTilesShifted() const;

    void_t clearSelections();

protected:
    friend class VASTilesLayer;

    // pass ownership
    Error::Enum add(VASTileContainer* aTile);
    void_t sortAddedSets();
    float64_t getLatitude();
    float64_t getLatitudeTop();
    float64_t getLatitudeBottom();

private:
    float64_t mLatitudeCenter;
    float64_t mLatitudeTop;
    float64_t mLatitudeBottom;
    VASTiles mTiles;
    VASTiles mTilesShifted;
};

/*
 * Container for all available tiles
 */
class VASTilesLayer
{
public:
    VASTilesLayer();
    virtual ~VASTilesLayer();

    void_t clear();
    bool_t isEmpty() const;
    void_t clearSelections();

    // pass ownership
    void_t add(VASTileContainer* aTile);
    void_t allSetsAdded(bool_t aCheckOverlaps);

    bool_t hasSeparateStereoTiles() const;
    size_t
    getRows(VASTileRows& aOutputRows, StereoRole::Enum channel, float64_t latitudeTop, float64_t latitudeBottom) const;

    // this should be only used when all tiles/adaptation sets need to be iterated
    DashAdaptationSetSubPicture* getAdaptationSetAt(size_t index) const;
    size_t getNrAdaptationSets() const;

    const TileAdaptationSets& getAdaptationSets();

    VASTileType::Enum getTileType() const;

private:
    void_t doAddEquirect(VASTileContainer* aTile, VASTileRows& aRows);
    size_t getRowsInternal(VASTileRows& aOutputRows,
                           const VASTileRows& aSourceRows,
                           float64_t aLatitudeTop,
                           float64_t aLatitudeBottom) const;

private:
    VASTileRows mRowsL;                  // left or packed L+R
    VASTileRows mRowsR;                  // right
    TileAdaptationSets mAdaptationSets;  // list of all added adaptation sets; ref only
    VASTileType::Enum mTileType;
};

OMAF_NS_END
