
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

#include "Media/NVRMP4Segment.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
class DashSegmentObserver
{
public:
    virtual void_t onSegmentDeleted() = 0;
};

class DashSegment : public MP4Segment
{
public:
    DashSegment(size_t bufferSize,
                uint32_t segmentId,
                uint32_t initSegmentId,
                bool_t aIsInitSegment,
                DashSegmentObserver* observer);
    DashSegment(int64_t rangeStart,
                size_t bufferSize,
                uint32_t segmentId,
                uint32_t initSegmentId,
                bool_t aIsInitSegment,
                DashSegmentObserver* observer);
    virtual ~DashSegment();

public:  // from MP4Segment
    DataBuffer<uint8_t>* getDataBuffer();

    bool_t isInitSegment() const;

    uint32_t getSegmentId() const;

    uint32_t getInitSegmentId() const;

    bool_t isSubSegment() const;

    uint32_t rangeStartByte() const;

    bool_t getSegmentContent(SegmentContent& segmentContent) const;

    uint32_t getTimestampBase() const;

public:  // new
    void setSegmentContent(const SegmentContent& segmentContent);

    void_t setIsCached();

    bool_t addSrcSegment(DashSegment* aSrcSegment);

    void_t setTimestampBase(uint32_t aTimestampBaseMs);

protected:
    DataBuffer<uint8_t>* mData;
    uint32_t mSegmentId;      // this media segments id
    uint32_t mInitSegmentId;  // to which initialization id this segment belongs to
    DashSegmentObserver* mObserver;
    SegmentContent mSegmentContent;
    bool_t mIsSubSegment;   // whether this is subsegment (true) or full segment (false)
    bool_t mIsInitSegment;  // whether this is initialization segment
    uint32_t mStartByte;    // in case this is subsegment

    bool_t mCached;
    uint32_t mTimestampBase;

    FixedArray<DashSegment*, 128> mLinkedSegments;
};
OMAF_NS_END
