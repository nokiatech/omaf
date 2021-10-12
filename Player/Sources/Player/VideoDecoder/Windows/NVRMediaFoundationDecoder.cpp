
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
#include "VideoDecoder/Windows/NVRMediaFoundationDecoder.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "VideoDecoder/Windows/NVRMediaFoundationDecoderHW.h"
OMAF_NS_BEGIN
OMAF_LOG_ZONE(MediaFoundationDecoder)
MediaFoundationDecoder::MediaFoundationDecoder()
{
    // changed to 2 when doing latency optimizations and when enabling decoder buffering check again
    mInitialBufferingThreshold = 2;	
}

MediaFoundationDecoder::~MediaFoundationDecoder()
{
    for (FreeDecoders::Iterator it = mFreeDecoders.begin(); it != mFreeDecoders.end(); ++it)
    {
        MediaFoundationDecoderHW* decoder = *it;
        decoder->deinitialize();
        decoder->destroyInstance();
        OMAF_DELETE_HEAP(decoder);
    }
}

VideoDecoderHW* MediaFoundationDecoder::reserveVideoDecoder(const DecoderConfig& config)
{
    uint32_t start = Time::getClockTimeMs();
    MediaFoundationDecoderHW* decoder = OMAF_NULL;
    Error::Enum initResult = Error::OK;
    for (FreeDecoders::Iterator it = mFreeDecoders.begin(); it != mFreeDecoders.end(); ++it)
    {
        const DecoderConfig& existingConfig = (*it)->getConfig();
        if (config.mimeType == existingConfig.mimeType && config.width == existingConfig.width &&
            config.height == existingConfig.height)
        {
            decoder = *it;
            mFreeDecoders.remove(it);
            decoder->setStreamId(config.streamId);
            break;
        }
    }
    if (decoder == OMAF_NULL)
    {
        decoder = OMAF_NEW_HEAP(MediaFoundationDecoderHW)(*mFrameCache);
        decoder->createInstance(config.mimeType);
        initResult = decoder->initialize(config);
    }
    if (initResult != Error::OK)
    {
        OMAF_DELETE_HEAP(decoder);
        return OMAF_NULL;
    }
    return decoder;
}

void_t MediaFoundationDecoder::releaseVideoDecoder(VideoDecoderHW* decoder)
{
    mFreeDecoders.add((MediaFoundationDecoderHW*) decoder);
}

OMAF_NS_END
