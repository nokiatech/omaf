
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

#include "DashProvider/NVRDashAdaptationSetExtractor.h"


OMAF_NS_BEGIN

    class DashAdaptationSetExtractorDepId : public DashAdaptationSetExtractor
    {
    public:
        DashAdaptationSetExtractorDepId(DashAdaptationSetObserver& observer);

        virtual ~DashAdaptationSetExtractorDepId();

    public: // from DashAdaptationSet
        virtual AdaptationSetType::Enum getType() const;
    
        virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);

        virtual const FixedArray<VASTileViewport*, 32> getCoveredViewports() const;

        virtual uint32_t peekNextSegmentId() const;
        virtual bool_t processSegmentDownload();
    
    public: // new
        virtual const RepresentationDependencies& getDependingRepresentations() const;
        virtual bool_t selectRepresentation(const VASTileViewport* aTile, uint32_t aNextNeededSegment);

    protected: // from DashAdaptationSet
        virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
        virtual DashRepresentation* createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth);
        virtual void_t doSwitchRepresentation();
        virtual bool_t parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityIndex, bool_t& aGlobal, DashRepresentation* aLatestRepresentation);

    protected: // new member variables

        // relevant with extractor types only; collects the adaptation sets that this set depends on
        RepresentationDependencies mDependingRepresentationIds;


    };
OMAF_NS_END
