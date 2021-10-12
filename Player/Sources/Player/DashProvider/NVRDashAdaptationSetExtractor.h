
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

#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "VAS/NVRVASTilePicker.h"

OMAF_NS_BEGIN

class DashAdaptationSetExtractor : public DashAdaptationSetTile
{
public:
    DashAdaptationSetExtractor(DashAdaptationSetObserver& observer);

    virtual ~DashAdaptationSetExtractor();

    static bool_t isExtractor(dash::mpd::IAdaptationSet* aAdaptationSet);
    static uint32_t parseId(dash::mpd::IAdaptationSet* aAdaptationSet);
    static AdaptationSetBundleIds hasPreselection(dash::mpd::IAdaptationSet* aAdaptationSet, uint32_t aAdaptationSetId);

	/**
	 * @returns number of elements in dependingRepresentationIds
	 */
	static uint32_t hasDependencies(dash::mpd::IAdaptationSet* aAdaptationSet,
                                    RepresentationDependencies& dependingRepresentationIds);

    static bool_t hasMultiResolution(dash::mpd::IAdaptationSet* aAdaptationSet);

public:  // from DashAdaptationSet
    virtual AdaptationSetType::Enum getType() const;

    virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);

    virtual Error::Enum startDownload(time_t startTime);
    virtual Error::Enum startDownload(time_t startTime, uint32_t aBufferingTimeMs);
    virtual Error::Enum startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                 uint32_t aNextToBeProcessedSegmentId,
                                                 uint32_t aBufferingTimeMs);
    virtual Error::Enum startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs);
    virtual Error::Enum stopDownload();
    virtual Error::Enum stopDownloadAsync(bool_t aAbort, bool_t aReset);
    virtual void_t clearDownloadedContent();
    virtual bool_t processSegmentDownload();
    virtual uint32_t getCurrentBandwidth() const;
    virtual bool_t isABRSwitchOngoing() const;
    virtual void_t handleSpeedFactor(float32_t aSpeedFactor);
    virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);
    virtual bool_t isBuffering();
    virtual bool_t isEndOfStream();
    virtual bool_t isError();
    virtual void_t
    setQualityLevels(uint8_t aFgQuality, uint8_t aMarginQuality, uint8_t aBgQuality, uint8_t aNrQualityLevels);
    virtual void_t setBufferingTime(uint32_t aBufferingTimeMs);
    virtual uint32_t getTimestampMsOfLastDownloadedFrame();

public:  // DashRepresentationObserver
    virtual void_t onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs);
    virtual void_t onNewStreamsCreated(MediaContent& aContent);

public:  // new
    virtual bool_t calculateBitrates(const VASTileSelection& aForegroundTiles, uint8_t aNrQualityLevels);
    virtual uint32_t calculateBitrate(const VASTileSelection& aForegroundTiles,
                                      uint8_t aNrQualityLevels,
                                      uint8_t aQualityFg,
                                      uint8_t aQualityBg);
    virtual const AdaptationSetBundleIds& getSupportingSets() const;
    virtual void_t addSupportingSet(DashAdaptationSetTile* aSupportingSet);
    virtual uint32_t getNextProcessedSegmentId() const;
    virtual void_t concatenateIfReady();
    virtual size_t getNrForegroundTiles();
    virtual void_t setVideoStreamId(streamid_t aStreamId);
    virtual streamid_t getVideoStreamId() const;

protected:  // from DashAdaptationSet
    virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
    virtual DashRepresentation* createRepresentation(DashComponents aDashComponents,
                                                     uint32_t aInitializationSegmentId,
                                                     uint32_t aBandwidth);
    virtual void_t parseVideoViewport(DashComponents& aNextComponents);

protected:  // new
    virtual void_t concatenateAndParseSegments();

protected:  // new member variables
    // relevant with extractor types only; collects the adaptation sets that this set depends on
    AdaptationSetBundleIds mSupportingSetIds;
    // there is only 64 tile flag bits so it is the limit
    typedef FixedArray<DashAdaptationSetTile*, 64> SupportingAdaptationSets; 
    SupportingAdaptationSets mSupportingSets;

    uint32_t mNextSegmentToBeConcatenated;
    uint32_t mNrSegmentsProcessed;
    uint32_t mTargetNextSegmentId;
    uint32_t mBufferingTimeMs;

    uint64_t mHighestSegmentIdtoDownload;
    uint64_t mCurrFlagsTileDownload;
    uint64_t mMaskTileDownload;
};
OMAF_NS_END
