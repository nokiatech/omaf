
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

class DashRepresentationTile;

namespace TileRole
{
    enum Enum
    {
        INVALID = -1,
        FOREGROUND,
        FOREGROUND_MARGIN,
        FOREGROUND_POLE,
        BACKGROUND,
        BACKGROUND_POLE,
        NON_TILE_PARTIAL_ADAPTATION_SET,
        COUNT
    };
}

class DashAdaptationSetTile : public DashAdaptationSetSubPicture
{
public:
    DashAdaptationSetTile(DashAdaptationSetObserver& observer);

    virtual ~DashAdaptationSetTile();

    static bool_t isPartialAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet, SupportingAdaptationSetIds& aSupportingIds);
    static bool_t isTile(dash::mpd::IAdaptationSet* aAdaptationSet, SupportingAdaptationSetIds& aSupportingIds);
    static bool_t isTile(dash::mpd::IAdaptationSet* aAdaptationSet,
                         RepresentationDependencies& aDependingRepresentations);

public:  // from DashAdaptationSet
    virtual AdaptationSetType::Enum getType() const;

    virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
    virtual Error::Enum initialize(DashComponents aDashComponents,
                                   uint32_t& aInitializationSegmentId,
                                   uint32_t& aAdaptationSetId);
    virtual Error::Enum stopDownload();
    virtual uint8_t getNrQualityLevels() const;
    virtual bool_t isBuffering();
    virtual uint32_t getLastSegmentId(bool_t aIncludeSegmentInDownloading = false) const;
    virtual void_t handleSpeedFactor(float32_t aSpeedFactor);
    virtual void_t setBufferingTime(uint32_t aBufferingTimeMs);
    virtual void_t clearDownloadedContent();
    virtual bool_t processSegmentDownload();

public:  // new
    bool_t trySwitchingRepresentation(uint32_t aNextSegmentId);
    bool_t selectQuality(uint8_t aQualityLevel,
                         uint8_t aNrQualityLevels,
                         uint32_t aNextProcessedSegment,
                         TileRole::Enum aRole);
    void_t prepareQuality(uint8_t aQualityLevel, uint8_t aNrQualityLevels);
    uint32_t getLastSegmentIdByQ(bool_t aIncludeSegmentInDownloading = false) const;

    // called by extractor for supporting sets if dependencyId based collection of tiles is used
    bool_t selectRepresentation(DependableRepresentations& aDependencies, uint32_t aNextProcessedSegment);
    DashRepresentation* getRepresentationForQuality(uint8_t aQualityLevel, uint8_t aNrLevels);

    // called by extractor for supporting sets if dependencyId based collection of tiles is used, hence public;
    // otherwise called by the tile itself
    bool_t prepareForSwitch(uint32_t aNextProcessedSegment, bool_t aAggressiveSwitch);
    void_t cleanUpOldSegments(uint32_t aNextSegmentId);

    virtual bool_t isBuffering(uint32_t aNextSegmentToBeConcatenated);
    DashSegment* getSegment(uint32_t aSegmentId, uint8_t& aFromQualityLevel);
    bool_t hasSegment(uint32_t aSegmentId, size_t& aSegmentSize);
    void_t setRole(TileRole::Enum aRole);
    TileRole::Enum getRole() const;
    void_t switchMarginRole(bool_t aToMargin);
    /*Add mask to synchronize tile download*/
    void_t addTileFlag(uint64_t TileFlag);
    uint64_t getTileFlag();

protected:  // from DashAdaptationSet
    virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
    virtual DashRepresentation* createRepresentation(DashComponents aDashComponents,
                                                     uint32_t aInitializationSegmentId,
                                                     uint32_t aBandwidth);
    virtual bool_t parseVideoProperties(DashComponents& aNextComponents);
    virtual void_t parseVideoViewport(DashComponents& aNextComponents);
    virtual bool_t parseVideoQuality(DashComponents& aNextComponents,
                                     uint8_t& aQualityIndex,
                                     bool_t& aGlobal,
                                     DashRepresentation* aLatestRepresentation);
    virtual void_t doSwitchRepresentation();

private:
    void_t addRepresentationByQuality(DashComponents aDashComponents, DashRepresentationTile* aRepresentation);

private:
    Representations mRepresentationsByQuality;  // ordered by quality ranking, only references
    uint8_t mNrQualityLevels;
    TileRole::Enum mRole;

    /*Add each tile a flag to synchronize tile download*/
    uint64_t mTileFlag;
};
OMAF_NS_END
