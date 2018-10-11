
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
        virtual uint8_t getNrQualityLevels() const;

    public: // new

        virtual bool_t processSegmentDownloadTile(uint32_t aNextSegmentId, bool_t aCanSwitchRepresentations);
        virtual bool_t selectQuality(uint8_t aQualityLevel, uint8_t aNrQualityLevels, uint32_t aStartingFromSegment);
        // called by extractor for supporting sets if dependencyId based collection of tiles is used
        virtual bool_t selectRepresentation(DependableRepresentations& aDependencies, uint32_t aNextNeededSegment);
        virtual DashRepresentation* getRepresentationForQuality(uint8_t aQualityLevel, uint8_t aNrLevels);

        // called by extractor for supporting sets if dependencyId based collection of tiles is used
        virtual bool_t prepareForSwitch(uint32_t aNextNeededSegment, bool_t aGoToBackground);
        virtual bool_t readyToSwitch(uint32_t aNextNeededSegment);
        virtual void_t setBufferingTime(uint32_t aExpectedPingTimeMs);

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
