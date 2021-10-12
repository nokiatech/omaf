
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

#include <reader/mp4vrfilestreaminterface.h>
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "DashProvider/NVRDashMediaSegment.h"

#include "NVRErrorCodes.h"

OMAF_NS_BEGIN

class MP4VRMediaPacket;

class MP4SegmentStreamer : public MP4VR::StreamInterface
{
public:
    MP4SegmentStreamer(MP4Segment* segment);
    virtual ~MP4SegmentStreamer();


    uint32_t getInitSegmentId();
    uint32_t getSegmentId();

    offset_t read(char* buffer, offset_t size);

    bool absoluteSeek(offset_t offset);

    offset_t tell();

    offset_t size();

private:
    MP4Segment* mSegment;
    size_t mReadIndex;
    uint32_t mId;
    bool_t mIsInitSegment;
};

OMAF_NS_END
