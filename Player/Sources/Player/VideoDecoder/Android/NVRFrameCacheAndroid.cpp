
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
#include <NVRGraphics.h>
#include "VideoDecoder/Android/NVRFrameCacheAndroid.h"
#include "VideoDecoder/NVRVideoDecoderFrame.h"
#include "NVRMediaCodecDecoderHW.h"
#include "Foundation/NVRLogger.h"

#if OMAF_GRAPHICS_API_FAMILY_OPENGL
#include "Graphics/OpenGL/Windows/NVRGLFunctions.h"
#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(FrameCacheMacOS);
FrameCacheAndroid::FrameCacheAndroid()
{
}

FrameCacheAndroid::~FrameCacheAndroid()
{
    destroyInstance();
}

DecoderFrame* FrameCacheAndroid::createFrame(uint32_t width, uint32_t height)
{
    DecoderFrameAndroid* frame = OMAF_NEW_HEAP(DecoderFrameAndroid);
    frame->width = width;
    frame->height = height;
    frame->decoderOutputIndex = 0;
    return frame;
}

void_t FrameCacheAndroid::destroyFrame(DecoderFrame *frame)
{
    OMAF_DELETE_HEAP(frame);
}


void_t FrameCacheAndroid::uploadTexture(DecoderFrame* frame)
{
    MediaCodecDecoderHW* decoder = (MediaCodecDecoderHW*)frame->decoder;
    if (decoder == OMAF_NULL)
    {
        OMAF_LOG_E("no decoder for a frame!!");
    }
    OMAF_ASSERT(decoder != OMAF_NULL, "Null decoder in upload texture");

    OutputTexture& textureHandle = decoder->getOutputTexture();
    VideoFrame& videoFrame = mCurrentVideoFrames.at(frame->streamId);
    OMAF_ASSERT(textureHandle.surface != OMAF_NULL, "No output texture");

    SurfaceTexture *surfaceTexture = textureHandle.surface->getSurfaceTexture();
    OMAF_ASSERT(surfaceTexture != OMAF_NULL, "No output texture");

    if (surfaceTexture != NULL)
    {
        surfaceTexture->update();

        releaseVideoFrame(videoFrame);

        unsigned int texture = surfaceTexture->getTexture();
        const float* stMatrix = surfaceTexture->getTransformMatrix();

        NativeTextureDesc textureDesc;
        textureDesc.type = TextureType::TEXTURE_VIDEO_SURFACE;
        textureDesc.width = frame->width;
        textureDesc.height = frame->height;
        textureDesc.numMipMaps = 1;
        textureDesc.format = TextureFormat::RGBA8;
        textureDesc.nativeHandle = &texture;
        videoFrame.textures[0] = RenderBackend::createNativeTexture(textureDesc);

        videoFrame.format = VideoPixelFormat::RGBA;
        videoFrame.numTextures = 1;
        videoFrame.width = frame->width;
        videoFrame.height = frame->height;
        videoFrame.pts = frame->pts;

        MemoryCopy(&videoFrame.matrix, stMatrix, 16 * sizeof(float));
    }
}

void_t FrameCacheAndroid::createTexture(streamid_t stream, const DecoderConfig& config)
{
    // Do nothing here on Android since the SurfaceTexture is created in the decoder itself
}

void_t FrameCacheAndroid::destroyTexture(streamid_t stream)
{
    VideoFrame& frame = mCurrentVideoFrames.at(stream);
    releaseVideoFrame(frame);
}


OMAF_NS_END
