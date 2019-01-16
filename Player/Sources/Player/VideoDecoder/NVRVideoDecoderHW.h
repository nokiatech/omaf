
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"
#include "VideoDecoder/NVRVideoDecoderConfig.h"
#include "VideoDecoder/NVRVideoDecoderTypes.h"
#include "VideoDecoder/NVRFrameCache.h"

OMAF_NS_BEGIN

struct DecoderFrame;

namespace DecoderHWState
{
    enum Enum
    {
        INVALID = -1,

        IDLE,
        INITIALIZED,
        CONFIGURED,
        STARTED,
        EOS,
        ERROR_STATE,

        COUNT

    };
}

class VideoDecoderHW
{

public:

    virtual ~VideoDecoderHW() {};

    // Creates the HW instance
    virtual Error::Enum createInstance(const MimeType& mimeType) = 0;

    // Initializes the decoder to be ready to receive packets
    virtual Error::Enum initialize(const DecoderConfig& config) = 0;

    // Flushes the current input and output of the decoder
    virtual void_t flush() = 0;

    // Deinitializes the instance
    virtual void_t deinitialize() = 0;

    // Destroys the decoder HW instance
    virtual void_t destroyInstance() = 0;

    // Tells the decoder to not expect more packets (unless flushed)
    virtual void setInputEOS() = 0;

    // Returns if the decoder has reached end of stream
    virtual bool_t isEOS() = 0;

    // Add packets to the decoder
    virtual DecodeResult::Enum decodeFrame(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking) = 0;

    // Called when a decoded frame has been consumed, needed mainly for Android
    virtual void_t consumedFrame(DecoderFrame* frame) {};

};

OMAF_NS_END
