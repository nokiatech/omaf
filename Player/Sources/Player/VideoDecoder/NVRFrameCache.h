
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

#include "VideoDecoder/NVRVideoDecoderConfig.h"
#include "VideoDecoder/NVRVideoDecoderTypes.h"
#include "VideoDecoder/NVRVideoDecoderFrame.h"
#include "VideoDecoder/NVRFreeFrameGroup.h"

#include "Foundation/NVRMutex.h"

OMAF_NS_BEGIN

static const Matrix44 FLIPPED_TEXTURE_MATRIX = makeTranslation(0, 1, 0) * makeScale(1, -1, 1);

class VideoDecoderHW;
class DecodedFrameGroup;


class FrameCache
{

public:

    static FrameCache* createFrameCache();

    virtual ~FrameCache();

public:
    
    Error::Enum initializeStream(streamid_t stream, const DecoderConfig& config);
    void_t shutdownStream(streamid_t stream);

    virtual Error::Enum activateStream(streamid_t stream);
    void_t deactivateStream(streamid_t stream);

    // Called by the decoder
    virtual DecoderFrame* getFreeFrame(streamid_t stream);
    size_t getFreeFrameCount(streamid_t stream) const;

    void_t addDecodedFrame(DecoderFrame* decodedFrame);

    virtual void_t releaseFrame(DecoderFrame* frame);

public: // Called from rendering thread

    const VideoFrame& getCurrentVideoFrame(streamid_t stream) const;

    virtual void_t createTexture(streamid_t stream, const DecoderConfig& config) = 0;
    virtual void_t destroyTexture(streamid_t stream) = 0;

    Error::Enum getSynchedFramesForPTS(const Streams& reqStreams, const Streams& enhancementStreams, uint64_t targetPTSUS, FrameList& frames);

public:

    DecoderFrame* findFrameWithPTS(streamid_t stream, uint64_t targetPTSUs, uint64_t now);

    void_t uploadFrame(DecoderFrame* frame, uint64_t uploadTimeUs);

    size_t getDecodedFrameCount(streamid_t stream);

    // Called by decoder manager when discarding frames 
    void_t setDiscardTarget(streamid_t stream, uint64_t targetPTSUs);

    void_t flushFrames(streamid_t stream);

    void_t cleanUpOldFrames(const Streams& streams, uint64_t currentPTS);

    void_t clearDiscardedFrames(const Streams& streams);

    bool_t syncStreams(streamid_t anchorStream, streamid_t stream);

    bool_t stageFrame(DecoderFrame* frame);

    DecoderFrame* fetchStagedFrame(streamid_t stream);

public:

    virtual DecoderFrame* createFrame(uint32_t width, uint32_t height) = 0;
    virtual void_t destroyFrame(DecoderFrame* frame) = 0;
    virtual void_t uploadTexture(DecoderFrame* frame) = 0;

    void_t releaseVideoFrame(VideoFrame& videoFrame);
    void_t destroyInstance();

protected:
    FrameCache();

private:
    void_t discardFrames(streamid_t, uint64_t targetPTSUs);

protected:

    VideoFrames mCurrentVideoFrames;

private:

    typedef FixedArray<DecodedFrameGroup*, MAX_STREAM_COUNT> DecodedFrameGroups;
    typedef FixedArray<FreeFrameGroup*, MAX_STREAM_COUNT> FreeFrameGroups;
    typedef FixedArray<uint64_t, MAX_STREAM_COUNT> DiscardTargets;

    Mutex mFramePoolMutex;
    FrameList mFramePool;
    DecodedFrameGroups mDecodedFrameGroups;
    FreeFrameGroups mFreeFrameGroups;
    FrameList mActiveFrames;

    DiscardTargets mDiscardTargets;

    uint64_t mLastUploadClockTime;

};

OMAF_NS_END
