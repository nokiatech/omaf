
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

#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRFixedQueue.h"
#include "NVRNamespace.h"
#include "Provider/NVRCoreProviderSources.h"
#include "VideoDecoder/NVRVideoDecoderFrame.h"

OMAF_NS_BEGIN

static const size_t MAX_STREAM_COUNT = 128;
static const size_t MAX_FRAME_COUNT = 512;
static const uint64_t INVALID_DISCARD_TARGET = OMAF_UINT64_MAX;
static const size_t INVALID_STREAM_INDEX = OMAF_UINT32_MAX;

typedef FixedQueue<DecoderFrame*, MAX_FRAME_COUNT> FrameQueue;
typedef FixedArray<DecoderFrame*, MAX_FRAME_COUNT> FrameList;
typedef FixedArray<VideoFrame, MAX_STREAM_COUNT> VideoFrames;

namespace DecodeResult
{
    enum Enum
    {
        INVALID,

        NOT_READY,
        PACKET_ACCEPTED,
        DECODER_FULL,
        DECODER_ERROR,

        COUNT
    };
}

OMAF_NS_END
