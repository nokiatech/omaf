
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
#include "NVRFreeFrameGroup.h"
#include "VideoDecoder/NVRFrameCache.h"
#include "VideoDecoder/NVRVideoDecoderHW.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(FreeFrameGroup)
FreeFrameGroup::FreeFrameGroup(streamid_t stream, uint32_t width, uint32_t height)
: mStream(stream)
, mWidth(width)
, mHeight(height)
, mActive(false)

{
}

FreeFrameGroup::~FreeFrameGroup()
{
    OMAF_ASSERT(mFrames.isEmpty() && !mActive, "Not deactivated");
}

DecoderFrame* FreeFrameGroup::aquireFreeFrame()
{
    //OMAF_ASSERT(mActive, "Not activated");
    Spinlock::ScopeLock lock(mSpinLock);

    if (mFrames.isEmpty())
    {
        return OMAF_NULL;
    }
    else
    {
        DecoderFrame* frame = mFrames.front();
        frame->streamId = mStream;
        frame->decoder = OMAF_NULL;
        frame->consumed = false;
        frame->flushed = false;
        frame->uploadTime = OMAF_UINT64_MAX;
        mFrames.pop();
        return frame;
    }
}

void_t FreeFrameGroup::returnFreeFrame(DecoderFrame* frame)
{
    OMAF_ASSERT(mActive, "Not activated");
    OMAF_ASSERT(frame->streamId == mStream, "Incorrect stream");
    Spinlock::ScopeLock lock (mSpinLock);
    if (frame->decoder != OMAF_NULL && !frame->consumed)
    {
        frame->decoder->consumedFrame(frame);
    }
    mFrames.push(frame);
}

uint32_t FreeFrameGroup::getWidth() const
{
    return mWidth;
}

uint32_t FreeFrameGroup::getHeight() const
{
    return mHeight;
}

void_t FreeFrameGroup::activate(FrameList &frames)
{
    OMAF_ASSERT(!mActive, "Already activated");
    Spinlock::ScopeLock lock(mSpinLock);
    for (FrameList::Iterator it = frames.begin(); it != frames.end(); ++it)
    {
        mFrames.push(*it);
    }
    mActive = true;
}

FrameList FreeFrameGroup::deactivate()
{
    Spinlock::ScopeLock lock(mSpinLock);
 //   OMAF_ASSERT(mActive, "Not active");
    //TODO activate is called from renderer thread, but deactivate can come from provider thread, before renderer thread has activated this
    if (!mActive)
    {
        FrameList frames;
        return frames;
    }
    FrameList frames;
    while(!mFrames.isEmpty())
    {
        frames.add(mFrames.front());
        mFrames.pop();
    }
    mActive = false;
    return frames;
}

size_t FreeFrameGroup::currentFreeFrames() const
{
    return mFrames.getSize();
}

OMAF_NS_END
