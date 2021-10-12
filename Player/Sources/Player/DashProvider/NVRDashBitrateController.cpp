
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
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashSpeedFactorMonitor.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "VAS/NVRVASTilePicker.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashBitrateContoller);

const float32_t BITRATE_UPGRADE_MULTIPLIER = 1.3f;
const int64_t RETRY_INTERVAL_MS = 500;
const float32_t UPSWITCH_SPEED_FACTOR = 2.f;
const int ISSUE_REPORTING_LIMIT = 5;  // magic number 5 is based on experiments

DashBitrateContoller::DashBitrateContoller()
    : mVideoDownloader(OMAF_NULL)
    , mCurrentBitrateIndex(0)
    , mNrVideoBitrates(1)
    , mIssueCount(0)
    , mUpdateInterval(2000)
    , mLastBitrateCheckTimeMs(0)
    , mRetry(false)
    , mNrQualityLevels(0)
    , mQualityForeground(1)
    , mQualityBackground(1)
    , mCanEstimate(false)
{
}

DashBitrateContoller::~DashBitrateContoller()
{
    BandwidthMonitor::setMode(false);
}

void_t DashBitrateContoller::initialize(DashVideoDownloader* aVideoDownloader)
{
    OMAF_ASSERT(mVideoDownloader == OMAF_NULL, "Already initialized");
    mVideoDownloader = aVideoDownloader;

    mLastBitrateCheckTimeMs = Time::getClockTimeMs();
    // Store the possible bitrates to member (assuming they don't change over time even if mpd gets updated)
    // First all the baselayer bitrates for non-tiled use
    // Note! The whole bitrate controller assumes that baselayer bitrate(0) is always used with tiles, and that there
    // are no base-only bitrate higher than base(0) + tiles. There is however no linking in the mpd between base
    // layer(s) and enh layer(s), so using other base layers would be solely player's decision. Also, it assumes the
    // tiles have equal bitrates, i.e. the bitrates from tile(0) can be used with all tiles. Even though that may not be
    // accurate, it may be a fair approximation, considering the complexity of the accurate estimation.
    mPossibleBitrates.add(mVideoDownloader->getBitrates());
    mNrVideoBitrates = (uint32_t) mPossibleBitrates.getSize();
    mCanEstimate = BandwidthMonitor::setMode(true);
    if (mPossibleBitrates.isEmpty())
    {
        mCanEstimate = false;
    }
}

bool_t DashBitrateContoller::update(uint32_t aBandwidthOverhead)
{
    int32_t now = Time::getClockTimeMs();

    // old single problem don't stay there forever preventing upgrades

    if (mIssueCount > ISSUE_REPORTING_LIMIT)
    {
        mUpdateInterval = 1000;
        if (mCurrentBitrateIndex > 0)
        {
            // check the throughput
            bool_t isFunctional = false;
            uint32_t throughput = BandwidthMonitor::getInstantaneousThroughputInBps(isFunctional) - aBandwidthOverhead;
            uint32_t oldBitrateIndex = mCurrentBitrateIndex;
            if (isFunctional)
            {
                OMAF_LOG_ABR("ABRSwitchDown: Go down");
                setBitrate(findDowngradeBitrateIndex(mPossibleBitrates, throughput));
            }
            else
            {
                OMAF_LOG_ABR("ABRSwitchDown: Go down by 1 step");
                // bandwidth monitor not functional
                setBitrate(mCurrentBitrateIndex - 1);
            }

            doUpdate();
            mLastBitrateCheckTimeMs = Time::getClockTimeMs();

            OMAF_LOG_ABR(
                "ABR(Time/State/OldQ/NewQ/throughput/delayedsegments/speedfactor) %d ABR_RULE_THROUGHPUT %d %d %d "
                "%d %f",
                mLastBitrateCheckTimeMs, oldBitrateIndex, mCurrentBitrateIndex, throughput, mIssueCount,
                DashSpeedFactorMonitor::getDownloadSpeed());
            return true;
        }
        else
        {
            OMAF_LOG_ABR("ABRSwitchDown: Already at lowest bitrate, resetting mIssueCount");
            increaseCache();
            mIssueCount = 0;
            return false;
        }
    }
    else if (!mCanEstimate || mCurrentBitrateIndex == mPossibleBitrates.getSize() - 1 ||
             ((now - mLastBitrateCheckTimeMs < mUpdateInterval) &&
              !(mRetry && now - mLastBitrateCheckTimeMs > RETRY_INTERVAL_MS)))
    {
        // It doesn't make sense to check for upgrade too frequently
        return false;
    }
    mLastBitrateCheckTimeMs = now;
    mRetry = false;
    mUpdateInterval = 1000;
    if (mCurrentBitrateIndex < mPossibleBitrates.getSize() - 1 &&
        DashSpeedFactorMonitor::getDownloadSpeed() > UPSWITCH_SPEED_FACTOR)
    {
        // step up by 1
        if (canSwitch())
        {
            bool_t isFunctional = false;
            uint8_t bitrateIndex = mCurrentBitrateIndex + 1;
            /* Sanity check, don't try to switch if the new rate is unsustainable */
            uint32_t throughput = BandwidthMonitor::getInstantaneousThroughputInBps(isFunctional) - aBandwidthOverhead;
            uint8_t bitrateIndexfromThroughput = bitrateIndex;
            if (isFunctional)
            {
                bitrateIndexfromThroughput = getQualityFromThroughput(mPossibleBitrates, throughput);
            }
            /* only check to make sure we are not increasing to unsustainable levels */
            if (bitrateIndex > bitrateIndexfromThroughput)
            {
                if (bitrateIndexfromThroughput <= mCurrentBitrateIndex)
                {
                    OMAF_LOG_ABR(
                        "Sanity check Throughput: Current: %d, Time: %d, Throughput: %d (%d) - Enforcing no change ",
                        mCurrentBitrateIndex, bitrateIndex, bitrateIndexfromThroughput, throughput);
                    /* Avoid oscillations - do not drop quality until buffer level falls */
                    return false;
                }
                else
                {
                    /* In the single incremental case, this  case should never happen, but if the algorithm does
                    bigger increments in the future, this failsafe would avoid pitfalls */
                    OMAF_LOG_ABR(
                        "Sanity check Throughput: Current: %d, Time: %d, Throughput: %d - Enforcing change to "
                        "throughput index",
                        mCurrentBitrateIndex, bitrateIndex, bitrateIndexfromThroughput);
                    bitrateIndex = bitrateIndexfromThroughput;
                }
            }
            OMAF_LOG_V("ABRSwitchUp by 1 step");
            setBitrate(bitrateIndex);
            doUpdate();
            return true;
        }
        else
        {
            // use temporarily shorter time
            OMAF_LOG_ABR("ABRSwitchUp: Bitrate change from %d to %d not possible now, try again later",
                         mPossibleBitrates[mCurrentBitrateIndex], mPossibleBitrates[mCurrentBitrateIndex + 1]);
            mRetry = true;
        }
    }
    else
    {
        OMAF_LOG_ABR("Upgrade not possible: CurrentIndex: %d, Speedfactor %f", mCurrentBitrateIndex,
                     DashSpeedFactorMonitor::getDownloadSpeed());
    }

    return false;
}

size_t DashBitrateContoller::getQualityFromThroughput(const Bitrates& aBitrates, uint32_t throughput)
{
    size_t chosenIndex = 0;

    /*Not enough instances of throughput, don't change the quality*/
    if (throughput == 0)
    {
        return mCurrentBitrateIndex;
    }


    for (int i = 0; i < aBitrates.getSize(); i++)
    {
        if (aBitrates[i] < throughput)
        {
            chosenIndex = i;
        }
        else
        {
            break;
        }
    }

    return chosenIndex;
}

bool_t DashBitrateContoller::canSwitch()
{
    if (mVideoDownloader->isABRSwitchOngoing())
    {
        return false;
    }
    return true;
}

void_t DashBitrateContoller::doUpdate()
{
    OMAF_LOG_V("Chosen bitrate is: %d", mPossibleBitrates.at(mCurrentBitrateIndex));
    mVideoDownloader->selectBitrate(mPossibleBitrates.at(mCurrentBitrateIndex));
    mIssueCount = 0;
}

int32_t DashBitrateContoller::findDowngradeBitrateIndex(const Bitrates& aBitrates, const uint32_t aThroughput)
{
    // Set to 0 at first so that if nothing better is found, the lowest one is used
    size_t chosenIndex = 0;
    if (aThroughput == 0)
    {
        if (mCurrentBitrateIndex > 0)
        {
            // not enough samples available yet (?), try stepping by 1
            return mCurrentBitrateIndex - 1;
        }
        else
        {
            return mCurrentBitrateIndex;
        }
    }

    uint32_t currentBitrate = aBitrates.at(mCurrentBitrateIndex);
    if (aThroughput > currentBitrate)
    {
        // There should be enough throughput, but still some issues detected? Try stepping by 1
        return mCurrentBitrateIndex - 1;
    }

    if (aBitrates.getSize() > 1)
    {
        for (size_t i = mCurrentBitrateIndex; i > 0; i--)
        {
            // we came here since there were download issues, so we start checking from current-1 index
            size_t index = i - 1;
            uint32_t bitrate = aBitrates.at(index);

            if (aThroughput > bitrate)
            {
                chosenIndex = index;
                break;
            }
        }
    }
    return chosenIndex;
}

bool_t DashBitrateContoller::setBitrate(size_t aBitrateIndex)
{
    uint8_t delta = mCurrentBitrateIndex - aBitrateIndex;
    mCurrentBitrateIndex = aBitrateIndex;
    OMAF_LOG_V("Update bitrate to index %d", mCurrentBitrateIndex);
    return true;
}

void_t DashBitrateContoller::increaseCache()
{
    mVideoDownloader->increaseCache();
}

bool_t DashBitrateContoller::reportDownloadProblem(IssueType::Enum aIssueType)
{
    OMAF_LOG_W("download problem! Issue type: %d", aIssueType);
    if (aIssueType == IssueType::BASELAYER_BUFFERING)
    {
        mIssueCount = ISSUE_REPORTING_LIMIT;
    }

    mIssueCount++;
    return false;
}


void_t DashBitrateContoller::setNrQualityLevels(uint8_t aLevels)
{
    mNrQualityLevels = aLevels;
    mQualityBackground = aLevels;
}

bool_t DashBitrateContoller::getQualityLevelForeground(uint8_t& aLevel, uint8_t& aNrLevels) const
{
    // the idea is that ABR would change at least mQualityForeground.
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

bool_t DashBitrateContoller::getQualityLevelMargin(uint8_t& aLevel) const
{
    if (mNrQualityLevels > 2)
    {
        aLevel = mQualityForeground - 1;
        return true;
    }
    else
    {
        aLevel = mQualityBackground;
        return false;
    }
}

bool_t DashBitrateContoller::getQualityLevelMargin(size_t aNrViewportTiles,
                                                   size_t& aMarginTilesBudget,
                                                   uint8_t& aLevel) const
{
    if (mNrQualityLevels > 2)
    {
        aMarginTilesBudget = 4;
        aLevel = min(mQualityForeground + 1,
                     (int) mQualityBackground);
        return true;
    }
    else
    {
        aMarginTilesBudget = 0;
        aLevel = mQualityBackground;
        return false;
    }
}


DashBitrateContollerExtractor::DashBitrateContollerExtractor()
    : DashBitrateContoller()
{
}

DashBitrateContollerExtractor::~DashBitrateContollerExtractor()
{
}

void_t DashBitrateContollerExtractor::initialize(DashVideoDownloader* aVideoDownloader)
{
    DashBitrateContoller::initialize(aVideoDownloader);
    mCurrentBitrateIndex = mNrVideoBitrates - 1;
}

void_t DashBitrateContollerExtractor::doUpdate()
{
    OMAF_LOG_V("Chosen bitrate is: %d", mPossibleBitrates.at(mCurrentBitrateIndex));
    mVideoDownloader->setQualityLevels(mQualityForeground, min((int) mQualityBackground, mQualityForeground + 1),
                                       mQualityBackground, mNrQualityLevels);
    mIssueCount = 0;
}

bool_t DashBitrateContollerExtractor::setBitrate(size_t aBitrateIndex)
{
    int32_t delta = (int32_t) mCurrentBitrateIndex - (int32_t) aBitrateIndex;
    mCurrentBitrateIndex = aBitrateIndex;
    OMAF_LOG_V("Update bitrate to index %d", mCurrentBitrateIndex);
    if (mQualityForeground + delta <= mNrQualityLevels && (int32_t) mQualityForeground + delta > 0)
    {
        mQualityForeground += delta;
        return true;
    }
    else if (mQualityForeground + delta > mNrQualityLevels)
    {
        mQualityForeground = mNrQualityLevels;
    }
    else if ((int32_t) mQualityForeground + delta > 0)
    {
        mQualityForeground = 1;
    }
    else
    {
        return false;
    }
    return true;
}


OMAF_NS_END
