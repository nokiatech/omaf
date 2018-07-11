
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
#include "VideoDecoder/NVRDecodedFrameGroup.h"
#include "VideoDecoder/NVRFrameCache.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DecodedFrameGroup)

static const uint64_t TOO_OLD_FRAME_THRESHOLD = 99000; // 99 ms

DecodedFrameGroup::DecodedFrameGroup(FrameCache* frameCache)
: mFrameCache(frameCache)
, mStagedFrame(OMAF_NULL)
{

}

DecodedFrameGroup::~DecodedFrameGroup()
{
    OMAF_ASSERT(mFrames.isEmpty(), "Frames not empty");
}

bool_t DecodedFrameGroup::addFrame(DecoderFrame* frame)
{
    Spinlock::ScopeLock lock(mFramesLock);
    bool_t added = false;
    // "Add to the end" optimization
    if (mFrames.isEmpty() || (frame->pts > mFrames.at(mFrames.getSize() - 1)->pts))
    {
        added = true;
        mFrames.add(frame);
    }
    else if (frame->pts < mFrames.at(0)->pts)
    {
        mFrames.add(frame, 0);
        added = true;
    }
    else
    {
        for (size_t index = 0; index < mFrames.getSize(); index++)
        {
            size_t reversedIndex = mFrames.getSize() - index - 1;
            if (frame->pts > mFrames.at(reversedIndex)->pts)
            {
                mFrames.add(frame, reversedIndex + 1);
                added = true;
                break;
            }
            else if (frame->pts == mFrames.at(reversedIndex)->pts)
            {
                // Matches an already added frame, something wrong. Break
                OMAF_LOG_D("Trying to add a frame with a duplicate PTS");
                break;
            }
        }
    }
    return added;
}

DecoderFrame* DecodedFrameGroup::findFrameWithPTS(uint64_t targetPTSUs)
{
    Spinlock::ScopeLock lock(mFramesLock);
    DecoderFrame* selectedFrame = findFrameWithPTSInternal(targetPTSUs, true);

#if VERIFY_CACHE
    uint64_t pts = 0;
    for (size_t index = 0; index < mFrames.getSize(); index++)
    {
        if (index == 0)
        {
            pts = mFrames.at(index)->pts;
        }
        else if (mFrames.at(index)->pts < pts)
        {
            OMAF_ASSERT(false, "INCORRECT PTS ORDER");
        }
    }
#endif

    return selectedFrame;
}

DecoderFrame* DecodedFrameGroup::findFrameWithPTSInternal(uint64_t targetPTSUs, bool_t ignoreOldFrames)
{
    DecoderFrame* selectedFrame = OMAF_NULL;
    if (mFrames.isEmpty())
    {
        return OMAF_NULL;
    }

    for (size_t index = 0; index < mFrames.getSize(); index++)
    {
        size_t reversedIndex = mFrames.getSize() - index - 1;
        if (mFrames.at(reversedIndex)->pts <= targetPTSUs)
        {
            // Select the frame only if it's not too old, this is to avoid the fast-forward effect in some cases
            if (!ignoreOldFrames)
            {
                selectedFrame = mFrames.at(reversedIndex);
            }
            else if (targetPTSUs - mFrames.at(reversedIndex)->pts < TOO_OLD_FRAME_THRESHOLD)
            {
                selectedFrame = mFrames.at(reversedIndex);
            }
            break;
        }
    }
    return selectedFrame;
}

void_t DecodedFrameGroup::removeFrame(OMAF::Private::DecoderFrame *frame)
{
    Spinlock::ScopeLock lock(mFramesLock);
    mFrames.remove(frame);
}

void_t DecodedFrameGroup::cleanupOldFrames(uint64_t currentPTS)
{
    uint64_t tooOldPTS = 0;
    if (currentPTS > TOO_OLD_FRAME_THRESHOLD)
    {
        tooOldPTS = currentPTS - TOO_OLD_FRAME_THRESHOLD;
    }
    discardFramesInternal(tooOldPTS);
}

void_t DecodedFrameGroup::flushFrames()
{
    discardFramesInternal(OMAF_UINT64_MAX);
    mStagedFrameLock.lock();
    if (mStagedFrame != OMAF_NULL)
    {
        DecoderFrame* stagedFrame = mStagedFrame;
        mStagedFrame = OMAF_NULL;
        mStagedFrameLock.unlock();
        mFrameCache->releaseFrame(stagedFrame);
    }
    else
    {
        mStagedFrameLock.unlock();
    }
    
    clearDiscardedFrames();
}

void_t DecodedFrameGroup::clearDiscardedFrames()
{
    mDiscardedFramesLock.lock();
    // Copy the frames to a temporary list to minimize the lock time
    FrameList tempList;
    tempList.add(mDiscardedFrames);
    mDiscardedFrames.clear();
    mDiscardedFramesLock.unlock();

    for (size_t index = 0; index < tempList.getSize(); index++)
    {
        mFrameCache->releaseFrame(tempList.at(index));
    }
}

void_t DecodedFrameGroup::discardFrames(uint64_t targetPTSUs)
{
    discardFramesInternal(targetPTSUs);
}

void_t DecodedFrameGroup::discardFramesInternal(uint64_t targetPTSUs)
{
    Spinlock::ScopeLock lock(mFramesLock);
    FrameList discardedFrames;
    size_t index = 0;
    while (index < mFrames.getSize())
    {
        DecoderFrame* frame = mFrames.at(index);
        if (frame->pts < targetPTSUs)
        {
            if (targetPTSUs != OMAF_UINT64_MAX)
            {
                OMAF_LOG_D("Discarding frame with PTS %llu target %llu for stream %d", frame->pts, targetPTSUs, frame->streamId);
            }
            discardedFrames.add(frame);
            index++;
        }
        else
        {
            break;
        }
    }

    if (index > 0)
    {
        mFrames.removeAt(0, index);
    }
    mDiscardedFramesLock.lock();
    mDiscardedFrames.add(discardedFrames);
    mDiscardedFramesLock.unlock();
}

size_t DecodedFrameGroup::getFrameCount()
{
    Spinlock::ScopeLock lock(mFramesLock);
    return mFrames.getSize();
}

bool_t DecodedFrameGroup::getOldestFramePts(uint64_t& pts) const
{
    if (mFrames.isEmpty())
    {
        return false;
    }

    pts = mFrames.at(0)->pts;
    return true;
}

bool_t DecodedFrameGroup::stageFrame(DecoderFrame* frame)
{
    Spinlock::ScopeLock lock(mFramesLock);
    if (mFrames.contains(frame))
    {
        Spinlock::ScopeLock lock(mStagedFrameLock);
        mFrames.remove(frame);
        if (mStagedFrame != OMAF_NULL)
        {
            mFrameCache->releaseFrame(mStagedFrame);
        }
        mStagedFrame = frame;
        return true;
    }
    else
    {
        //OMAF_ASSERT_UNREACHABLE();
        return false;
    }
}

DecoderFrame* DecodedFrameGroup::fetchStagedFrame()
{
    Spinlock::ScopeLock lock(mStagedFrameLock);
    DecoderFrame* fetched = mStagedFrame;
    mStagedFrame = OMAF_NULL;
    return fetched;
}

OMAF_NS_END
