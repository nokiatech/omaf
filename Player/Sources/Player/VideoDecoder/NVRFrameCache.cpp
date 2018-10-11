
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
#include "VideoDecoder/NVRFrameCache.h"
#include "VideoDecoder/NVRVideoDecoderHW.h"
#include "VideoDecoder/NVRDecodedFrameGroup.h"
#include "VideoDecoder/NVRFreeFrameGroup.h"

#include "Foundation/NVRLogger.h"
#include "Graphics/NVRRenderBackend.h"
#include "Foundation/NVRClock.h"

#if OMAF_VIDEO_DECODER_NULL
#elif OMAF_PLATFORM_ANDROID
#include "VideoDecoder/Android/NVRFrameCacheAndroid.h"
#elif OMAF_PLATFORM_WINDOWS
#include "VideoDecoder/Windows/NVRFrameCacheWindows.h"
#elif OMAF_PLATFORM_UWP
#include "VideoDecoder/UWP/NVRFrameCacheWindows.h"
#endif

OMAF_NS_BEGIN

const uint32_t FRAMES_PER_STREAM = 3; //Changed to 3 to optimize caching latency
static const int64_t MAX_FRAME_AGE = 50000;

static uint64_t REQUIRED_FRAME_DURATION_PERCENTAGE = 90;

OMAF_LOG_ZONE(FrameCache)

FrameCache* FrameCache::createFrameCache()
{
#if OMAF_VIDEO_DECODER_NULL
    return OMAF_NULL;
#elif OMAF_PLATFORM_UWP
    return OMAF_NEW_HEAP(FrameCacheWindows);
#elif OMAF_PLATFORM_WINDOWS
    return OMAF_NEW_HEAP(FrameCacheWindows);
#elif OMAF_PLATFORM_ANDROID
    return OMAF_NEW_HEAP(FrameCacheAndroid);
#endif
    return OMAF_NULL;
}

FrameCache::FrameCache()
: mLastUploadClockTime(0)
{
    FrameList frames;

    while (mDecodedFrameGroups.getSize() != MAX_STREAM_COUNT)
    {
        mDecodedFrameGroups.add(OMAF_NEW_HEAP(DecodedFrameGroup)(this));
    }

    while (mDiscardTargets.getSize() != MAX_STREAM_COUNT)
    {
        mDiscardTargets.add(INVALID_DISCARD_TARGET);
    }

    while (mActiveFrames.getSize() != MAX_STREAM_COUNT)
    {
        mActiveFrames.add(OMAF_NULL);
    }

    while (mCurrentVideoFrames.getSize() != MAX_STREAM_COUNT)
    {
        mCurrentVideoFrames.add(VideoFrame());
    }

    while (mFreeFrameGroups.getSize() != MAX_STREAM_COUNT)
    {
        mFreeFrameGroups.add(OMAF_NULL);
    }
}

FrameCache::~FrameCache()
{
    // Resources are released in destroy which should be called from the subclassses
    OMAF_ASSERT(mFreeFrameGroups.isEmpty(), "Destroy instance not called");
}

void_t FrameCache::destroyInstance()
{
    for (streamid_t stream = 0; stream < MAX_STREAM_COUNT; stream++)
    {
        OMAF_ASSERT(mActiveFrames.at(stream) == OMAF_NULL, "Active frame still exists");
    }

    for (DecodedFrameGroups::Iterator groupIt = mDecodedFrameGroups.begin(); groupIt != mDecodedFrameGroups.end(); ++groupIt)
    {
        OMAF_DELETE_HEAP(*groupIt);
    }

    for (FreeFrameGroups::Iterator groupIt = mFreeFrameGroups.begin(); groupIt != mFreeFrameGroups.end(); ++groupIt)
    {
        OMAF_DELETE_HEAP(*groupIt);
    }
    
    mFreeFrameGroups.clear();

    for (streamid_t stream = 0; stream < MAX_STREAM_COUNT; stream++)
    {
        destroyTexture(stream);
    }

    for (FrameList::Iterator it = mFramePool.begin(); it != mFramePool.end(); ++it)
    {
        destroyFrame(*it);
    }
    
    mFramePool.clear();
}

void_t FrameCache::addDecodedFrame(DecoderFrame* decodedFrame)
{
    if (mDiscardTargets.at(decodedFrame->streamId) != INVALID_DISCARD_TARGET)
    {
        if (decodedFrame->pts < mDiscardTargets.at(decodedFrame->streamId))
        {
            OMAF_LOG_V("Stream %d releasing a frame with PTS: %lld, discard target %lld", decodedFrame->streamId, decodedFrame->pts, mDiscardTargets.at(decodedFrame->streamId));
            releaseFrame(decodedFrame);
            return;
        }
        else if (decodedFrame->pts == mDiscardTargets.at(decodedFrame->streamId))
        {
            // Set the discard target to invalid
            mDiscardTargets.at(decodedFrame->streamId) = INVALID_DISCARD_TARGET;
        }
    }
    // New frame is older than the active frame (can happen during seek with B-frames)
    // Discard it immediately
    if (mActiveFrames.at(decodedFrame->streamId) != OMAF_NULL
        && !mActiveFrames.at(decodedFrame->streamId)->flushed
        && decodedFrame->pts < mActiveFrames.at(decodedFrame->streamId)->pts)
    {
        releaseFrame(decodedFrame);
    }
    else
    {
        bool_t success = mDecodedFrameGroups.at(decodedFrame->streamId)->addFrame(decodedFrame);
        if (!success)
        {
            // Adding the frame failed for some reason or another, so just release it
            releaseFrame(decodedFrame);
        }
    }
}

DecoderFrame* FrameCache::findFrameWithPTS(streamid_t stream, uint64_t targetPTSUs, uint64_t now)
{
    DecoderFrame* selectedFrame = mDecodedFrameGroups.at(stream)->findFrameWithPTS(targetPTSUs);
    DecoderFrame* activeFrame = mActiveFrames.at(stream);
    if (activeFrame != OMAF_NULL && activeFrame->flushed)
    {
        activeFrame = OMAF_NULL;
    }

    if (activeFrame == OMAF_NULL)
    {
        return selectedFrame;
    }
    else if (selectedFrame == OMAF_NULL)
    {
        return activeFrame;
    }
    else
    {
        
        if (activeFrame->duration != 0 && ((now - activeFrame->uploadTime) < (activeFrame->duration / 100 * REQUIRED_FRAME_DURATION_PERCENTAGE)))
        {
            //OMAF_LOG_D("Returning activeFrame since visible only for %d", now - mLastUploadTime);
            return activeFrame;
        }
        else if (selectedFrame->pts >= activeFrame->pts)
        {
            return selectedFrame;
        }
        else
        {
            return activeFrame;
        }
    }
}

Error::Enum FrameCache::getSynchedFramesForPTS(const Streams& reqStreams, const Streams& enhancementStreams, uint64_t targetPTSUS, FrameList& frames)
{
    if (reqStreams.isEmpty())
    {
        return Error::ITEM_NOT_FOUND;
    }

    uint64_t smallestPTS = OMAF_UINT64_MAX;

    // First find the smallest PTS from reqStreams
    for (Streams::ConstIterator it = reqStreams.begin(); it != reqStreams.end(); ++it)
    {
        DecoderFrame* frame = findFrameWithPTS((*it), targetPTSUS, targetPTSUS);
        
        if (frame == OMAF_NULL)
        {
            // OMAF_LOG_V("getSynchedFramesForPTS target PTS %llu NULL frame from stream %d", targetPTSUS, (*it));
            return Error::ITEM_NOT_FOUND;
        }
        
        if (frame->pts < smallestPTS)
        {
            smallestPTS = frame->pts;
            //OMAF_LOG_D("getSynchedFramesForPTS target PTS %llu use smallest PTS %llu from stream %d", targetPTSUS, smallestPTS, (*it));
        }
    }
    if (smallestPTS == OMAF_UINT64_MAX)
    {
        return Error::ITEM_NOT_FOUND;
    }
    // Then do the same for the optStreams
    uint64_t smallestPTSOpt = smallestPTS;
    for (Streams::ConstIterator it = enhancementStreams.begin(); it != enhancementStreams.end(); ++it)
    {
        DecoderFrame* frame = findFrameWithPTS((*it), targetPTSUS, targetPTSUS);

        if (frame && frame->pts < smallestPTSOpt && (targetPTSUS - frame->pts < MAX_FRAME_AGE))    // allow max 50 ms old frames from enh layer
        {
            smallestPTSOpt = frame->pts;
            //OMAF_LOG_D("getSynchedFramesForPTS target PTS %llu use smallest PTS %llu from enhancementStream %d", targetPTSUS, smallestPTSOpt, (*it));
        }
    }

    // Then find the frames for the very smallest PTS
    for (Streams::ConstIterator it = reqStreams.begin(); it != reqStreams.end(); ++it)
    {
        DecoderFrame* frame = findFrameWithPTS(*it, smallestPTSOpt, targetPTSUS);
        
        if (frame && frame->pts == smallestPTSOpt)
        {
            frames.add(frame);
        }
        else
        {
            // we must always have a frame from all the reqStreams
            OMAF_LOG_D("No frame found from the req stream %d for pts %llu", *it, smallestPTSOpt);
            frames.clear();
            if (smallestPTS == smallestPTSOpt)
            {
                // no need to check any more, frames not found
                return Error::ITEM_NOT_FOUND;
            }
            // else try with the smallestPTS obtained from reqStreams
            break;
        }
    }
    if (frames.isEmpty())
    {
        // reqStreams didn't have that old frame any more; try again, now with the smallestPTS obtained from reqStreams
        for (Streams::ConstIterator it = reqStreams.begin(); it != reqStreams.end(); ++it)
        {
            DecoderFrame *frame = findFrameWithPTS(*it, smallestPTS, targetPTSUS);

            if (frame && frame->pts == smallestPTS)
            {
                frames.add(frame);
            }
            else
            {
                // we must always have a frame from all the reqStreams
                OMAF_LOG_D("No frame found from the req stream %d for pts %llu", *it, smallestPTS);
                frames.clear();
                return Error::ITEM_NOT_FOUND;
            }
        }
    }
    else
    {
        // do the enhancement stream search once with the pts used for reqStreams
        smallestPTS = smallestPTSOpt;
    }
    // then find the frames from enhancement streams
    for (Streams::ConstIterator it = enhancementStreams.begin(); it != enhancementStreams.end(); ++it)
    {
        DecoderFrame* frame = findFrameWithPTS(*it, smallestPTS, targetPTSUS);

        if (frame && frame->pts == smallestPTS)
        {
            frames.add(frame);
        }
        // can skip an enhancementStream
    }

    //OMAF_ASSERT(frames.getSize() == streams.getSize(), "Incorrect number of frames");
    //OMAF_LOG_D("Selected frames from %zd streams out of %zd, pts %llu, target %llu", frames.getSize(), reqStreams.getSize() + enhancementStreams.getSize(), frames[0]->pts, targetPTSUS);
    return Error::OK;
}

const VideoFrame& FrameCache::getCurrentVideoFrame(streamid_t stream) const
{
    return mCurrentVideoFrames.at(stream);
}

Error::Enum FrameCache::initializeStream(streamid_t stream, const DecoderConfig& config)
{
    OMAF_ASSERT(mFreeFrameGroups.at(stream) == OMAF_NULL, "stream already initialized");
    mFreeFrameGroups.at(stream) = OMAF_NEW_HEAP(FreeFrameGroup)(stream, config.width, config.height);
    
    return Error::OK;
}

Error::Enum FrameCache::activateStream(streamid_t stream)
{
    OMAF_ASSERT(mFreeFrameGroups.at(stream) != OMAF_NULL, "Stream not initialized");
    
    uint32_t width = mFreeFrameGroups.at(stream)->getWidth();
    uint32_t height = mFreeFrameGroups.at(stream)->getHeight();
    
    FrameList framesForStream;

    // If there's already an active frame, we should create one less frame for the stream
    uint32_t frameCount = FRAMES_PER_STREAM;
    
    if (mActiveFrames.at(stream) != OMAF_NULL)
    {
        frameCount--;
    }

    mFramePoolMutex.lock();

    for(size_t index = 0; index < mFramePool.getSize(); )
    {
        DecoderFrame *frame = mFramePool.at(index);
        
        if (frame->width == width && frame->height == height)
        {
            framesForStream.add(frame);
            mFramePool.removeAt(index);
        }
        else
        {
            index++;
        }

        if (framesForStream.getSize() == frameCount)
        {
            break;
        }
    }
    mFramePoolMutex.unlock();

    while (framesForStream.getSize() != frameCount)
    {
        framesForStream.add(createFrame(width, height));
    }
    mFreeFrameGroups.at(stream)->activate(framesForStream);
    // TODO: Memory management
    return Error::OK;
}

void_t FrameCache::deactivateStream(streamid_t stream)
{
    OMAF_ASSERT(mFreeFrameGroups.at(stream) != OMAF_NULL, "Stream not initialized");
    flushFrames(stream);
    FrameList frames = mFreeFrameGroups.at(stream)->deactivate();
    if (frames.isEmpty())
    {
        // the stream was not activated yet
        return;
    }
    uint32_t expectedFrameCount = FRAMES_PER_STREAM;
    if (mActiveFrames.at(stream) != OMAF_NULL)
    {
        expectedFrameCount--;
    }
    //TODO this was once FRAMES_PER_STREAM + 1??
    //OMAF_ASSERT(expectedFrameCount == frames.getSize(), "Missing frames in deactivate");
    Mutex::ScopeLock lock(mFramePoolMutex);
    mFramePool.add(frames);
}

void_t FrameCache::shutdownStream(streamid_t stream)
{
    OMAF_DELETE_HEAP(mFreeFrameGroups.at(stream));
    mFreeFrameGroups.at(stream) = OMAF_NULL;
    if (mActiveFrames.at(stream) != OMAF_NULL)
    {
        Mutex::ScopeLock lock(mFramePoolMutex);
        mFramePool.add(mActiveFrames.at(stream));
        mActiveFrames.at(stream) = OMAF_NULL;
    }
}


DecoderFrame* FrameCache::getFreeFrame(streamid_t stream)
{
    OMAF_ASSERT(mFreeFrameGroups.at(stream) != OMAF_NULL, "Stream not initialized");
    DecoderFrame* frame = mFreeFrameGroups.at(stream)->aquireFreeFrame();
    return frame;
}

size_t FrameCache::getFreeFrameCount(streamid_t stream) const
{
    OMAF_ASSERT(mFreeFrameGroups.at(stream) != OMAF_NULL, "Stream not initialized");
    return mFreeFrameGroups.at(stream)->currentFreeFrames();
}

void_t FrameCache::releaseFrame(DecoderFrame *frame)
{
    OMAF_ASSERT(mFreeFrameGroups.at(frame->streamId) != OMAF_NULL, "Stream not initialized");
    mFreeFrameGroups.at(frame->streamId)->returnFreeFrame(frame);
}

void_t FrameCache::uploadFrame(DecoderFrame* frame, uint64_t uploadTimeUs)
{
    DecoderFrame* oldActiveFrame = mActiveFrames.at(frame->streamId);
    
    if (oldActiveFrame == frame)
    {
        // Trying to upload an already uploaded frame
        return;
    }
    
    mDecodedFrameGroups.at(frame->streamId)->removeFrame(frame);
    mActiveFrames.at(frame->streamId) = frame;
    uint64_t oldFrameDuration = 0;
    if (oldActiveFrame != OMAF_NULL)
    {
        oldFrameDuration = oldActiveFrame->duration;
        releaseFrame(oldActiveFrame);
    }
/*    
#if OMAF_DEBUG_BUILD
    uint32_t uploadTime = Clock::getMilliseconds();

    uint32_t deltaTime = uploadTime - mLastUploadClockTime;
    if (deltaTime > (oldFrameDuration / 1000) + 2 || deltaTime < (oldFrameDuration / 1000) - 2)
    {
        OMAF_LOG_D("Time since last upload: %d", deltaTime);
    }
    mLastUploadClockTime = uploadTime;
#endif
*/
    frame->uploadTime = uploadTimeUs;
    uploadTexture(frame);
}

size_t FrameCache::getDecodedFrameCount(streamid_t stream)
{
    return mDecodedFrameGroups.at(stream)->getFrameCount();
}

void_t FrameCache::setDiscardTarget(streamid_t stream, uint64_t targetPTSUs)
{
    mDiscardTargets.at(stream) = targetPTSUs;
    discardFrames(stream, targetPTSUs);
}

void_t FrameCache::cleanUpOldFrames(const Streams& streams, uint64_t currentPTS)
{
    for (streamid_t streamID = 0; streamID < mDecodedFrameGroups.getSize(); streamID++)
    {
        if (streams.contains(streamID))
        {
            mDecodedFrameGroups.at(streamID)->cleanupOldFrames(currentPTS);
        }
    }
}

void_t FrameCache::flushFrames(streamid_t stream)
{
    // Mark the active frame as flushed
    if (mActiveFrames.at(stream) != OMAF_NULL)
    {
        mActiveFrames.at(stream)->flushed = true;
    }
    mDiscardTargets.at(stream) = INVALID_DISCARD_TARGET;
    mDecodedFrameGroups.at(stream)->flushFrames();
}

// called from renderer thread
bool_t FrameCache::syncStreams(streamid_t anchorStream, streamid_t stream)
{
    //OMAF_LOG_D("syncStreams stream: %d", stream);
    uint64_t anchorPts = 0;
    if (!mDecodedFrameGroups.at(anchorStream)->getOldestFramePts(anchorPts))
    {
        // no decoded frames in the anchor
        return false;
    }
    discardFrames(stream, anchorPts);
    DecoderFrame* frame = mDecodedFrameGroups.at(stream)->findFrameWithPTS(anchorPts);
    if (frame == OMAF_NULL)
    {
        // no frames in the stream
        //OMAF_LOG_D("Stream %d has no suitable frames, anchor %llu", stream, anchorPts);
        return false;
    }
    if (frame->pts != anchorPts)
    {
        // still not in sync with the anchor
        //OMAF_LOG_D("Stream %d still not in sync %llu, %llu", stream, frame->pts, anchorPts);
        return false;
    }
    else
    {
        OMAF_LOG_D("synced stream %d, pts %llu anchor %llu", stream, frame->pts, anchorPts);
        return true;
    }
}

void_t FrameCache::clearDiscardedFrames(const Streams& streams)
{
    for (Streams::ConstIterator it = streams.begin(); it != streams.end(); ++it)
    {
        mDecodedFrameGroups.at(*it)->clearDiscardedFrames();
    }
}

void_t FrameCache::discardFrames(streamid_t stream, uint64_t targetPTSUs)
{
    mDecodedFrameGroups.at(stream)->discardFrames(targetPTSUs);
}

void_t FrameCache::releaseVideoFrame(VideoFrame &videoFrame)
{
    for (size_t index = 0; index < videoFrame.numTextures; ++index)
    {
        RenderBackend::destroyTexture(videoFrame.textures[index]);
        videoFrame.textures[index] = TextureID::Invalid;
    }
}

bool_t FrameCache::stageFrame(DecoderFrame* frame)
{
    return mDecodedFrameGroups.at(frame->streamId)->stageFrame(frame);
}

DecoderFrame* FrameCache::fetchStagedFrame(streamid_t stream)
{
    return mDecodedFrameGroups.at(stream)->fetchStagedFrame();
}

OMAF_NS_END
