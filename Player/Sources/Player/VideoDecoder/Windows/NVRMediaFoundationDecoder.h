
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
#pragma once
#include "VideoDecoder/NVRVideoDecoderManager.h"

OMAF_NS_BEGIN

class MediaFoundationDecoderHW;
class MediaFoundationDecoder : public VideoDecoderManager
{
public:

    MediaFoundationDecoder();
    ~MediaFoundationDecoder();

protected:
    virtual VideoDecoderHW* reserveVideoDecoder(const DecoderConfig& config);
    virtual void_t releaseVideoDecoder(VideoDecoderHW* decoder);


private:
    typedef FixedArray<MediaFoundationDecoderHW*, MAX_STREAM_COUNT> FreeDecoders;
    FreeDecoders mFreeDecoders;


};


OMAF_NS_END
