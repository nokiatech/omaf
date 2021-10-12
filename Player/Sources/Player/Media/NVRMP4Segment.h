
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

#include "Foundation/NVRDataBuffer.h"
#include "Foundation/NVRFixedString.h"
#include "NVRMediaType.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
typedef FixedString1024 RepresentationId;

struct SegmentContent
{
    // this
    MediaContent type;
    RepresentationId representationId;
    uint32_t initializationSegmentId;

    // what this depends on
    RepresentationId associatedToRepresentationId;
    const char_t* associationType;
    uint32_t associatedToInitializationSegmentId;
};

class MP4Segment
{
public:
    virtual ~MP4Segment(){};

    virtual DataBuffer<uint8_t>* getDataBuffer() = 0;

    virtual bool_t isInitSegment() const = 0;

    virtual uint32_t getSegmentId() const = 0;

    virtual uint32_t getInitSegmentId() const = 0;

    virtual bool_t isSubSegment() const = 0;

    virtual uint32_t rangeStartByte() const = 0;

    virtual bool_t getSegmentContent(SegmentContent& segmentContents) const = 0;

    virtual uint32_t getTimestampBase() const = 0;
};
OMAF_NS_END
