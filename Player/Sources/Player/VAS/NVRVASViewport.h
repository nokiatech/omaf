
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

#include "Provider/NVRCoreProvider.h"

OMAF_NS_BEGIN
    
    namespace VASTileType
    {
        enum Enum
        {
            INVALID = -1,
            EQUIRECT_ENHANCEMENT,
            //EQUIRECT
            //CUBEMAP_ENHANCEMENT,
            CUBEMAP,
            COUNT
        };
    }

namespace VASLongitudeDirection
{
    enum Enum
    {
        INVALID = -1,
        CLOCKWISE,          // longitude increases from left to right
        COUNTER_CLOCKWISE,  // OMAF, longitude increases from right to left. This is the internal direction in viewport classes
        COUNT
    };
}

    /*
     * Viewport/tile representing area in equirect video
     */
    class VASTileViewport
    {
    public:
        VASTileViewport();
        virtual ~VASTileViewport();

        
        virtual void_t set(float64_t longitude, float64_t latitude, float64_t horFov, float64_t verFov, VASLongitudeDirection::Enum aDirection);
        virtual float64_t getCenterLatitude() const;
        virtual float64_t getCenterLongitude(VASLongitudeDirection::Enum aDirection = VASLongitudeDirection::COUNTER_CLOCKWISE) const;
        virtual void_t getTopBottom(float64_t& top, float64_t& bottom) const;
        virtual void_t getLeftRight(float64_t& left, float64_t& right) const;
        virtual void_t getSpan(float64_t& horFov, float64_t& verFov) const;
        virtual bool_t operator==(const VASTileViewport& other) const;
        virtual VASTileType::Enum getTileType() const;
    protected:
        float64_t mLongitude;
        float64_t mLatitude;
        float64_t mHorFov;
        float64_t mVerFov;

        float64_t mTopLeftX;
        float64_t mBottomLeftX;
        float64_t mTopRightX;
        float64_t mBottomRightX;
        float64_t mTopY;
        float64_t mBottomY;
    };

    class VASTileViewportCube : public VASTileViewport
    {
    public:
        VASTileViewportCube();
        virtual ~VASTileViewportCube();

        virtual VASTileType::Enum getTileType() const;
    };

    /*
     * Viewport used by the renderer; can be non-rectangular due to equirectangular mapping
     */
    class VASRenderedViewport : public VASTileViewport
    {
    public:
        VASRenderedViewport();
        virtual ~VASRenderedViewport();

        void_t setPosition(float64_t longitude, float64_t latitude, float64_t width, float64_t height, VASTileType::Enum aTileType);

        virtual float64_t intersect(const VASTileViewport& tile) const;
    private:
        float64_t calcEquiRLeftX(float64_t aLimitY, float64_t aTileY) const;
        float64_t calcEquiRRightX(float64_t aLimitY, float64_t aTileY) const;
        float64_t doCheckIntersection(const VASTileViewport& tile, bool_t wrapLeft = false) const;
        float64_t doCheckIntersectionCubeMiddle(const VASTileViewportCube& tile) const;
        float64_t doCheckIntersectionCubeTopBottom(const VASTileViewportCube& tile) const;
        float64_t findIntersectionWidth(float64_t tileLeftX, float64_t tileRightX) const;
    private:
        float64_t mSlopeLeft;
        float64_t mSlopeRight;
        float64_t mShiftLeft;
        float64_t mShiftRight;

    };


OMAF_NS_END
