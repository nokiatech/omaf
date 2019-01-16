
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
#include "NVREssentials.h"
#include "Foundation/NVRFixedArray.h"
#include "DashProvider/NVRDashIssueType.h"
#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"

OMAF_NS_BEGIN

class DashAdaptationSet;
class VASTilePicker;

class DashBitrateContoller
{
public:
    DashBitrateContoller();
    ~DashBitrateContoller();

    void_t initialize(DashAdaptationSet* aBaseLayer, DashAdaptationSet* aBaseLayerStereo, const TileAdaptationSets& aTiles, VASTilePicker* aTilepicker);

    bool_t update(uint32_t aBandwidthOverhead);
    bool_t reportDownloadProblem(IssueType::Enum aIssue);

    bool_t allowEnhancement() const;

    void_t setNrQualityLevels(uint8_t aLevel);
    bool_t getQualityLevelForeground(uint8_t& aLevel, uint8_t& aNrLevels) const;
    bool_t getQualityLevelBackground(uint8_t& aLevel, uint8_t& aNrLevels) const;


private:
    bool_t canSwitch();
    void_t doUpdate();
    uint32_t estimateAvailableBandwidth(uint32_t aBandwidthOverhead);
    size_t findCorrectBitrateIndex(const Bitrates& aBitrates, const uint32_t aAvailableBandwidth, const uint32_t aCurrentBitrate);
    void_t dropToMono();

private:

    TileAdaptationSets mEnhTileAdaptationSets;  // Enhancement layer tiles that can be dropped, not OMAF tiles that are essential
    DashAdaptationSet* mAudioAdaptationSet;
    DashAdaptationSet* mBaselayerAdaptationSet;
    DashAdaptationSet* mBaselayerAdaptationStereoSet;

    Bitrates mPossibleBitrates;
    Bitrates mTileBitrates;
    size_t mCurrentBitrateIndex;
    uint32_t mNrBaselayerBitrates;
    Bitrates mPossibleMonoBitrates;

    VASTilePicker* mTilePicker; // Not own
    bool_t mAllowEnhancement;
    uint32_t mDelayedSegmentCount;
    uint32_t mLastBitrateCheckTimeMs;
    uint64_t mUpdateInterval;
    bool_t mRetry;
    bool_t mUseWithEnhancementLayer;

    bool_t mForcedToMono;

    uint8_t mNrQualityLevels;
    uint8_t mQualityBackground;
    uint8_t mQualityForeground;
};


OMAF_NS_END
