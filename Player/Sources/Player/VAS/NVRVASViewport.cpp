
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
#include "VAS/NVRVASViewport.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(VASTileViewport);

    VASTileViewport::VASTileViewport()
        : mLongitude(0.f)
        , mLatitude(0.f)
        , mHorFov(360.f)
        , mVerFov(180.f)
        , mTopLeftX(180.f)
        , mBottomLeftX(180.f)
        , mTopRightX(-180.f)
        , mBottomRightX(-180.f)
        , mTopY(90.f)
        , mBottomY(-90.f)
    {
    }

    VASTileViewport::~VASTileViewport()
    {
    }


    void_t VASTileViewport::set(float64_t longitude, float64_t latitude, float64_t horFov, float64_t verFov, VASLongitudeDirection::Enum aDirection)
    {
        if (aDirection == VASLongitudeDirection::CLOCKWISE)
        {
            mLongitude = (-1)*longitude;
        }
        else
        {
            mLongitude = longitude;
        }
        mLatitude = latitude;
        mTopLeftX = mBottomLeftX = mLongitude + horFov / 2;
        mTopRightX = mBottomRightX = mLongitude - horFov / 2;
        mHorFov = horFov;
        mVerFov = verFov;
        mTopY = mLatitude + verFov / 2;
        mBottomY = mLatitude - verFov / 2;
    }

    float64_t VASTileViewport::getCenterLatitude() const
    {
        return mLatitude;
    }

    float64_t VASTileViewport::getCenterLongitude(VASLongitudeDirection::Enum aDirection) const
    {
        if (aDirection == VASLongitudeDirection::CLOCKWISE)
        {
            return (-1)*mLongitude;
        }
        else
        {
            return mLongitude;
        }
    }

    void_t VASTileViewport::getTopBottom(float64_t& top, float64_t& bottom) const
    {
        top = mTopY;
        bottom = mBottomY;
    }

    void_t VASTileViewport::getLeftRight(float64_t& left, float64_t& right) const
    {
        if (mBottomLeftX < mTopLeftX)
        {
            left = mTopLeftX;
        }
        else
        {
            left = mBottomLeftX;
        }
        if (mBottomRightX < mTopRightX)
        {
            right = mTopRightX;
        }
        else
        {
            right = mBottomRightX;
        }
    }

    void_t VASTileViewport::getSpan(float64_t& horFov, float64_t& verFov) const
    {
        horFov = mHorFov;
        verFov = mVerFov;
    }

    bool_t VASTileViewport::operator==(const VASTileViewport& other) const
    {
        return ((mLongitude == other.mLongitude) && (mLatitude == other.mLatitude) &&
            (mHorFov == other.mHorFov) && (mVerFov == other.mVerFov));
    }

    VASTileType::Enum VASTileViewport::getTileType() const
    {
        return VASTileType::EQUIRECT_ENHANCEMENT; 
    }

    VASTileViewportCube::VASTileViewportCube()
        : VASTileViewport()
    {

    }

    VASTileViewportCube::~VASTileViewportCube()
    {

    }

    VASTileType::Enum VASTileViewportCube::getTileType() const
    {
        return VASTileType::CUBEMAP; 
    }


    /*
     *  Viewport from renderer. If the tiles are in equirectangular, the shape depends on the location in the equirect coordinates.
     *  The implementation simplifies the actual shape to contain straight sides, so it uses the same 
     *  4 corner points but draws direct lines between them.
     *  E.g. if viewport is at equator, the correct shape would look a bit like hour glass, but becomes a rectangle.
     *       Equator:               Towards north pole           South pole
     *  ------       ------            ----------                  ----
     *  \    /      |      |           \        /                 /    \
     *   )  (   =>  |      |            \      /                 /      \
     *  /    \      |      |             \    /                 /        \
     *  ------       ------               ----                  ----------
     *
     *  If latitude is significantly above or below 0, the shape becomes V-style, and the approximation gets closer.
     *  Viewport rotations are not yet supported, but rotation doesn't typically change the coverage area that much
     */

    VASRenderedViewport::VASRenderedViewport()
        : VASTileViewport()
        , mSlopeLeft(0.f)
        , mSlopeRight(0.f)
        , mShiftLeft(-180.f)
        , mShiftRight(180.f)
    {}

    VASRenderedViewport::~VASRenderedViewport()
    {
    }

    void_t VASRenderedViewport::setPosition(float64_t longitude, float64_t latitude, float64_t width, float64_t height, VASTileType::Enum aTileType)
    {
        mLongitude = longitude;
        mLatitude = latitude;
        mHorFov = width;
        mVerFov = height;

        // latitude increases to upwards, longitude to right
        // remember that latitude = 0 is at equator, and +-90 at poles; hence top is center + height
        mTopY = mLatitude + height / 2;
        if (mTopY > 90)
        {
            mTopY = 90;
        }
        mBottomY = mLatitude - height / 2;
        if (mBottomY < -90)
        {
            mBottomY = -90;
        }

        // this uses rough approximation of equirect projection properties - also even if tiles are in cubemap
        if (mLatitude + height / 2.f > 90.f)
        {
            // top of viewport crosses north pole
            // the bottom is at least 180 degree, plus something more depending how much the viewport crosses the pole
            float64_t crossing = mLatitude + height / 2.f - 90.f;
            float64_t angle = toDegrees(asin(crossing / (height / 2.f)));
            mTopLeftX = mLongitude + 180.f;
            mBottomLeftX = mLongitude + 90.f + angle;
            mTopRightX = mLongitude - 180.f;
            mBottomRightX = mLongitude - 90.f - angle;
        }
        else if (mLatitude - height / 2.f < -90.f)
        {
            // bottom of viewport crosses south pole
            // the top is at least 180 degree, plus something more depending how much the viewport crosses the pole
            float64_t crossing = mLatitude - height / 2.f + 90.f;
            float64_t angle = toDegrees(asin(crossing / (height / 2.f)));
            mTopLeftX = mLongitude + 90.f - angle;  // angle is < 0 when crossing < 0
            mBottomLeftX = mLongitude + 180.f;
            mTopRightX = mLongitude - 90.f + angle;
            mBottomRightX = mLongitude - 180.f;
        }
        else
        {
            float64_t stretchTopX = 0;
            float64_t stretchBottomX = 0;

            if (mLatitude + height / 2.f > 60.f)
            {
                // top of viewport is above 60 degree
                stretchTopX = 30;
            }
            else if (mLatitude - height / 2.f < -60.f)
            {
                // bottom of viewport is below -60 degree
                stretchBottomX = 30;
            }

            // the values may go out of +-180 range, but that simplifies the math below; the wrap needs to be taken into account after calculations
            mTopLeftX = mLongitude + width / 2 + stretchTopX;
            mBottomLeftX = mLongitude + width / 2 + stretchBottomX;
            mTopRightX = mLongitude - width / 2 - stretchTopX;
            mBottomRightX = mLongitude - width / 2 - stretchBottomX;
        }

        if (aTileType == VASTileType::EQUIRECT_ENHANCEMENT)
        {
            // For equirect tiles, we can also adjust the tile shapes when computing the intersection
            // slope (k) and shift (b) for equation: x = k*y + b
            // used when tile crops the viewport to estimate the x-position of the viewport edge (y == tile top/bottom)
            // simplification: ignores possible thinner part in the middle (if latitude is near 0), so in practice the area is larger than it should be; further, the factor is 0 if latitude = 0
            mSlopeLeft = (mTopLeftX - mBottomLeftX) / (mTopY - mBottomY);
            mShiftLeft = mTopLeftX - mSlopeLeft * mTopY;
            mSlopeRight = (mTopRightX - mBottomRightX) / (mTopY - mBottomY);
            mShiftRight = mTopRightX - mSlopeRight * mTopY;
        }
    }

    // tile is always rectangular shape, 'this' is viewport that can be non-rectangular
    float64_t VASRenderedViewport::intersect(const VASTileViewport& tile) const
    {
        float64_t tileLeftX, tileRightX;
        tile.getLeftRight(tileLeftX, tileRightX);
        if (tile.getTileType() == VASTileType::EQUIRECT_ENHANCEMENT)
        {
            if (tileLeftX <= 180.f && tileRightX >= -180.f)
            {
                // normal case, tile does not wrap around
                return doCheckIntersection(tile);
            }
            else
            {
                // this tile wraps from right to left; try intersection twice
                float64_t intersect = doCheckIntersection(tile);
                if (intersect == 0)
                {
                    intersect = doCheckIntersection(tile, true);
                }
                return intersect;
            }
        }
        else // cubemap
        {
            if (tile.getCenterLatitude() > 45.f || tile.getCenterLatitude() < -45.f)
            {
                // top / bottom face; assuming tile remains within a face and the same tile doesn't cover e.g. parts of top and front faces
                return doCheckIntersectionCubeTopBottom((VASTileViewportCube&)tile);
            }
            else if (tileLeftX <= 180.f && tileRightX >= -180.f)
            {
                // middle faces, tile does not wrap around
                return doCheckIntersectionCubeMiddle((VASTileViewportCube&)tile);
            }
            else
            {
                //TODO this seem to occur now with directional extractors
			    //OMAF_ASSERT(false, "Cubemap tile wrap around +-180 not supported atm");
            }
        }
        return 0.f;
    }

    float64_t VASRenderedViewport::calcEquiRLeftX(float64_t aLimitY, float64_t aTileY) const
    {
        if (aLimitY == aTileY)
        {
            // the viewport is cut on top/bottom by the tile, need to compute the left-most/right-most points for the viewport using the slope-formula (x = ky + b)
            return mShiftLeft + mSlopeLeft * aLimitY;
        }
        else // cubemap or the viewport top/bottom part is within the tile; use real values
        {
            return mBottomLeftX; // == mTopLeftX
        }
    }
    float64_t VASRenderedViewport::calcEquiRRightX(float64_t aLimitY, float64_t aTileY) const
    {
        if (aLimitY == aTileY)
        {
            // the viewport is cut on top/bottom by the tile, need to compute the left-most/right-most points for the viewport using the slope-formula (x = ky + b)
            return mShiftRight + mSlopeRight * aLimitY;
        }
        else // cubemap or the viewport top/bottom part is within the tile; use real values
        {
            return mBottomRightX; // == mTopRightX
        }
    }

    float64_t VASRenderedViewport::doCheckIntersection(const VASTileViewport& tile, bool_t wrapLeft) const
    {
        // take smallest top and largest bottom
        float64_t tileTopY, tileBottomY;
        tile.getTopBottom(tileTopY, tileBottomY);

        float64_t rTopY, rBottomY;
        getTopBottom(rTopY, rBottomY);

        float64_t topY = min(tileTopY, rTopY);
        float64_t bottomY = max(tileBottomY, rBottomY);
        float64_t height = topY - bottomY;
        if (height > 0)
        {
            // the tile and 'this' can intersect based on the y-coordinates

            float64_t tileLeftX, tileRightX;
            tile.getLeftRight(tileLeftX, tileRightX);

            float64_t topLeftX = mTopLeftX, 
                bottomLeftX = mBottomLeftX, 
                topRightX = mTopRightX, 
                bottomRightX = mBottomRightX;

            if (wrapLeft)
            {
                // check a wrap-around tile by virtually moving it to the right side; without this it is checked on the left side
                if (tileRightX > 0)
                {
                    // the tile is defined to be around +180
                    tileLeftX -= 360.f;
                    tileRightX -= 360.f;
                }
                else
                {
                    // the tile is defined to be around -180
                    tileLeftX += 360;
                    tileRightX += 360;
                }
            }

            // find top corners
            float64_t topLeftVP, topRightVP;
            topLeftVP = calcEquiRLeftX(topY, tileTopY);
            topRightVP = calcEquiRRightX(topY, tileTopY);

            // TODO would this need to be done for bottom left/right too, in equirect?
            if (topRightVP < -180.f)
            {
                // the viewport passes the +-180 degree boundary from -180
                if (topRightVP <= tileLeftX - 360.f)
                {
                    // the tile shifted left by 360 can overlap with the viewport
                    if (tileRightX > topLeftVP)
                    {
                        // the tile is on the right of the viewport in normal coordinates; tile shifted to -180...-480 space
                        tileLeftX -= 360.f;
                        tileRightX -= 360.f;
                    }
                    else if (tileRightX <= -180.f && tileLeftX >= 180.f)
                    {
                        // special case: the tile covers in practice full 360 degree as it overlaps with viewport in both sides of the +-180 boundary => keep the right but extend the left further beyond the -180
                        tileRightX -= 360.f;
                        // don't touch the right coordinate
                    }
                    // narrower wrap-around tiles are handled separately (with wrapLeft parameter)
                }
            }
            else if (topLeftVP > 180.f)
            {
                // the viewport passes the +-180 degree boundary from +180
                if (topLeftVP >= tileRightX + 360.f)
                {
                    // the tile shifted right by 360 can overlap with the viewport
                    if (tileLeftX < topRightVP)
                    {
                        // tile is on the left side of the viewport; tile shifted to +180...+480 space
                        tileLeftX += 360.f;
                        tileRightX += 360.f;
                    }
                    else if (tileRightX <= -180.f && tileLeftX >= 180.f)
                    {
                        // special case: the tile covers full 360 degree as it overlaps with viewport in both sides of the +-180 boundary => keep the left but extend the right further beyond +180
                        tileLeftX += 360.f;
                        // don't touch the left coordinate
                    }
                    // narrower wrap-around tiles are handled separately (with wrapLeft parameter)
                }
            }

            // top left corner
            if (tileLeftX <= topLeftVP)
            {
                // 'this' does not cross the left edge of tile
                topLeftX = tileLeftX;
            }
            else if (topLeftVP >= tileRightX)
            {
                // 'this' top left corner is inside the tile
                topLeftX = topLeftVP;
            }
            else
            {
                // triangle case: one corner is out of intersection
                topLeftX = tileRightX;
            }

            // top right corner
            if (tileRightX >= topRightVP)
            {
                // 'this' does not cross the right side of tile
                topRightX = tileRightX;
            }
            else if (topRightVP <= tileLeftX)
            {
                // 'this' top right corner is inside the tile
                topRightX = topRightVP;
            }
            else
            {
                // triangle case: one corner is out of intersection
                topRightX = tileLeftX;
            }

            // find bottom corners
            float64_t bottomLeftVP, bottomRightVP;
            bottomLeftVP = calcEquiRLeftX(bottomY, tileBottomY);
            bottomRightVP = calcEquiRRightX(bottomY, tileBottomY);

            // bottom left corner
            if (tileLeftX <= bottomLeftVP)
            {
                // 'this' does not cross the left side of tile
                bottomLeftX = tileLeftX;
            }
            else if (bottomLeftVP >= tileRightX)
            {
                bottomLeftX = bottomLeftVP;
            }
            else
            {
                // triangle case: one corner is out of intersection
                bottomLeftX = tileRightX;
            }

            // bottom right corner
            if (tileRightX >= bottomRightVP)
            {
                // 'this' does not cross the right side of tile
                bottomRightX = tileRightX;
            }
            else if (bottomRightVP <= tileLeftX)
            {
                bottomRightX = bottomRightVP;
            }
            else
            {
                bottomRightX = tileLeftX;
            }

            // check that the found top and bottom intersect also in x-coordinates
            if (topLeftX - topRightX >= 0 || bottomLeftX - bottomRightX >= 0)
            {
                // return the intersection area; for X take avg position of left and right
                return height * ((topLeftX + bottomLeftX) - (topRightX + bottomRightX));
                // should divide x by 2, but since we are only comparing values, are not interested in the absolute area, we can skip the division
                //return height * ((topLeftX + bottomLeftX)/2 - (topRightX + bottomRightX)/2);
            }
        }
        return 0;
    }

    float64_t VASRenderedViewport::findIntersectionWidth(float64_t aTileLeftX, float64_t aTileRightX) const
    {
        float64_t leftX = mTopLeftX,
            rightX = mTopRightX;

        if (rightX < -180.f)
        {
            // the viewport passes the +-180 degree boundary from -180, i.e. center is between -180 and 0 so right is below -180
            if (rightX <= aTileLeftX - 360.f)
            {
                // the tile shifted right by 360 can overlap with the viewport
                if (aTileRightX > leftX)
                {
                    // the tile is on the left of the viewport in normal coordinates; tile shifted to -180...-480 space
                    aTileLeftX -= 360.f;
                    aTileRightX -= 360.f;
                }
                else if (aTileRightX <= -180.f && aTileLeftX >= 180.f)
                {
                    // special case: the tile covers in practice full 360 degree as it overlaps with viewport in both sides of the +-180 boundary => keep the left but extend the right further beyond the -180
                    aTileRightX -= 360.f;
                    // don't touch the left coordinate
                }
                // narrower wrap-around tiles are handled separately (with wrapLeft parameter)
            }
        }
        else if (leftX > 180.f)
        {
            // the viewport passes the +-180 degree boundary from +180, i.e. center is between 0 and +180, so left is above +180
            if (leftX >= aTileRightX + 360.f)
            {
                // the tile shifted left by 360 can overlap with the viewport
                if (aTileLeftX < rightX)
                {
                    // tile is on the right side of the viewport; tile shifted to +180...+480 space
                    aTileLeftX += 360.f;
                    aTileRightX += 360.f;
                }
                else if (aTileRightX <= -180.f && aTileLeftX >= 180.f)
                {
                    // special case: the tile covers full 360 degree as it overlaps with viewport in both sides of the +-180 boundary => keep the left but extend the right further beyond +180
                    aTileLeftX += 360.f;
                    // don't touch the right coordinate
                }
                // narrower wrap-around tiles are handled separately (with wrapLeft parameter)
            }
        }

        // right side
        if (aTileRightX >= rightX)
        {
            // 'this' does not cross the left edge of tile
            rightX = aTileRightX;
        }

        // left side
        if (aTileLeftX <= leftX)
        {
            // 'this' does not cross the right side of tile
            leftX = aTileLeftX;
        }

        // return the width of the intersection
        if (leftX - rightX > 0)
        {
            return (leftX - rightX);
        }

        return 0.f;
    }


    // simpler version, used with cubemap middle faces.
    float64_t VASRenderedViewport::doCheckIntersectionCubeMiddle(const VASTileViewportCube& tile) const
    {
        // take smallest top and largest bottom
        float64_t tileTopY, tileBottomY;
        tile.getTopBottom(tileTopY, tileBottomY);

        float64_t rTopY, rBottomY;
        getTopBottom(rTopY, rBottomY);

        float64_t topY = min(tileTopY, rTopY);
        float64_t bottomY = max(tileBottomY, rBottomY);
        float64_t height = topY - bottomY;
        if (height > 0)
        {
            // the tile and 'this' can intersect based on the y-coordinates

            float64_t tileLeftX, tileRightX;
            tile.getLeftRight(tileLeftX, tileRightX);

            // Find left and right boundaries, and check that the found top and bottom intersect also in x-coordinates
            return height * findIntersectionWidth(tileLeftX, tileRightX);
        }
        return 0;
    }

    // Version for cubemap top and bottom faces. The intersection is computed in parts: each side of top/bottom face is considered separately, as the top/bottom face covers the full 360 longitude
    float64_t VASRenderedViewport::doCheckIntersectionCubeTopBottom(const VASTileViewportCube& tile) const
    {
        // take smallest top and largest bottom
        float64_t tileTopY, tileBottomY;
        tile.getTopBottom(tileTopY, tileBottomY);

        float64_t rTopY, rBottomY;
        getTopBottom(rTopY, rBottomY);

        float64_t topY = min(tileTopY, rTopY);
        float64_t bottomY = max(tileBottomY, rBottomY);
        float64_t height = topY - bottomY;
        if (height > 0)
        {
            // the tile and 'this' can intersect based on the y-coordinates

            float64_t factor = 1;
            float64_t tileLeftX, tileRightX;
            tile.getLeftRight(tileLeftX, tileRightX);

            float64_t width = abs(tileLeftX - tileRightX);

            // only a limited set of tile combos supported for now
            if (width > 89.f)
            {
                // 90 degrees wide => extend to 180 degrees
                tileLeftX += 45.f;
                tileRightX -= 45.f;
                factor = 2;
                // check height too
                float64_t tileTopY, tileBottomY;
                tile.getTopBottom(tileTopY, tileBottomY);
                if (abs(tileTopY - tileBottomY) > 45.f)
                {
                    // only 1 tile, extend further to cover the whole face
                    tileLeftX = 180.f;
                    tileRightX = -180.f;
                    factor = 4;
                }
            }
            else if (width > 44.f)
            {
                // 45 degrees == 4 tiles per face, extend to 90 degrees
                tileLeftX += 22.5;
                tileRightX -= 22.5;
                factor = 2;
                // check height too
                float64_t tileTopY, tileBottomY;
                tile.getTopBottom(tileTopY, tileBottomY);
                if (abs(tileTopY - tileBottomY) > 45.f)
                {
                    // only 2 tiles, extend further to cover half of the face
                    tileLeftX += 45.f;
                    tileRightX -= 45.f;
                    factor = 4;
                }
            }

            return (height * findIntersectionWidth(tileLeftX, tileRightX) / factor);

        }
        return 0;
    }


OMAF_NS_END
