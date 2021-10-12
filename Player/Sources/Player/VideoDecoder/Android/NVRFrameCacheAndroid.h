
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
#include "VideoDecoder/Android/NVRSurface.h"
#include "VideoDecoder/NVRFrameCache.h"

OMAF_NS_BEGIN

struct OutputTexture
{
    OutputTexture()
    {
        surface = OMAF_NULL;
        nativeWindow = OMAF_NULL;
    }
    Surface* surface;
    ANativeWindow* nativeWindow;
};

class FrameCacheAndroid : public FrameCache
{
public:
    FrameCacheAndroid();
    virtual ~FrameCacheAndroid();

    virtual void_t createTexture(streamid_t stream, const DecoderConfig& config);
    virtual void_t destroyTexture(streamid_t stream);

public:  // Android specific
protected:
    virtual void_t uploadTexture(DecoderFrame* frame);


    virtual DecoderFrame* createFrame(uint32_t width, uint32_t height);
    virtual void_t destroyFrame(DecoderFrame* frame);
};

OMAF_NS_END
