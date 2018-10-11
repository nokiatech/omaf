
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
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "VAS/NVRVASTilePicker.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashBitrateContoller);


DashBitrateContoller::DashBitrateContoller()
    : mTilePicker(OMAF_NULL)
    , mBaselayerAdaptationSet(OMAF_NULL)
    , mBaselayerAdaptationStereoSet(OMAF_NULL)
    , mCurrentBitrateIndex(0)
    , mNrBaselayerBitrates(1)
    , mAllowEnhancement(false)
    , mDelayedSegmentCount(0)
    , mUpdateInterval(2000)
    , mLastBitrateCheckTimeMs(0)
    , mRetry(false)
    , mUseWithEnhancementLayer(false)
    , mForcedToMono(false)
    , mNrQualityLevels(0)
    , mQualityForeground(1)
    , mQualityBackground(1)
{

}

DashBitrateContoller::~DashBitrateContoller()
{
    BandwidthMonitor::setMode(false);
}

void_t DashBitrateContoller::initialize(DashAdaptationSet* aBaseLayer, DashAdaptationSet* aBaseLayerStereo, const TileAdaptationSets& aEnhTiles, VASTilePicker* aTilepicker)
{
}

bool_t DashBitrateContoller::update(uint32_t aBandwidthOverhead)
{
    return false;
}

bool_t DashBitrateContoller::canSwitch()
{
    return false;
}

void_t DashBitrateContoller::doUpdate()
{
}

size_t DashBitrateContoller::findCorrectBitrateIndex(const Bitrates& aBitrates, const uint32_t aAvailableBandwidth, const uint32_t aCurrentBitrate)
{
    return 0;
}

uint32_t DashBitrateContoller::estimateAvailableBandwidth(uint32_t aBandwidthOverhead)
{
    return 0;
}

void_t DashBitrateContoller::dropToMono()
{
}

bool_t DashBitrateContoller::reportDownloadProblem(IssueType::Enum aIssueType)
{
    return false;
}

bool_t DashBitrateContoller::allowEnhancement() const
{
    return false;
}

void_t DashBitrateContoller::setNrQualityLevels(uint8_t aLevels)
{
    mNrQualityLevels = aLevels;
    mQualityBackground = aLevels;
}

bool_t DashBitrateContoller::getQualityLevelForeground(uint8_t& aLevel, uint8_t& aNrLevels) const
{
    //TODO the idea is that ABR would change at least mQualityForeground. 
    //TODO Initial value is currently the highest, is that in line with bitrate selection?
    if (mNrQualityLevels > 0)
    {
        aNrLevels = mNrQualityLevels;
        aLevel = mQualityForeground;
        return true;
    }
    else
    {
        return false;
    }
}

bool_t DashBitrateContoller::getQualityLevelBackground(uint8_t& aLevel, uint8_t& aNrLevels) const
{
    if (mNrQualityLevels > 0)
    {
        aNrLevels = mNrQualityLevels;
        aLevel = mQualityBackground;
        return true;
    }
    else
    {
        return false;
    }
}

OMAF_NS_END