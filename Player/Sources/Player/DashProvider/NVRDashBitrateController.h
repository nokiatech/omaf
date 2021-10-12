
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
#include "DashProvider/NVRDashIssueType.h"
#include "DashProvider/NVRDashVideoDownloader.h"
#include "Foundation/NVRFixedArray.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

class VASTilePicker;

class DashBitrateContoller
{
public:
    DashBitrateContoller();
    virtual ~DashBitrateContoller();

    virtual void_t initialize(DashVideoDownloader* aVideoDownloader);

    virtual bool_t update(uint32_t aBandwidthOverhead);
    bool_t reportDownloadProblem(IssueType::Enum aIssue);

    virtual void_t setNrQualityLevels(uint8_t aLevel);
    virtual bool_t getQualityLevelForeground(uint8_t& aLevel, uint8_t& aNrLevels) const;
    virtual bool_t getQualityLevelBackground(uint8_t& aLevel, uint8_t& aNrLevels) const;
    virtual bool_t getQualityLevelMargin(uint8_t& aLevel) const;
    virtual bool_t getQualityLevelMargin(size_t aNrViewportTiles, size_t& aMarginTilesBudget, uint8_t& aLevel) const;

protected:
    virtual void_t doUpdate();
    virtual bool_t setBitrate(size_t aBitrateIndex);
    virtual void_t increaseCache();

private:
    bool_t canSwitch();
    int32_t findDowngradeBitrateIndex(const Bitrates& aBitrates, const uint32_t aAvailableBandwidth);
    size_t getQualityFromThroughput(const Bitrates& aBitrates, uint32_t throughput);

protected:
    DashVideoDownloader* mVideoDownloader;
    int32_t mCurrentBitrateIndex;
    uint32_t mIssueCount;
    uint8_t mNrQualityLevels;
    uint8_t mQualityBackground;
    uint8_t mQualityForeground;
    uint32_t mNrVideoBitrates;
    Bitrates mPossibleBitrates;

private:
    uint32_t mLastBitrateCheckTimeMs;
    uint64_t mUpdateInterval;
    bool_t mRetry;
    bool_t mCanEstimate;
};

class DashBitrateContollerExtractor : public DashBitrateContoller
{
public:
    DashBitrateContollerExtractor();
    virtual ~DashBitrateContollerExtractor();

    virtual void_t initialize(DashVideoDownloader* aVideoDownloader);

protected:
    virtual void_t doUpdate();
    virtual bool_t setBitrate(size_t aBitrateIndex);
};


OMAF_NS_END
