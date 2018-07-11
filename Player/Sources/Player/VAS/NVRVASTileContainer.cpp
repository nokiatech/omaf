
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
#include "VAS/NVRVASTileContainer.h"
#include "Foundation/NVRLogger.h"


OMAF_NS_BEGIN

OMAF_LOG_ZONE(VASTileContainer);

    const int32_t KOverlapMarginPixels = 10;

    VASTileContainer::VASTileContainer(DashAdaptationSetSubPicture* aSet, const VASTileViewport& aTileViewport)
        : mAdaptationSet(aSet)
        , mTileViewport(aTileViewport)
        , mIsSelected(false)
        , mIntersectionArea(0.f)
    {

    }

    VASTileContainer::~VASTileContainer()
    {
        // the mAdaptationSet is only a reference, do not delete it here
    }

    const VASTileViewport& VASTileContainer::getCoveredViewport() const
    {
        OMAF_ASSERT_NOT_NULL(mAdaptationSet);
        return mTileViewport;
    }

    DashAdaptationSetSubPicture* VASTileContainer::getAdaptationSet() const
    {
        return mAdaptationSet;
    }

    MP4VideoStreams VASTileContainer::getVideoStreams() const
    {
        OMAF_ASSERT_NOT_NULL(mAdaptationSet);
        return mAdaptationSet->getCurrentVideoStreams();
    }

    void_t VASTileContainer::setSelected(bool_t selected)
    {
        mIsSelected = selected;
    }

    bool_t VASTileContainer::getSelected() const
    {
        return mIsSelected;
    }

    void_t VASTileContainer::setIntersectionArea(float64_t area)
    {
        mIntersectionArea = area;
    }

    float64_t VASTileContainer::getIntersectionArea() const
    {
        return mIntersectionArea;
    }


    VASTileRow::VASTileRow(float64_t latitudeCenter, float64_t latitudeTop, float64_t latitudeBottom)
    : mLatitudeCenter(latitudeCenter)
    , mLatitudeTop(latitudeTop)
    , mLatitudeBottom(latitudeBottom)
    {

    }
    VASTileRow::~VASTileRow()
    {
        for (VASTiles::Iterator it = mTiles.begin(); it != mTiles.end(); ++it)
        {
            OMAF_DELETE_HEAP((*it));
        }
        mTiles.clear();
        for (VASTiles::Iterator it = mTilesShifted.begin(); it != mTilesShifted.end(); ++it)
        {
            OMAF_DELETE_HEAP((*it));
        }
        mTilesShifted.clear();
    }

    const VASTiles& VASTileRow::getTiles() const
    {
        return mTiles;
    }
    const VASTiles& VASTileRow::getTilesShifted() const
    {
        return mTilesShifted;
    }

    void_t VASTileRow::clearSelections()
    {
        for (VASTiles::Iterator it = mTiles.begin(); it != mTiles.end(); ++it)
        {
            (*it)->setSelected(false);
        }

        for (VASTiles::Iterator it = mTilesShifted.begin(); it != mTilesShifted.end(); ++it)
        {
            (*it)->setSelected(false);
        }
    }

    static bool_t overlapInRow(const VASTileViewport& first, const VASTileViewport& second)
    {
        float64_t left0, right0;
        first.getLeftRight(left0, right0);
        float64_t left1, right1;
        second.getLeftRight(left1, right1);

        // in case a tile spans across +-180 degree boundary, the coordinates are given greater than 180 degree (or smaller than -180) since left = center - span/2, right = center + span/2
        if (right0 > left1 + KOverlapMarginPixels)
        {
            return true;
        }
        return false;
    }

    Error::Enum VASTileRow::add(DashAdaptationSetSubPicture* aSet, const VASTileViewport& aTileViewport)
    {
        if (mTiles.getSize() == 0)
        {
            // the very first one, add to mTiles
            mTiles.add(OMAF_NEW_HEAP(VASTileContainer)(aSet, aTileViewport));
            return Error::OK;
        }
        else
        {
            for (size_t i = 0; i < mTiles.getSize(); i++)
            {
                if (aTileViewport.getCenterLongitude() < mTiles[i]->getCoveredViewport().getCenterLongitude())
                {
                    // found an ordered place
                    mTiles.add(OMAF_NEW_HEAP(VASTileContainer)(aSet, aTileViewport), i);
                    return Error::OK;
                }
            }
        }

        // add to the end; mTiles.getSize() > 0 was checked above
        size_t last = mTiles.getSize() - 1;
        mTiles.add(OMAF_NEW_HEAP(VASTileContainer)(aSet, aTileViewport));
        return Error::OK;
    }

    void_t VASTileRow::sortAddedSets()
    {
        for (size_t i = 0; i < mTiles.getSize() - 1; i++)
        {
            // if overlaps with mTiles, move to mTilesShifted instead of mTiles
            if (overlapInRow(mTiles[i]->getCoveredViewport(), mTiles[i+1]->getCoveredViewport()))
            {
                mTilesShifted.add(mTiles[i+1]);
                mTiles.removeAt(i+1);
            }
        }
    }

    float64_t VASTileRow::getLatitude()
    {
        return mLatitudeCenter;
    }

    float64_t VASTileRow::getLatitudeTop()
    {
        return mLatitudeTop;
    }

    float64_t VASTileRow::getLatitudeBottom()
    {
        return mLatitudeBottom;
    }



    VASTilesLayer::VASTilesLayer()
    {

    }

    VASTilesLayer::~VASTilesLayer()
    {
        clear();
    }

    void_t VASTilesLayer::clear()
    {
        for (VASTileRows::Iterator it = mRowsL.begin(); it != mRowsL.end(); ++it)
        {
            OMAF_DELETE_HEAP((*it));
        }
        mRowsL.clear();

        for (VASTileRows::Iterator it = mRowsR.begin(); it != mRowsR.end(); ++it)
        {
            OMAF_DELETE_HEAP((*it));
        }
        mRowsR.clear();

        mAdaptationSets.clear();
    }

    bool_t VASTilesLayer::isEmpty() const
    {
        return (mRowsL.isEmpty() && mRowsR.isEmpty());
    }

    void_t VASTilesLayer::clearSelections()
    {
        for (VASTileRows::Iterator it = mRowsL.begin(); it != mRowsL.end(); ++it)
        {
            (*it)->clearSelections();
        }
        for (VASTileRows::Iterator it = mRowsR.begin(); it != mRowsR.end(); ++it)
        {
            (*it)->clearSelections();
        }
    }

    size_t VASTilesLayer::getNrAdaptationSets() const
    {
        return mAdaptationSets.getSize();
    }

    DashAdaptationSetSubPicture* VASTilesLayer::getAdaptationSetAt(size_t index) const
    {
        return mAdaptationSets.at(index);
    }

    const TileAdaptationSets& VASTilesLayer::getAdaptationSets()
    {
        return mAdaptationSets;
    }

    void_t VASTilesLayer::add(DashAdaptationSetSubPicture* aSet, const VASTileViewport& aTileViewport)
    {
        mAdaptationSets.add(aSet);
        if (aTileViewport.getTileType() == VASTileType::EQUIRECT_ENHANCEMENT)
        {
            if (aSet->getVideoChannel() == StereoRole::RIGHT)
            {
                doAddEquirect(aSet, aTileViewport, mRowsR);
            }
            else
            {
                doAddEquirect(aSet, aTileViewport, mRowsL);
            }
            mTileType = VASTileType::EQUIRECT_ENHANCEMENT;
        }
        else
        {
            //TODO something special for cubemap? E.g. face-based data structure? At minimum, should ensure the top and bottom tiles coverage is extended
            if (aSet->getVideoChannel() == StereoRole::RIGHT)
            {
                doAddEquirect(aSet, aTileViewport, mRowsR);
            }
            else
            {
                doAddEquirect(aSet, aTileViewport, mRowsL);
            }
            mTileType = VASTileType::CUBEMAP;
        }
    }

    void_t VASTilesLayer::doAddEquirect(DashAdaptationSetSubPicture* aSet, const VASTileViewport& aTileViewport, VASTileRows& aRows)
    {
        for (size_t i = 0; i < aRows.getSize(); i++)
        {
            if (aRows[i]->getLatitude() == aTileViewport.getCenterLatitude()) // comparing equality of floats, but they are stored values, not computed
            {
                aRows[i]->add(aSet, aTileViewport);
                return;
            }
        }
        // not found, create a row
        float64_t top, bottom;
        aTileViewport.getTopBottom(top, bottom);
        VASTileRow* row = OMAF_NEW_HEAP(VASTileRow)(aTileViewport.getCenterLatitude(), top, bottom);
        row->add(aSet, aTileViewport);
        // add in increasing order
        size_t i = 0;
        for (; i < aRows.getSize(); i++)
        {
            if (aRows[i]->getLatitude() > aTileViewport.getCenterLatitude())
            {
                break;
            }
        }
        aRows.add(row, i);
    }

    void_t VASTilesLayer::allSetsAdded(bool_t aCheckOverlaps)
    {
        if (aCheckOverlaps)
        { 
            // check overlaps and organize sets on all rows
            for (size_t i = 0; i < mRowsR.getSize(); i++)
            {
                mRowsR[i]->sortAddedSets();
            }
            for (size_t i = 0; i < mRowsL.getSize(); i++)
            {
                mRowsL[i]->sortAddedSets();
            }
        }
    }

    bool_t VASTilesLayer::hasSeparateStereoTiles() const
    {
        return !(mRowsR.isEmpty());
    }

    size_t VASTilesLayer::getRows(VASTileRows& aOutputRows, StereoRole::Enum channel, float64_t latitudeTop, float64_t latitudeBottom) const
    {
        aOutputRows.clear();
        if (channel == StereoRole::RIGHT)
        {
            return getRowsInternal(aOutputRows, mRowsR, latitudeTop, latitudeBottom);
        }
        else
        {
            return getRowsInternal(aOutputRows, mRowsL, latitudeTop, latitudeBottom);
        }
    }

    size_t VASTilesLayer::getRowsInternal(VASTileRows& aOutputRows, const VASTileRows& aSourceRows, float64_t aLatitudeTop, float64_t aLatitudeBottom) const
    {
        // first check if we have a row that completely covers the viewport
        bool_t found = false;
        for (size_t i = 0; i < aSourceRows.getSize(); i++)
        {
            if ((aSourceRows[i]->getLatitudeTop() >= aLatitudeTop && aSourceRows[i]->getLatitudeBottom() <= aLatitudeBottom))
            {
                aOutputRows.add(aSourceRows[i]);
                found = true;
                break;
            }
        }
        if (!found)
        {
            // then check for partial coverage
            for (size_t i = 0; i < aSourceRows.getSize(); i++)
            {
                if ((aSourceRows[i]->getLatitudeTop() >= aLatitudeTop && aSourceRows[i]->getLatitudeBottom() < aLatitudeTop) || // crosses top
                    (aSourceRows[i]->getLatitudeTop() > aLatitudeBottom && aSourceRows[i]->getLatitudeBottom() <= aLatitudeBottom) || // crosses bottom
                    (aSourceRows[i]->getLatitudeTop() <= aLatitudeTop && aSourceRows[i]->getLatitudeBottom() >= aLatitudeBottom)) // is inside
                {
                    aOutputRows.add(aSourceRows[i]);
                }
            }
            if (aOutputRows.isEmpty())
            {
                return 0;
            }
            // then check if there are overlapped rows. 
            // E.g. a resource optimization for the 6x1 + poles tile scheme could have the pole tile splitted into two: actual pole tile + a horizontal tile supporting the pole when watching the pole that reduces resource usage compared to the use of vertical tiles in that case
            for (size_t i = 0; i < aOutputRows.getSize()-1; )
            {
                if (aOutputRows.at(i)->getLatitudeTop() > aOutputRows.at(i + 1)->getLatitudeBottom())
                {
                    // the rows i and i+1 overlap
                    if (aLatitudeTop <= aOutputRows.at(i)->getLatitudeTop())
                    {
                        // southern hemisphere
                        // the row i+1 appears useless, since the row i covers the required area?
                        aOutputRows.removeAt(i + 1);
                    }
                    else if (aLatitudeBottom >= aOutputRows.at(i + 1)->getLatitudeBottom())
                    {
                        // northern hemisphere
                        // the row i appears useless, since the row i+1 covers the required area?
                        aOutputRows.removeAt(i);
                    }
                    else if (aOutputRows.at(i + 1)->getLatitudeBottom() <= aOutputRows.at(i)->getLatitudeBottom())
                    {
                        // the row i appears useless, since it does not cover the viewport completely, and the row above it cover the same area?
                        aOutputRows.removeAt(i);
                    }
                    else if (aOutputRows.at(i)->getLatitudeTop() >= aOutputRows.at(i + 1)->getLatitudeTop())
                    {
                        // the row i+1 appears useless, since it does not cover the viewport completely, and the row below it cover the same area?
                        aOutputRows.removeAt(i + 1);
                    }
                    else
                    {
                        i++;
                    }
                }
                else
                {
                    i++;
                }
            }
        }
        return aOutputRows.getSize();
    }

    VASTileType::Enum VASTilesLayer::getTileType() const
    {
        return mTileType;
    }

OMAF_NS_END
