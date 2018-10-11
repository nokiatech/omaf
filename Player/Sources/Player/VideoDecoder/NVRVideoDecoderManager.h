
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

#include "NVRNamespace.h"
#include "Media/NVRMediaPacket.h"
#include "VideoDecoder/NVRVideoDecoderConfig.h"
#include "VideoDecoder/NVRVideoDecoderTypes.h"
#include "Media/NVRMediaInformation.h"

OMAF_NS_BEGIN

class VideoDecoderHW;
class FrameCache;

struct DecoderState
{
    DecoderState() 
    {
        decoderActive = false;
        frameCacheActive = false;
        textureActive = false;
        decoderHW = OMAF_NULL;
        initialBufferingDone = false;
        free = true;
    }

    bool_t decoderActive;
    bool_t frameCacheActive;
    bool_t textureActive;
    bool_t initialBufferingDone;
    bool_t free;
    VideoDecoderHW* decoderHW;
    DecoderConfig config;
};

struct TextureLoadOutput
{
    Streams updatedStreams;
    uint64_t pts;
    uint64_t frameDuration;
};

class VideoDecoderManager
{

public: // Singleton

    static VideoDecoderManager* getInstance();
    static void_t destroyInstance();

    virtual ~VideoDecoderManager();

public:
    // Virtuals
    // Called from the provider thread
    virtual Error::Enum initializeStream(streamid_t stream, const DecoderConfig& config);
    virtual void_t shutdownStream(streamid_t stream);

    // Can be used to precreate video decoders as a performance optimization
    virtual void_t createVideoDecoders(VideoCodec::Enum codec, uint32_t decoderCount) {};

    virtual Error::Enum activateStream(streamid_t stream, uint64_t currentPTS);
    virtual void_t deactivateStream(streamid_t stream);

    virtual Error::Enum preloadTexturesForPTS(
        const Streams& baseStreams, const Streams& enhancementStreams, const Streams& skippedStreams,
        uint64_t targetPTSUs, bool_t isAuxiliary = false);

    // Called from the rendering thread
    virtual void_t activateDecoder(const streamid_t stream);
    virtual Error::Enum uploadTexturesForPTS(
        const Streams& baseStreams, const Streams& enhancementStreams,
        uint64_t targetPTSUs, TextureLoadOutput& output, bool_t isAuxiliary = false);

public:

    // Called from the provider thread
    streamid_t generateUniqueStreamID();
    streamid_t getSharedStreamID();
    DecodeResult::Enum decodeMediaPacket(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking = false);
    void_t seekToPTS(const Streams& streams, uint64_t seekTargetPTS);
    bool_t isBuffering(const Streams& streams);
    void_t flushStream(streamid_t stream);
    bool_t isEOS(streamid_t stream);
    void_t setInputEOS(streamid_t stream);
    bool_t isByteStreamHeadersMode() { return mByteStreamHeadersMode; };

    bool_t isActive(const Streams& streams);


    // Called from the rendering thread
    const VideoFrame& getCurrentVideoFrame(streamid_t stream);

    virtual bool_t syncStreams(streamid_t anchorStream, streamid_t stream);

    // Called from the provider thread
    uint64_t getLatestPTS();

protected:
    VideoDecoderManager();

    virtual VideoDecoderHW* reserveVideoDecoder(const DecoderConfig& config) = 0;
    virtual void_t releaseVideoDecoder(VideoDecoderHW* decoder) = 0;

protected:
    FrameCache* mFrameCache;

    typedef FixedArray<DecoderState, MAX_STREAM_COUNT> DecoderStateList;


    DecoderStateList mDecoders;

    size_t mInitialBufferingThreshold;
    bool_t mByteStreamHeadersMode;

    uint64_t mLatestPts;

private:
    static VideoDecoderManager* sInstance;
    streamid_t mNextStreamID;
    streamid_t mSharedStreamID;
};

OMAF_NS_END
