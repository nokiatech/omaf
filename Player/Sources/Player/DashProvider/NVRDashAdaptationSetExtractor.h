
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

#include "DashProvider/NVRDashAdaptationSetTile.h"


OMAF_NS_BEGIN

    class DashAdaptationSetExtractor : public DashAdaptationSetTile
    {
    public:
        DashAdaptationSetExtractor(DashAdaptationSetObserver& observer);

        virtual ~DashAdaptationSetExtractor();

        static bool_t isExtractor(DashComponents aDashComponents, uint32_t& aAdaptationSetId);
        static SupportingAdaptationSetIds hasPreselection(DashComponents aDashComponents, uint32_t aAdaptationSetId);
        static RepresentationDependencies hasDependencies(DashComponents aDashComponents);
        
    public: // from DashAdaptationSet
        virtual AdaptationSetType::Enum getType() const;

        virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);

        virtual Error::Enum stopDownload();
        virtual Error::Enum stopDownloadAsync(bool_t aReset);
        virtual void_t clearDownloadedContent();
        virtual bool_t processSegmentDownload();
        virtual uint32_t getCurrentBandwidth() const;
        virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);
        virtual bool_t isBuffering();

    public: // new
        virtual bool_t updateBitrate(size_t aNrForegroundTiles, uint8_t aQualityForeground, uint8_t aQualityBackground, uint8_t aNrLevels);
        virtual const SupportingAdaptationSetIds& getSupportingSets() const;
        virtual void_t addSupportingSet(DashAdaptationSetTile* aSupportingSet);

    protected: // from DashAdaptationSet
        virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
        virtual DashRepresentation* createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth);
        virtual void_t parseVideoViewport(DashComponents& aNextComponents);

    protected:  // new
        virtual void_t concatenateAndParseSegments();

    protected: // new member variables

        // relevant with extractor types only; collects the adaptation sets that this set depends on
        SupportingAdaptationSetIds mSupportingSetIds;
        typedef FixedArray<DashAdaptationSetTile*, 128> SupportingAdaptationSets;
        SupportingAdaptationSets mSupportingSets;

        uint32_t mNextSegmentToBeConcatenated;

    };
OMAF_NS_END
