
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

#include "DashProvider/NVRDashAdaptationSetSubPicture.h"


OMAF_NS_BEGIN

    class DashRepresentationTile;


    class DashAdaptationSetTile : public DashAdaptationSetSubPicture
    {
    public:
        DashAdaptationSetTile(DashAdaptationSetObserver& observer);

        virtual ~DashAdaptationSetTile();

        static bool_t isTile(DashComponents aDashComponents, AdaptationSetBundleIds& aSupportingIds);
        static bool_t isTile(DashComponents aDashComponents, RepresentationDependencies& aDependingRepresentations);

        virtual AdaptationSetType::Enum getType() const;

    public: // from DashAdaptationSet
        virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
        virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId, uint32_t& aAdaptationSetId);
        virtual Error::Enum stopDownload();
        virtual uint8_t getNrQualityLevels() const;

    public: // new

        virtual bool_t processSegmentDownload();
        virtual bool_t trySwitchingRepresentation(uint32_t aNextSegmentId);
        virtual bool_t selectQuality(uint8_t aQualityLevel, uint8_t aNrQualityLevels, uint32_t aNextProcessedSegment);
        // called by extractor for supporting sets if dependencyId based collection of tiles is used
        virtual bool_t selectRepresentation(DependableRepresentations& aDependencies, uint32_t aNextProcessedSegment);
        virtual DashRepresentation* getRepresentationForQuality(uint8_t aQualityLevel, uint8_t aNrLevels);

        // called by extractor for supporting sets if dependencyId based collection of tiles is used, hence public; otherwise called by the tile itself
        virtual bool_t prepareForSwitch(uint32_t aNextProcessedSegment, bool_t aGoToBackground);
        virtual void_t cleanUpOldSegments(uint32_t aNextSegmentId);

        virtual void_t setBufferingTime(uint32_t aExpectedPingTimeMs);

        DashSegment* getSegment(uint32_t aSegmentId);
        bool_t hasSegment(uint32_t aSegmentId, size_t& aSegmentSize);

    protected: // from DashAdaptationSet
        virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
        virtual DashRepresentation* createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth);
        virtual bool_t parseVideoProperties(DashComponents& aNextComponents);
        virtual void_t parseVideoViewport(DashComponents& aNextComponents);
        virtual bool_t parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityIndex, bool_t& aGlobal, DashRepresentation* aLatestRepresentation);
        virtual void_t doSwitchRepresentation();

    private: 
        void_t addRepresentationByQuality(DashComponents aDashComponents, DashRepresentationTile* aRepresentation);

    private: 
        Representations mRepresentationsByQuality; // ordered by quality ranking, only references
        uint8_t mNrQualityLevels;

    };
OMAF_NS_END
