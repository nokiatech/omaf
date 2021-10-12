
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

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "DashProvider/NVRDashSegmentStream.h"

OMAF_NS_BEGIN
class DashTemplateStreamDynamic : public DashSegmentStream
{
public:
    DashTemplateStreamDynamic(uint32_t bandwidth,
                              DashComponents dashComponents,
                              uint32_t aInitializationSegmentId,
                              DashSegmentStreamObserver *observer);

    virtual ~DashTemplateStreamDynamic();

    virtual uint32_t calculateSegmentId(uint64_t &ptsUs);
    virtual bool_t hasFixedSegmentSize() const;

protected:
    virtual void_t downloadStopped();

    virtual bool_t waitForStreamHead();
    virtual dash::mpd::ISegment *getNextSegment(uint32_t &segmentId);
    virtual void_t downloadAborted();

private:
    uint32_t initializeSegmentIndex();
    uint32_t getSegmentIndexForTime(time_t time, bool_t printLog);


private:
    uint32_t mStartIndex;
    uint32_t mCurrentSegmentIndex;

    uint32_t mMinDelayInSegments;
    uint32_t mMinPlaybackDelayMs;
    uint32_t mMaxBufferSizeMs;

    time_t mAvailabilityStartTimeUTCSec;
};
OMAF_NS_END
