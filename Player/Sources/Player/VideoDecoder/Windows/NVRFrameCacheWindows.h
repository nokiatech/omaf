
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
#include "VideoDecoder/NVRFrameCache.h"

#if OMAF_GRAPHICS_API_D3D11
struct ID3D11Device;
#endif

OMAF_NS_BEGIN

class FrameCacheWindows : public FrameCache
{
public:

    FrameCacheWindows();
    virtual ~FrameCacheWindows();

    virtual void_t createTexture(streamid_t stream, const DecoderConfig& config);
    virtual void_t destroyTexture(streamid_t stream);

protected:
    virtual void_t uploadTexture(DecoderFrame* frame);

private:

    virtual DecoderFrame* createFrame(uint32_t width, uint32_t height);
    virtual void_t destroyFrame(DecoderFrame* frame);

private:
#if OMAF_GRAPHICS_API_OPENGL
    struct TextureHandles
    {
        TextureHandles() { Y = 0; UV = 0; }
        uint32_t Y;
        uint32_t UV;
    };
    typedef FixedArray<TextureHandles, MAX_STREAM_COUNT> TextureHandleList;

    TextureHandleList mTextureHandles;
#endif
#if OMAF_GRAPHICS_API_D3D11
    ID3D11Device* mDecoderDevice;
#endif
};

OMAF_NS_END
