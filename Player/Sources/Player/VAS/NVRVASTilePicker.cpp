
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
#include "VAS/NVRVASTilePicker.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Math/OMAFMathFunctions.h"
#include "VAS/NVRVASLog.h"

OMAF_LOG_ZONE(VASTilePicker)

OMAF_NS_BEGIN

// The intersection area is calculated in degree space (ie it is independent on resolution in pixels).
// For simplicity, top and bottom widths are not averaged but summed in VASRenderedViewport::doCheckIntersection, so the
// width is 2x real width.
// So if a 90x90 degree tile & viewport fully intersect, the result is 16200, and 300 is ~2% of that
static const float64_t INTERSECTION_AREA_THR = 300.f;
// Thresholds for triggering tile reselection in degrees
static const float64_t MOTION_THR_STILL_DEG = 0.5f;
static const float64_t MOTION_THR_FAST_DEG = 10.f;

static const float64_t VIEWPORT_EXTENSION =
    1.2f;  // multiplier for the high quality area around the actual viewport (width,height)

void_t VASTilePicker::MotionVector::clear()
{
    deltaLatitude = 0.f;
    deltaLongitude = 0.f;
}

// ideally, these three criteria would compare the total motion of the two components jointly, i.e. sqrt(lat^2 + lon^2)
// but for computational simplicity we threshold them individually
bool_t VASTilePicker::MotionVector::isStill()
{
    if (abs(deltaLatitude) < MOTION_THR_STILL_DEG && abs(deltaLongitude) < MOTION_THR_STILL_DEG)
    {
        return true;
    }
    return false;
}
bool_t VASTilePicker::MotionVector::isMoving()
{
    return !isStill();
}
bool_t VASTilePicker::MotionVector::isFast()
{
    if (abs(deltaLatitude) > MOTION_THR_FAST_DEG || abs(deltaLongitude) > MOTION_THR_FAST_DEG)
    {
        return true;
    }
    return false;
}

VASTilePicker::VASTilePicker()
    : mRenderedViewport()
    , mLastMove(MoveType::STILL)
    , mSpinlock()
    , mDownloadedViewportTiles()
    , mSelectedTiles()
    , mSelectionUpdated(false)
    , mUpdateNonSelected(false)
    , mPendingTiles(false)
    , mTriggerTileSelection(false)
    , mMaxTileCount(DeviceInfo::maxLayeredVASTileCount())
    , mNeededTileCount(0)
    , mNeededTileCountMono(0)
    , mForcedToMono(false)
    , mTileCountRestricted(false)
    , mTileType(VASTileType::EQUIRECT_ENHANCEMENT)
{
    mRenderedViewport.setPosition(
        0.f, 0.f, 90.f, 90.f,
        mTileType);  // we need some initial values, otherwise the first match would be done for the full sphere
}

VASTilePicker::~VASTilePicker()
{
    reset();
}

void_t VASTilePicker::reset()
{
}

// called from renderer thread
bool_t VASTilePicker::setRenderedViewPort(float64_t longitude,
                                          float64_t latitude,
                                          float64_t roll,
                                          float64_t width,
                                          float64_t height)
{
    // OMAF_LOG_V("setRenderedViewPort (%f,%f), (w x h) = (%f x %f)", longitude, latitude, width, height);
    width *= VIEWPORT_EXTENSION;
    height *= VIEWPORT_EXTENSION;

    bool_t triggerTileSelection = true;
    if (!mSelectedTiles.isEmpty())
    {
        MotionVector mv;
        mv.deltaLongitude = longitude - mRenderedViewport.getCenterLongitude();
        mv.deltaLatitude = latitude - mRenderedViewport.getCenterLatitude();

        if (mv.isMoving())
        {
            if (mv.isFast())
            {
                // too high motion, wait
                triggerTileSelection = false;
                mLastMove = MoveType::FAST;
                mTotalVectorSlow.clear();
                // OMAF_LOG_D("FAST");
            }
            else
            {
                triggerTileSelection = true;
                mLastMove = MoveType::SLOW;
                // OMAF_LOG_D("SLOW");
            }
            // OMAF_LOG_D("Viewport moved: %f,%f", mv->deltaLongitude, mv->deltaLatitude);
        }
        else
        {
            // not moving now
            if (mTotalVectorSlow.isMoving() && mLastMove == MoveType::STILL)
            {
                // too small motion now, but small accumulated motion is getting large
                triggerTileSelection = true;
                mLastMove = MoveType::SLOW;
                // OMAF_LOG_D("SLOW TOTAL");
            }
            else if (mLastMove == MoveType::FAST)
            {
                // if previous vectors were moving, also the very last one, freeze this position and trigger calculation
                triggerTileSelection = true;
                mLastMove = MoveType::STILL;
                // OMAF_LOG_D("STILL AFTER FAST");
            }
            else
            {
                // too small motion, ignore
                triggerTileSelection = false;
                mLastMove = MoveType::STILL;
                // but save the motion
                mTotalVectorSlow.deltaLongitude += mv.deltaLongitude;
                mTotalVectorSlow.deltaLatitude += mv.deltaLatitude;
                // OMAF_LOG_D("STILL");
            }
        }
    }
    mRenderedViewport.setPosition(longitude, latitude, width, height, mTileType);

    // then do the tile selection
    if (triggerTileSelection || mPendingTiles || mTriggerTileSelection)
    {
        VAS_LOG_D("Head moved to (long, lat):\t(%f, %f)", longitude, latitude);
        // OMAF_LOG_D("Head moved to (long, lat):\t(%f, %f)", longitude, latitude);
        mTotalVectorSlow.clear();

        return true;
    }
    return false;
}

// called from renderer thread
void_t VASTilePicker::setupTileRendering(VASTilesLayer& allTiles,
                                         float64_t aWidth,
                                         float64_t aHeight,
                                         uint32_t aBaseLayerDecoderPixelsInSec,
                                         VASLongitudeDirection::Enum aDirection)
{
    aWidth *= VIEWPORT_EXTENSION;
    aHeight *= VIEWPORT_EXTENSION;

    if (aBaseLayerDecoderPixelsInSec > 0)
    {
        if (mNeededTileCount == 0)
        {
            Spinlock::ScopeLock lock(mSpinlock);
            estimateSupportedTileCount(allTiles, aWidth, aHeight, aBaseLayerDecoderPixelsInSec, aDirection);
        }
        mTileCountRestricted = true;
    }
    else
    {
        mTileCountRestricted = false;
    }
    mTileType = allTiles.getTileType();
}

// called from renderer thread
VASTileSelection& VASTilePicker::pickTiles(VASTilesLayer& allTiles, uint32_t aBaseLayerDecoderPixelsInSec)
{
    VASTileSelection selectedTiles;
    if (findSwitchingTiles(allTiles, selectedTiles, aBaseLayerDecoderPixelsInSec))
    {
        Spinlock::ScopeLock lock(mSpinlock);
        // store the selection to wait for provider to read it
        mSelectedTiles = selectedTiles;
        mSelectionUpdated = true;
        mUpdateNonSelected = true;
        OMAF_LOG_D("Tiles for (%f, %f) updated, now %zd", mRenderedViewport.getCenterLongitude(),
                   mRenderedViewport.getCenterLatitude(), mSelectedTiles.getSize());
    }
    mTriggerTileSelection = false;
    return mSelectedTiles;
}

// called from provider thread
VASTileSelection& VASTilePicker::getLatestTiles()
{
    return mDownloadedViewportTiles;
}

// called from provider thread
VASTileSelection& VASTilePicker::getLatestTiles(bool_t& selectionUpdated,
                                                VASTileSelection& aToViewport,
                                                VASTileSelection& aToBackground)
{
    Spinlock::ScopeLock lock(mSpinlock);
    if (mSelectionUpdated)
    {
        aToViewport.add(mSelectedTiles.difference(mDownloadedViewportTiles));
        aToBackground.add(mDownloadedViewportTiles.difference(mSelectedTiles));
        OMAF_LOG_D("Tiles updated; old %zd new viewport %zd, added %zd, dropped %zd",
                   mDownloadedViewportTiles.getSize(), mSelectedTiles.getSize(), aToViewport.getSize(),
                   aToBackground.getSize());
        VAS_LOG_TILES_D("New tiles:", aToViewport);
        VAS_LOG_TILES_D("Dropped tiles:", aToBackground);

        selectionUpdated = true;
    }
    else
    {
        selectionUpdated = false;
    }
    mSelectionUpdated = false;
    mDownloadedViewportTiles = mSelectedTiles;

    return mDownloadedViewportTiles;
}

VASTileSelection& VASTilePicker::getLatestMargins(size_t aNumberOfTiles, VASTileSelection& aToBackground)
{
    Spinlock::ScopeLock lock(mSpinlock);
    // aToBackground is the set of tiles that did belong to the previous viewport but not to the new viewport

    // take old margin tiles set, except those that are in current viewport
    VASTileSelection oldMargin = mDownloadedMarginTiles.difference(mDownloadedViewportTiles);
    if (findMarginTiles(aNumberOfTiles))
    {
        // mDownloadedMarginTiles were updated
        // if there are margin tiles, they need to be excluded from the set that goes to the background
        // i.e. background tiles are non-viewport tiles, except those that are margin tiles
        aToBackground = aToBackground.difference(mDownloadedMarginTiles);
        // add old margin tiles, except those that are also new margin tiles
        aToBackground.add(oldMargin.difference(mDownloadedMarginTiles));
    }
    else
    {
        // if there are no margin tiles, aToBackground needs to have also previous margin tiles that are not in the
        // viewport
        aToBackground.add(mDownloadedMarginTiles.difference(mDownloadedViewportTiles));
        mDownloadedMarginTiles.clear();
    }
    return mDownloadedMarginTiles;
}

void_t VASTilePicker::setAllSelected(const VASTilesLayer& aTiles)
{
    if (mSelectionAll.isEmpty() && !aTiles.isEmpty())
    {
        findAllTiles(aTiles);
    }
    mSelectedTiles = mSelectionAll;
    mDownloadedViewportTiles = mSelectionAll;
    mTriggerTileSelection = true;
}

bool_t VASTilePicker::reduceMaxNrTiles(size_t count)
{
    if (mMaxTileCount > 4 && mMaxTileCount - count > 4)  // never go below 4 tiles (2 per channel in stereo)
    {
        mMaxTileCount = max((size_t) 4, mMaxTileCount - count);
        OMAF_LOG_W("Reduced the number of tiles to %zd", mMaxTileCount);
        return true;
    }
    return false;
}

size_t VASTilePicker::getNrUsedTiles(bool_t aUseOnlyMono)
{
    Spinlock::ScopeLock lock(mSpinlock);
    if (mNeededTileCount > 0)
    {
        if (aUseOnlyMono)
        {
            return min(mMaxTileCount, mNeededTileCountMono);
        }
        else
        {
            return min(mMaxTileCount, mNeededTileCount);
        }
    }
    else
    {
        return mMaxTileCount;
    }
}

void_t VASTilePicker::findAllTiles(const VASTilesLayer& tiles)
{
    float64_t top = 90.0;
    float64_t bottom = -90.0;
    VASTileRows rowsL;

    if (tiles.getRows(rowsL, StereoRole::LEFT, top, bottom))
    {
        for (VASTileRows::ConstIterator it = rowsL.begin(); it != rowsL.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();

            for (size_t i = 0; i < tilesInRow.getSize(); i++)
            {
                mSelectionAll.add(tilesInRow[i]);
            }
        }
    }

    VASTileRows rowsR;
    if (tiles.getRows(rowsR, StereoRole::RIGHT, top, bottom))
    {
        for (VASTileRows::ConstIterator it = rowsR.begin(); it != rowsR.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();

            for (size_t i = 0; i < tilesInRow.getSize(); i++)
            {
                mSelectionAll.add(tilesInRow[i]);
            }
        }
    }
}

// When starting to download tiles (no previous tiles used), the best selection minimizes the number of tiles
void_t VASTilePicker::findStartTiles(const VASTilesLayer& allTiles, VASTileSelection& selectedTiles)
{
    for (VASTileSelection::ConstIterator it = mSelectedTiles.begin(); it != mSelectedTiles.end(); ++it)
    {
        (*it)->setSelected(false);
    }
    mSelectedTiles.clear();

    // OMAF_LOG_D("findStartTiles for %f,%f", mPredictedViewport.getCenterLongitude(),
    // mPredictedViewport.getCenterLatitude());
    // mPredictedViewport.getCenterLatitude());
    if (pickTilesFullSearch(allTiles, StereoRole::RIGHT, selectedTiles))
    {
        VAS_LOG_TILES_D("Selected right channel tiles:", selectedTiles);
    }
    if (!mForcedToMono)
    {
        VASTileSelection selectedLeft;
        if (pickTilesFullSearch(allTiles, StereoRole::LEFT, selectedLeft))
        {
            VAS_LOG_TILES_D("Selected left channel tiles:", selectedLeft);
            selectedTiles.add(selectedLeft);
        }
    }
}

// When updating the tile selection after a head move, the best selection minimizes the number of new tiles. This should
// be called to see immediate changes while current segments are still being played
bool_t VASTilePicker::findSwitchingTiles(const VASTilesLayer& allTiles,
                                         VASTileSelection& selectedTiles,
                                         uint32_t aBaseLayerDecoderPixelsInSec)
{
    VASTileSelection newTiles;
    VASTileSelection droppedTiles;
    uint32_t maxPixelsInSec = DeviceInfo::maxDecodedPixelCountPerSecond() - aBaseLayerDecoderPixelsInSec;
    uint32_t maxNewTileCount = 0;
    mSelectedMarginTiles.clear();
    if (mSelectedTiles.isEmpty())
    {
        // initial search need to go through full search
        // OMAF_LOG_D("findSwitchingTiles no current selection => do full search");
        // all found tiles are new
        findStartTiles(allTiles, newTiles);
        droppedTiles.clear();
        addTilesInSortedOrder(selectedTiles, newTiles);
        removeExtraTiles(selectedTiles, mMaxTileCount, maxPixelsInSec);
        if (allTiles.hasSeparateStereoTiles())
        {
            maxNewTileCount = 2;
        }
        else
        {
            maxNewTileCount = 1;
        }
    }
    else
    {
        // OMAF_LOG_D("FindSwitchingTiles for %f,%f", mRenderedViewport.getCenterLongitude(),
        // mRenderedViewport.getCenterLatitude());
        VASTileSelection selectedRight;
        if (pickTilesDeltaSearch(allTiles, StereoRole::RIGHT, selectedRight))
        {
            // else mono or frame-packed tiles
            VAS_LOG_TILES_D("Selected right channel tiles:", selectedRight);
            addTilesInSortedOrder(selectedTiles, selectedRight);
            maxNewTileCount++;
        }
        if (!mForcedToMono)
        {
            VASTileSelection selectedLeft;
            if (pickTilesDeltaSearch(allTiles, StereoRole::LEFT, selectedLeft))
            {
                VAS_LOG_TILES_D("Selected left channel tiles:", selectedLeft);
                addTilesInSortedOrder(selectedTiles, selectedLeft);
                maxNewTileCount++;
            }
        }

        removeExtraTiles(selectedTiles, mMaxTileCount, maxPixelsInSec);
        droppedTiles.add(mSelectedTiles.difference(selectedTiles));
        newTiles.add(selectedTiles.difference(mSelectedTiles));
    }

    // limit the number of newTiles so that the prioritized tiles get the bandwidth, and assume the other new tiles will
    // be noticed in the next round
    if (allTiles.getTileType() == VASTileType::EQUIRECT_ENHANCEMENT)
    {
        mPendingTiles = false;
        while (newTiles.getSize() > maxNewTileCount)
        {
            VASTileContainer* tile = newTiles.at(newTiles.getSize() - 1);
            OMAF_LOG_D("Too many new tiles for one round; removing tile at (%f, %f)",
                       tile->getCoveredViewport().getCenterLongitude(), tile->getCoveredViewport().getCenterLatitude());
            selectedTiles.remove(tile);
            newTiles.removeAt(newTiles.getSize() - 1);
            // need to ensure the tile picking is done in the next round, regardless of the head motion
            mPendingTiles = true;
        }
    }
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

bool_t VASTilePicker::findMarginTiles(size_t aNrTiles)
{
    if (aNrTiles == 0 || mSelectedMarginTiles.isEmpty())
    {
        return false;
    }
    mDownloadedMarginTiles.clear();
    for (size_t i = 0; i < aNrTiles && i < mSelectedMarginTiles.getSize(); i++)
    {
        OMAF_LOG_V("Margin: %d area %f", mSelectedMarginTiles.at(i)->getAdaptationSet()->getId(),
                   mSelectedMarginTiles.at(i)->getIntersectionArea());
        mDownloadedMarginTiles.add(mSelectedMarginTiles.at(i));
    }

    return true;
}

// Does full search by checking intersection of all tiles and the current mRenderedViewport
bool_t VASTilePicker::pickTilesFullSearch(const VASTilesLayer& allTiles,
                                          StereoRole::Enum channel,
                                          VASTileSelection& selected)
{
    float64_t top, bottom;
    mRenderedViewport.getTopBottom(top, bottom);

    VASTileRows rows;
    if (allTiles.getRows(rows, channel, top, bottom))
    {
        for (VASTileRows::ConstIterator it = rows.begin(); it != rows.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();
            VASTileSelection rowCandidates;

            doFullSearchRow(tilesInRow, rowCandidates);

            // add (copy) the whole array
            selected.add(rowCandidates);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool_t VASTilePicker::pickTilesDeltaSearch(const VASTilesLayer& allTiles,
                                           StereoRole::Enum channel,
                                           VASTileSelection& selected)
{
    // get rows based on mRenderedViewport
    float64_t top, bottom;
    mRenderedViewport.getTopBottom(top, bottom);

    // We already now eliminate useless overlap partially: if a row covers the viewport completely, we don't consider
    // other rows even if they would partially cover the viewport too (due to overlap)
    VASTileRows rows;
    if (allTiles.getRows(rows, channel, top, bottom))
    {
        for (VASTileRows::ConstIterator it = rows.begin(); it != rows.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();
            VASTileSelection rowCandidates;

            // check if any tile on the row has been selected
            size_t first = OMAF_UINT32_MAX;  // there is no max for size_t, but this value is out of normal bounds
            for (size_t i = 0; i < tilesInRow.getSize(); i++)
            {
                if (tilesInRow[i]->getSelected())
                {
                    first = i;
                    break;
                }
            }

            if (first < OMAF_UINT32_MAX)
            {
                // there is at least 1 tile from this row, do delta search: current tiles and +- enough many tiles
                // we could further optimize this with the mMotionVector
                doDeltaSearchRow(tilesInRow, first, rowCandidates);
                if (!rowCandidates.isEmpty())
                {
                    selected.add(rowCandidates);
                }
            }
            // if no tile, do full search like above for the row
            else
            {
                // VAS_LOG_D("Do full search");
                doFullSearchRow(tilesInRow, rowCandidates);
                // pick the row with less tiles

                // add (copy) the whole array
                selected.add(rowCandidates);
            }
        }
        return true;
    }
    else
    {
        return false;
    }
}

void_t VASTilePicker::doFullSearchRow(const VASTiles& tilesInRow, VASTileSelection& candidates)
{
    FixedArray<size_t, 60> noMatch;

    for (size_t i = 0; i < tilesInRow.getSize(); i++)
    {
        float64_t area = mRenderedViewport.intersect(tilesInRow[i]->getCoveredViewport());
        if (area > INTERSECTION_AREA_THR)
        {
            // OMAF_LOG_D("Candidate tile %f", tilesInRow[i]->getCoveredViewport().getCenterLongitude());
            // tiles will be in left-to-right order
            tilesInRow[i]->setIntersectionArea(area);
            candidates.add(tilesInRow[i]);
        }
        else if (area > 0)
        {
            addMarginTile(tilesInRow[i], area);
        }
        else
        {
            tilesInRow[i]->setIntersectionArea(0);
            noMatch.add(i);
        }
    }
    for (size_t j : noMatch)
    {
        checkMarginTileNeighbor(tilesInRow, j);
    }
}

void_t VASTilePicker::doDeltaSearchRow(const VASTiles& tilesInRow, size_t firstOld, VASTileSelection& candidates)
{
    int32_t first = (int32_t) firstOld;
    int32_t firstIncluded = -1;
    int32_t lastIncluded = -1;
    FixedArray<size_t, 60> noMatch;

    for (int32_t i = first; i >= 0; i--)
    {
        // check how many tiles to left we need to go
        float64_t area = mRenderedViewport.intersect(tilesInRow[i]->getCoveredViewport());
        if (area > INTERSECTION_AREA_THR)
        {
            if (lastIncluded < 0)
            {
                lastIncluded = i;
            }
            firstIncluded = i;
            tilesInRow[i]->setIntersectionArea(area);
            candidates.add(tilesInRow[i]);
        }
        else if (area > 0)
        {
            addMarginTile(tilesInRow[i], area);
            break;
        }
        else
        {
            tilesInRow[i]->setIntersectionArea(0);
            noMatch.add(i);
            break;
        }
    }
    for (int32_t i = first + 1; i < tilesInRow.getSize(); i++)
    {
        // check how many tiles to right we need to go
        float64_t area = mRenderedViewport.intersect(tilesInRow[i]->getCoveredViewport());
        if (area > INTERSECTION_AREA_THR)
        {
            if (firstIncluded < 0)
            {
                firstIncluded = i;
            }
            lastIncluded = i;
            tilesInRow[i]->setIntersectionArea(area);
            candidates.add(tilesInRow[i]);
        }
        else if (area > 0)
        {
            addMarginTile(tilesInRow[i], area);
            break;
        }
        else
        {
            tilesInRow[i]->setIntersectionArea(0);
            noMatch.add(i);
            break;
        }
    }
    if (firstIncluded >= 0 && lastIncluded >= 0)
    {
        // plus check if the intersection continues after wrap-around (either a wrap-around tile or intersection is in
        // both ends of the equirect)
        if (lastIncluded == tilesInRow.getSize() - 1 && firstIncluded > 0)
        {
            // wrap around from right to left
            for (int32_t i = 0; i < firstIncluded; i++)
            {
                float64_t area = mRenderedViewport.intersect(tilesInRow[i]->getCoveredViewport());
                if (area > INTERSECTION_AREA_THR)
                {
                    //      OMAF_LOG_D("doDeltaSearchRow matched after wrap-around %d", i);
                    // OMAF_LOG_D("Delta: Candidate tile %f", tilesInRow[i]->getCoveredViewport().getCenterLongitude());
                    tilesInRow[i]->setIntersectionArea(area);
                    candidates.add(tilesInRow[i]);
                }
                else if (area > 0)
                {
                    addMarginTile(tilesInRow[i], area);
                    break;
                }
                else
                {
                    tilesInRow[i]->setIntersectionArea(0);
                    noMatch.add(i);
                    break;
                }
            }
        }
        else if (firstIncluded == 0 && lastIncluded < tilesInRow.getSize() - 1)
        {
            // wrap around from left to right
            for (int32_t i = (int32_t)(tilesInRow.getSize() - 1); i > lastIncluded; i--)
            {
                float64_t area = mRenderedViewport.intersect(tilesInRow[i]->getCoveredViewport());
                if (area > INTERSECTION_AREA_THR)
                {
                    // OMAF_LOG_D("doDeltaSearchRow matched after wrap-around %d", i);
                    // OMAF_LOG_D("Delta: Candidate tile %f", tilesInRow[i]->getCoveredViewport().getCenterLongitude());
                    tilesInRow[i]->setIntersectionArea(area);
                    candidates.add(tilesInRow[i]);
                }
                else if (area > 0)
                {
                    addMarginTile(tilesInRow[i], area);
                    break;
                }
                else
                {
                    tilesInRow[i]->setIntersectionArea(0);
                    noMatch.add(i);
                    break;
                }
            }
        }
        for (size_t j : noMatch)
        {
            checkMarginTileNeighbor(tilesInRow, j);
        }
    }
    else
    {
        // try full search; this happens e.g. if the head motion jumps over a tile.
        // Utilizing the motion vector could help too
        doFullSearchRow(tilesInRow, candidates);
    }
}

void_t VASTilePicker::checkMarginTileNeighbor(const VASTiles& tilesInRow, size_t aIndex)
{
    if (aIndex == 0)
    {
        // first tile of the row => previous tile wrap around from the beginning
        if (tilesInRow[1]->getIntersectionArea() > INTERSECTION_AREA_THR ||
            tilesInRow[tilesInRow.getSize() - 1]->getIntersectionArea() > INTERSECTION_AREA_THR)
        {
            addMarginTile(tilesInRow[aIndex], 1.0f);
        }
    }
    else if (aIndex == tilesInRow.getSize() - 1)
    {
        // last tile of the row => next tile wrap around from the end
        if (tilesInRow[0]->getIntersectionArea() > INTERSECTION_AREA_THR ||
            tilesInRow[tilesInRow.getSize() - 2]->getIntersectionArea() > INTERSECTION_AREA_THR)
        {
            addMarginTile(tilesInRow[aIndex], 1.0f);
        }
    }
    else
    {
        // other cases
        if (tilesInRow[aIndex + 1]->getIntersectionArea() > INTERSECTION_AREA_THR ||
            tilesInRow[aIndex - 1]->getIntersectionArea() > INTERSECTION_AREA_THR)
        {
            addMarginTile(tilesInRow[aIndex], 1.0f);
        }
    }
}

void_t VASTilePicker::addMarginTile(VASTileContainer* aTile, float64_t aArea)
{
    if (mSelectedMarginTiles.contains(aTile))
    {
        return;
    }
    aTile->setIntersectionArea(aArea);
    size_t j = 0;
    for (; j < mSelectedMarginTiles.getSize(); j++)
    {
        if (mSelectedMarginTiles[j]->getIntersectionArea() < aArea)
        {
            break;
        }
    }
    mSelectedMarginTiles.add(aTile, j);
}

// adds tiles from rowCandidates to candidates in largest area first order
void_t VASTilePicker::addTilesInSortedOrder(VASTileSelection& allCandidates, const VASTileSelection& aCandidates)
{
    for (size_t i = 0; i < aCandidates.getSize(); i++)
    {
        size_t j = 0;
        for (; j < allCandidates.getSize(); j++)
        {
            if (allCandidates[j]->getIntersectionArea() < aCandidates[i]->getIntersectionArea())
            {
                break;
            }
        }
        allCandidates.add(aCandidates[i], j);
    }
}

void_t VASTilePicker::removeExtraTiles(VASTileSelection& selectedTiles, size_t maxTiles, uint32_t aMaxPixelsInSec)
{
    if (!mTileCountRestricted)
    {
        return;
    }
    if (selectedTiles.getSize() > maxTiles)
    {
        // number of tiles based decision
        size_t count = selectedTiles.getSize() - maxTiles;
        //            OMAF_LOG_V("Removing %zd tiles due to tile count constraints", count);
        selectedTiles.removeAt(maxTiles, count);
    }

    if (aMaxPixelsInSec > 0)
    {
        // decoder resource usage based decision
        uint32_t pixelsInSec = 0;
        for (size_t i = 0; i < selectedTiles.getSize(); i++)
        {
            pixelsInSec += (uint32_t)(selectedTiles[i]->getAdaptationSet()->getCurrentWidth() *
                                      selectedTiles[i]->getAdaptationSet()->getCurrentHeight() *
                                      selectedTiles[i]->getAdaptationSet()->getCurrentFramerate());
            if (pixelsInSec > aMaxPixelsInSec)
            {
                size_t count = selectedTiles.getSize() - i;
                OMAF_LOG_V("Removing %zd tiles due to resolution constraints", count);
                selectedTiles.removeAt(i, count);
                break;
            }
        }
    }
}
size_t VASTilePicker::countNeededHorizontalTiles(const VASTiles& aTilesInRow,
                                                 float64_t aViewportWidth,
                                                 VASLongitudeDirection::Enum aDirection) const
{
    // number of horizontal tiles at given row
    size_t horizontal = 0;
    float64_t totalWidth = 0;
    // some rounding for width comparison, so that e.g. 90.0 tile width is enough for 90.5 viewport
    while (totalWidth - aViewportWidth < -2.f && horizontal < aTilesInRow.getSize())
    {
        float64_t left;
        float64_t right;
        aTilesInRow[horizontal]->getCoveredViewport().getLeftRight(left, right);
        if (aDirection == VASLongitudeDirection::CLOCKWISE)
        {
            totalWidth += right - left;
        }
        else
        {
            totalWidth += left - right;
        }
        horizontal++;
    }
    // and 1 more since viewport can cross the tile in both sides
    if (aTilesInRow.getSize() > 1)  // assuming == 1 means it covers the full width
    {
        horizontal++;
    }

    return horizontal;
}

void_t VASTilePicker::checkTileResolutions(VASTilesLayer& aTiles,
                                           float64_t aViewportTop,
                                           float64_t aViewportBottom,
                                           float64_t aViewportWidth,
                                           uint32_t aMaxPixelsInSec,
                                           VASLongitudeDirection::Enum aDirection) const
{
    VASTileRows rows;
    aTiles.getRows(rows, StereoRole::UNKNOWN, aViewportTop, aViewportBottom);
    uint32_t reprCount = aTiles.getAdaptationSetAt(0)->getNrOfRepresentations();
    int32_t index = reprCount - 1;
    while (index >= 0)
    {
        float64_t tilesReso = 0;
        for (VASTileRows::ConstIterator it = rows.begin(); it != rows.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();
            uint32_t width = 0;
            uint32_t height = 0;
            float64_t fps = 0.f;
            // assuming tiles on a row are equal size
            tilesInRow.at(0)->getAdaptationSet()->getRepresentationParameters(index, width, height, fps);
            tilesReso += width * height * fps * countNeededHorizontalTiles(tilesInRow, aViewportWidth, aDirection);
        }
        if ((uint32_t) tilesReso > aMaxPixelsInSec)
        {
            // disable this ABR level on this device, all tiles
            size_t count = aTiles.getNrAdaptationSets();
            for (size_t i = 0; i < count; i++)
            {
                aTiles.getAdaptationSetAt(i)->setRepresentationNotSupported(index);
            }
        }
        index--;
    }
}

bool_t VASTilePicker::estimateSupportedTileCount(VASTilesLayer& aTiles,
                                                 float64_t aViewportWidth,
                                                 float64_t aViewportHeight,
                                                 uint32_t aBaseLayerDecoderPixelsInSec,
                                                 VASLongitudeDirection::Enum aDirection)
{
    float64_t maxTileCountRowCenter = 0.f;
    mNeededTileCountMono = mNeededTileCount =
        estimateTileCount(aTiles, aViewportWidth, aViewportHeight, maxTileCountRowCenter, aDirection);

    // then measure the area in pixels that the tiles use => check if the decoding resources in this device are
    // sufficient to decode the tiles
    uint32_t maxPixelsInSec = DeviceInfo::maxDecodedPixelCountPerSecond() - aBaseLayerDecoderPixelsInSec;
    if (aTiles.hasSeparateStereoTiles())
    {
        // stereo => 1/2 of the capacity available per eye
        maxPixelsInSec /= 2;
    }

    checkTileResolutions(aTiles, maxTileCountRowCenter + (aViewportHeight / 2),
                         maxTileCountRowCenter - (aViewportHeight / 2), aViewportWidth, maxPixelsInSec, aDirection);

    if (aTiles.hasSeparateStereoTiles())
    {
        mNeededTileCount *= 2;
    }

    return true;
}

size_t VASTilePicker::estimateTileCount(VASTilesLayer& aTiles,
                                        float64_t aViewportWidth,
                                        float64_t aViewportHeight,
                                        float64_t& aMaxTileCountRowCenter,
                                        VASLongitudeDirection::Enum aDirection)
{
    size_t avgTileCount = 0;
    size_t maxTileCount = 0;
    size_t counter = 0;
    // rows are arranged smallest first, so we work on the southern hemisphere
    VASTileRows rows;
    aTiles.getRows(rows, StereoRole::UNKNOWN, 90, -90);
    // first, place the viewport on all tile row centers from pole to equator (inclusive) and check the needed tile
    // counts
    for (size_t i = 0; i < rows.getSize(); i++)
    {
        float64_t center = rows.at(i)->getTiles().at(0)->getCoveredViewport().getCenterLatitude();
        if (center > 0)
        {
            // we work on the southern hemisphere
            break;
        }
        VASTileRows rowsInner;
        aTiles.getRows(rowsInner, StereoRole::UNKNOWN, center + aViewportHeight / 2, center - aViewportHeight / 2);
        size_t tileCount = 0;
        for (VASTileRows::ConstIterator it = rowsInner.begin(); it != rowsInner.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();
            tileCount += countNeededHorizontalTiles(tilesInRow, aViewportWidth, aDirection);
        }
        avgTileCount += tileCount;
        counter++;
        if (tileCount > maxTileCount)
        {
            maxTileCount = tileCount;
            aMaxTileCountRowCenter = center;
        }
    }
    // then check the same when viewport is at tile row boundaries
    for (size_t i = 0; i < rows.getSize(); i++)
    {
        float64_t top, bottom;
        rows.at(i)->getTiles().at(0)->getCoveredViewport().getTopBottom(top, bottom);
        // now use top as the center of viewport (top in south starts from first tile row boundary
        if (top > 0)
        {
            // we work on the southern hemisphere
            break;
        }
        VASTileRows rowsInner;
        aTiles.getRows(rowsInner, StereoRole::UNKNOWN, top + aViewportHeight / 2, top - aViewportHeight / 2);
        size_t tileCount = 0;
        for (VASTileRows::ConstIterator it = rowsInner.begin(); it != rowsInner.end(); ++it)
        {
            const VASTiles& tilesInRow = (*it)->getTiles();
            tileCount += countNeededHorizontalTiles(tilesInRow, aViewportWidth, aDirection);
        }
        avgTileCount += tileCount;
        counter++;
        if (tileCount > maxTileCount)
        {
            maxTileCount = tileCount;
            aMaxTileCountRowCenter = top;
        }
    }

    return (size_t)(avgTileCount / counter);
}

size_t VASTilePicker::getNrVisibleTiles(VASTilesLayer& aTiles,
                                        float64_t aViewportWidth,
                                        float64_t aViewportHeight,
                                        VASLongitudeDirection::Enum aDirection)
{
    float64_t maxRow = 0.f;

    size_t count = estimateTileCount(aTiles, aViewportWidth, aViewportHeight, maxRow, aDirection);

    if (aTiles.hasSeparateStereoTiles() &&
        DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::FULL_STEREO)
    {
        count *= 2;
    }

    return count;
}

VASTileSelection VASTilePicker::estimateTiles(VASTilesLayer& aAllTiles,
                                              float64_t aTop,
                                              float64_t aBottom,
                                              float64_t aLeft,
                                              float64_t aRight)
{
    VASTileRows rows;
    aAllTiles.getRows(rows, StereoRole::MONO, aTop, aBottom);
    VASTileSelection viewportTiles;
    for (VASTileRows::Iterator row = rows.begin(); row != rows.end(); ++row)
    {
        const VASTiles& tilesInRow = (*row)->getTiles();
        for (size_t i = 0; i < tilesInRow.getSize(); i++)
        {
            VASTileContainer* tile = tilesInRow.at(i);
            float64_t left, right;
            tile->getCoveredViewport().getLeftRight(left, right);
            if (aRight < -180.f)
            {
                // across +-180
                if (right >= aLeft && left < aRight + 360.f)
                {
                    continue;
                }
            }
            else if (aLeft > 180.f)
            {
                // across +-180
                if (left <= aRight && right > aLeft - 360.f)
                {
                    continue;
                }
            }
            else if (left <= aRight)
            {
                continue;
            }
            else if (right >= aLeft)
            {
                break;
            }
            // OMAF_LOG_V("Include tile between %f...%f ", left, right);
            viewportTiles.add(tile);
        }
    }
    OMAF_ASSERT(!viewportTiles.isEmpty(), "No tiles for viewport!");
    return viewportTiles;
}

void_t VASTilePicker::forceToMono(bool_t aMono)
{
    mForcedToMono = aMono;
}

OMAF_NS_END
