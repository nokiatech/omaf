
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
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRSpinlock.h"
#include "VideoDecoder/NVRVideoDecoderTypes.h"

OMAF_NS_BEGIN
class FrameCache;
class DecodedFrameGroup
{
public:
    DecodedFrameGroup(FrameCache* frameCache);

    ~DecodedFrameGroup();

    bool_t addFrame(DecoderFrame* frame);
    DecoderFrame* findFrameWithPTS(uint64_t targetPTSUs);
    void_t removeFrame(DecoderFrame* frame);

    bool_t getOldestFramePts(uint64_t& pts) const;

    size_t getFrameCount();

    void_t flushFrames();
    void_t discardFrames(uint64_t targetPTSUs);

    void_t cleanupOldFrames(uint64_t currentPTS);
    
    void_t clearDiscardedFrames();

    bool_t stageFrame(DecoderFrame* frame);
    DecoderFrame* fetchStagedFrame();

private:

    void_t discardFramesInternal(uint64_t targetPTSUs);
    DecoderFrame* findFrameWithPTSInternal(uint64_t targetPTSUs, bool_t ignoreOldFrames);

private:

    FrameCache* mFrameCache;
    Spinlock mFramesLock;
    Spinlock mStagedFrameLock;
    FrameList mFrames;
    Spinlock mDiscardedFramesLock;
    FrameList mDiscardedFrames;
    DecoderFrame* mStagedFrame;
    bool_t mDiscardOldFrames;
};

OMAF_NS_END
