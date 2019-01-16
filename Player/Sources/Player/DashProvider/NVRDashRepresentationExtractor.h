
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

#include "DashProvider/NVRDashRepresentationTile.h"

OMAF_NS_BEGIN

    class DashRepresentationExtractor : public DashRepresentationTile
    {
    public:
        
        DashRepresentationExtractor();
        virtual ~DashRepresentationExtractor();

        virtual void_t createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel);

        bool_t isDone(uint32_t& aSegmentId);
        bool_t readyForSegment(uint32_t aId);
        Error::Enum parseConcatenatedMediaSegment(DashSegment *aSegment);

        void_t setCoveredViewport(VASTileViewport* aCoveredViewport);
        VASTileViewport* getCoveredViewport();

    private:
        sourceid_t mSourceId;
        SourceType::Enum mSourceType;
        StereoRole::Enum mRole;

        VASTileViewport* mCoveredViewport;
    };
OMAF_NS_END
