
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

#include "NVRNamespace.h"
#include "NVRErrorCodes.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRArray.h"

#include "DashProvider/NVRDashSegmentStream.h"

OMAF_NS_BEGIN
    class DashTimelineStreamDynamic : public DashSegmentStream
    {
    public:

        DashTimelineStreamDynamic(uint32_t bandwidth,
                                  DashComponents dashComponents,
                                  uint32_t aInitializationSegmentId,
                                  DashSegmentStreamObserver *observer);

        virtual ~DashTimelineStreamDynamic();

        virtual bool_t mpdUpdateRequired();
        virtual Error::Enum updateMPD(DashComponents components);
        virtual uint32_t calculateSegmentId(uint64_t ptsUs);
    protected:
        virtual void_t downloadStopped();
        virtual bool_t waitForStreamHead();
        virtual dash::mpd::ISegment*  getNextSegment(uint32_t &segmentId);
        virtual void_t downloadAborted();

    private:

        uint32_t initializeDynamicSegmentIndex();
        uint32_t getSegmentIndexForTime(time_t time);

    private:
        Array<DashTimelineSegment> mSegments;
        uint32_t mCurrentSegmentIndex;
        uint32_t mPrevCheckedIndex;

        time_t mAvailabilityStartTimeUTCSec;
        uint64_t mPeriodStartTimeMS;
        uint64_t mPresentationTimeOffset;
        
    };
OMAF_NS_END
