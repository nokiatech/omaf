
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

class MP4VRParser;


class DashSegmentStreamOnDemand : public DashSegmentStream
{
public:
    DashSegmentStreamOnDemand(uint32_t bandwidth,
                              DashComponents dashComponents,
                              uint32_t aInitializationSegmentId,
                              DashSegmentStreamObserver* observer);

    virtual ~DashSegmentStreamOnDemand();

    virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs, uint32_t& aSeekSegmentIndex);
    virtual uint32_t calculateSegmentId(uint64_t& ptsUs);
    virtual bool_t hasFixedSegmentSize() const;
    virtual bool_t setLatencyReq(LatencyRequirement::Enum aType);
    virtual void_t setLoopingOn();

protected:
    virtual dash::mpd::ISegment* getNextSegment(uint32_t& segmentId);
    virtual void_t downloadAborted();
    virtual bool_t isLastSegment() const;

    virtual void processUninitialized();
    virtual void processDownloadingInitSegment();
    virtual void processIdleOrRetry();
    virtual bool_t needInitSegment(bool_t aIsIndependent);

    virtual void_t handleDownloadedSegment(int64_t aDownloadTimeMs, size_t aBytesDownloaded);

private:
    bool_t doLoop(uint32_t& aTimestampBase);

private:
    uint32_t mStartIndex;
    uint32_t mCurrentSegmentIndex;
    uint32_t mIndexForSeek;
    uint32_t mSegmentCount;

    uint32_t mInitSegmentSize;

    Url mFullBaseUrl;
    MP4VRParser* mSidxParser;  // only for segment index parsing
    bool_t mLoopingOn;
    uint32_t mLoopedSegmentIndex;
    uint32_t mNrSegmentsMerged;
    uint32_t mNrSegmentsMergedTarget;
    LatencyRequirement::Enum mLatencyReq;

    uint64_t mOverridePositionUs;
};
OMAF_NS_END
