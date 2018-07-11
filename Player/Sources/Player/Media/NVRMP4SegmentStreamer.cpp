
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
#include "Media/NVRMP4SegmentStreamer.h"

OMAF_NS_BEGIN
    MP4SegmentStreamer::MP4SegmentStreamer(MP4Segment* segment)
        : mSegment(segment)
        , mReadIndex(0)
        , mId(0)
    {
        mId=mSegment->getSegmentId();
        mIsInitSegment = mSegment->isInitSegment();
    }

    MP4SegmentStreamer::~MP4SegmentStreamer()
    {        
        if (!mIsInitSegment) //If not initsegment, initsegment is deleted elsewhere!
        {
            OMAF_DELETE_HEAP(mSegment);
        }
    }

    uint32_t MP4SegmentStreamer::getInitSegmentId()
    {
        return mSegment->getInitSegmentId();
    }

    uint32_t MP4SegmentStreamer::getSegmentId()
    {
        return mSegment->getSegmentId();
    }

    MP4VR::StreamInterface::offset_t MP4SegmentStreamer::read(char *buffer, MP4VR::StreamInterface::offset_t size)
    {
        if (mReadIndex + size >= mSegment->getDataBuffer()->getSize())
        {
            size = (MP4VR::StreamInterface::offset_t)(mSegment->getDataBuffer()->getSize() - mReadIndex);
        }

        MemoryCopy(buffer, mSegment->getDataBuffer()->getDataPtr() + mReadIndex, size);
        mReadIndex += size;

        return size;
    }

    bool MP4SegmentStreamer::absoluteSeek(MP4VR::StreamInterface::offset_t offset)
    {
        if (offset <= static_cast<MP4VR::StreamInterface::offset_t>(mSegment->getDataBuffer()->getSize()))
        {
            mReadIndex = offset;
            return true;
        }
        else
        {
            return false;
        }
    }

    MP4VR::StreamInterface::offset_t MP4SegmentStreamer::tell()
    {
        return (MP4VR::StreamInterface::offset_t)mReadIndex;
    }

    MP4VR::StreamInterface::offset_t MP4SegmentStreamer::size()
    {
        return (MP4VR::StreamInterface::offset_t)mSegment->getDataBuffer()->getSize();
    }
OMAF_NS_END
