
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
#include "NVRMediaCodecDecoderHW.h"


OMAF_NS_BEGIN

    struct UploadedFrame
    {
        DecoderFrame* frame;
        uint64_t pts;
        streamid_t stream;
    };

    class MediaCodecDecoderHW;
    class MediaCodecDecoder : public VideoDecoderManager
    {
    public:

        MediaCodecDecoder();
        virtual ~MediaCodecDecoder();

        virtual Error::Enum preloadTexturesForPTS(const Streams& baseStreams, const Streams& enhancementStreams, const Streams& skippedStreams, uint64_t targetPTSUs, bool_t isAuxiliary = false);
        virtual void_t activateDecoder(const streamid_t stream);
        virtual void_t deactivateStream(streamid_t stream);

        virtual Error::Enum uploadTexturesForPTS(const Streams& baseStreams, const Streams& enhancementStreams, uint64_t targetPTSUs, TextureLoadOutput& output, bool_t isAuxiliary = false);

        virtual void_t createVideoDecoders(VideoCodec::Enum codec, uint32_t decoderCount);

        virtual bool_t syncStreams(streamid_t anchorStream, streamid_t stream);

    protected:
        virtual VideoDecoderHW* reserveVideoDecoder(const DecoderConfig& config);
        virtual void_t releaseVideoDecoder(VideoDecoderHW* decoder);

    private:
        Error::Enum setupStreams(const Streams& streams);

        void_t removeStreamFromUploaded(streamid_t stream);

    private:

        typedef FixedArray<UploadedFrame, MAX_STREAM_COUNT> UploadedFrames;
        typedef FixedArray<MediaCodecDecoderHW*, MAX_STREAM_COUNT> FreeDecoders;
        Mutex mUploadedFramesMutex;
        UploadedFrames mUploadedFrames;
        FreeDecoders mFreeDecoders;
        Streams mAllUploadedStreams;

        Mutex mUploadedFramesMutexAuxiliary;
        UploadedFrames mUploadedFramesAuxiliary;
        Streams mUploadedStreamsAuxiliary;
    };


OMAF_NS_END