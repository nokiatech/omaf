
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

class FreeFrameGroup
{
public:
    
    FreeFrameGroup(streamid_t stream, uint32_t width, uint32_t height);
    ~FreeFrameGroup();

    void_t activate(FrameList& frames);
    FrameList deactivate();

    DecoderFrame* aquireFreeFrame();
    void_t returnFreeFrame(DecoderFrame* frame);

    uint32_t getWidth() const;
    uint32_t getHeight() const;

    size_t currentFreeFrames() const;

private:
    
    streamid_t mStream;
    uint32_t mWidth;
    uint32_t mHeight;

    bool_t mActive;

    Spinlock mSpinLock;
    FrameQueue mFrames;
};

OMAF_NS_END
