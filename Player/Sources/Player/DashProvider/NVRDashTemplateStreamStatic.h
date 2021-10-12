
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

#include "NVRErrorCodes.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "DashProvider/NVRDashSegmentStream.h"

OMAF_NS_BEGIN
class DashTemplateStreamStatic : public DashSegmentStream
{
public:
    DashTemplateStreamStatic(uint32_t bandwidth,
                             DashComponents dashComponents,
                             uint32_t aInitializationSegmentId,
                             DashSegmentStreamObserver* observer);

    virtual ~DashTemplateStreamStatic();

    virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs, uint32_t& aSeekSegmentIndex);
    virtual uint32_t calculateSegmentId(uint64_t& ptsUs);
    virtual bool_t hasFixedSegmentSize() const;

protected:
    virtual dash::mpd::ISegment* getNextSegment(uint32_t& segmentId);
    virtual void_t downloadAborted();
    virtual bool_t isLastSegment() const;


private:
    uint32_t mStartIndex;
    uint32_t mCurrentSegmentIndex;
    uint32_t mIndexForSeek;
    uint32_t mSegmentCount;
};
OMAF_NS_END
