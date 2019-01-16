
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

#include "NVREssentials.h"

#include "VideoDecoder/NVRVideoDecoderHW.h"

#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include "Media/NVRMediaFormat.h"
#include "Media/NVRMediaPacket.h"
#include "NVRFrameCacheAndroid.h"

OMAF_NS_BEGIN

class CodecEntry;

struct DecoderFrameAndroid : public DecoderFrame
{
    ssize_t decoderOutputIndex;
};

class MediaCodecDecoderHW : public VideoDecoderHW
{
public:
    MediaCodecDecoderHW(FrameCache& frameCache);

    virtual ~MediaCodecDecoderHW();

    const MimeType& getMimeType() const;
    uint32_t getWidth() const;
    uint32_t getHeight() const;

    virtual Error::Enum createInstance(const MimeType& mimeType);
    virtual Error::Enum initialize(const DecoderConfig& config);
    virtual void_t flush();
    virtual void_t freeCodec();
    virtual void_t deinitialize();
    virtual void_t destroyInstance();

    virtual void setInputEOS();

    virtual bool_t isEOS();
    virtual DecodeResult::Enum decodeFrame(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking);

    virtual void_t consumedFrame(DecoderFrame* frame);

    void_t releaseOutputBuffer(DecoderFrame* frame, bool_t upload);

public:
    Error::Enum createOutputTexture();
    OutputTexture& getOutputTexture();

    Error::Enum configureCodec();

    DecoderHWState::Enum getState() const;

    Error::Enum fetchDecodedFrames();


private:
    OMAF_NO_COPY(MediaCodecDecoderHW);
    OMAF_NO_ASSIGN(MediaCodecDecoderHW);

private:
    Error::Enum startCodec();


private:
private:
    MimeType mMimeType;

    DecoderConfig mDecoderConfig;

    bool_t mInputEOS;
    DecoderHWState::Enum mState;

    FrameCache& mFrameCache;

    OutputTexture mOutputTexture;

    CodecEntry* mCodec;
};
OMAF_NS_END
