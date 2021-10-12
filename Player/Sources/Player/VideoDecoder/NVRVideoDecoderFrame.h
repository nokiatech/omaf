
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
#include "Provider/NVRCoreProvider.h"

OMAF_NS_BEGIN

class VideoDecoderHW;

struct DecoderFrame
{
    DecoderFrame()
        : width(0)
        , height(0)
        , pts(0)
        , dts(0)
        , duration(0)
        , uploadTime(0)
        , flushed(false)
        , consumed(false)
        , staged(false)
        , streamId(OMAF_UINT8_MAX)
        , decoder(OMAF_NULL)
    {
    }

    uint32_t width;
    uint32_t height;
    uint64_t pts;
    uint64_t dts;
    uint64_t duration;
    uint64_t uploadTime;
    bool_t flushed;
    bool_t consumed;
    bool_t staged;
    streamid_t streamId;
    VideoDecoderHW* decoder;
    VideoColorInfo colorInfo;
};

OMAF_NS_END
