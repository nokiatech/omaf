
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
#include "DashProvider/NVRDashMediaSegment.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemorySystem.h"

#include <IMPD.h>
#include <string>

OMAF_LOG_ZONE(DashSegment)

OMAF_NS_BEGIN

    DashSegment::DashSegment(size_t bufferSize, uint32_t segmentId, uint32_t initSegmentId, bool_t aIsInitSegment, DashSegmentObserver *observer)
        : mData(OMAF_NEW(*MemorySystem::DefaultHeapAllocator(), DataBuffer<uint8_t>)(*MemorySystem::DefaultHeapAllocator(), bufferSize))
        , mSegmentId(segmentId)
        , mInitSegmentId(initSegmentId)
        , mObserver(observer)
        , mIsSubSegment(false)
        , mIsInitSegment(aIsInitSegment)
        , mStartByte(0)
        , mSegmentContent()
        , mCached(false)
    {
    }

    DashSegment::DashSegment(int64_t rangeStart, size_t bufferSize, uint32_t segmentId, uint32_t initSegmentId, bool_t aIsInitSegment, DashSegmentObserver *observer)
        : mData(OMAF_NEW(*MemorySystem::DefaultHeapAllocator(), DataBuffer<uint8_t>)(*MemorySystem::DefaultHeapAllocator(), bufferSize))
        , mSegmentId(segmentId)
        , mInitSegmentId(initSegmentId)
        , mObserver(observer)
        , mIsSubSegment(true)
        , mIsInitSegment(aIsInitSegment)
        , mStartByte(rangeStart)
        , mSegmentContent()
        , mCached(false)
    {
    }

    DashSegment::~DashSegment()
    {
        if (mLinkedSegments.getSize() > 0)
        {
            while (mLinkedSegments.getSize() > 0)
            {
                DashSegment* s = mLinkedSegments.at(0);
                mLinkedSegments.removeAt(0);
                OMAF_DELETE(*MemorySystem::DefaultHeapAllocator(), s);
                OMAF_LOG_V("deleted linked segment");
            }
        }
        if (mObserver != OMAF_NULL && mCached)
        {
            mObserver->onSegmentDeleted();
        }
        OMAF_DELETE(mData->getAllocator(), mData);
    }

    uint32_t DashSegment::getSegmentId() const
    {
        return mSegmentId;
    }

    uint32_t DashSegment::getInitSegmentId() const
    {
        return mInitSegmentId;
    }

    bool_t DashSegment::isSubSegment() const
    {
        return mIsSubSegment;
    }

    uint32_t DashSegment::rangeStartByte() const
    {
        return mStartByte;
    }

    bool_t DashSegment::getSegmentContent(SegmentContent& segmentContent) const
    {
        segmentContent = mSegmentContent;
        return true;
    }

    void DashSegment::setSegmentContent(const SegmentContent& segmentContent)
    {
        mSegmentContent = segmentContent;
    }

    DataBuffer<uint8_t>* DashSegment::getDataBuffer()
    {
        return mData;
    }

    bool_t DashSegment::isInitSegment() const
    {
        return mIsInitSegment;
    }

    void_t DashSegment::setIsCached()
    {
        mCached = true;
    }

    bool_t DashSegment::addSrcSegment(DashSegment* aSrcSegment)
    {
        OMAF_LOG_V("AddSrcSegment");
        mLinkedSegments.add(aSrcSegment);
        return true;
    }

OMAF_NS_END
