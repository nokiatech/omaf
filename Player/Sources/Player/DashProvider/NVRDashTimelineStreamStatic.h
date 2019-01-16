
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
#include "Foundation/NVRArray.h"

#include "DashProvider/NVRDashSegmentStream.h"

OMAF_NS_BEGIN
    class DashTimelineStreamStatic : public DashSegmentStream
    {
    public:

        DashTimelineStreamStatic(uint32_t bandwidth,
                                 DashComponents dashComponents,
                                 uint32_t aInitializationSegmentId,
                                 DashSegmentStreamObserver *observer);

        virtual ~DashTimelineStreamStatic();

        virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);
        virtual uint32_t calculateSegmentId(uint64_t ptsUs);

    protected:
        virtual dash::mpd::ISegment*  getNextSegment(uint32_t &segmentId);
        virtual void_t downloadAborted();

    private:
        Array<DashTimelineSegment> mSegments;
        uint64_t mCurrentSegmentIndex;
        uint64_t mStartNumber;

        uint64_t mIndexForSeek;
        time_t mAvailabilityStartTimeUTCSec;

    };
OMAF_NS_END
