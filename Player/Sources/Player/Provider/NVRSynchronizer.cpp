
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Provider/NVRSynchronizer.h"
#include "Audio/NVRAudioInputBuffer.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN

static const uint64_t POLLING_FREQUENCY = 1000 * 1000;

OMAF_LOG_ZONE(Synchronizer)

Synchronizer::Synchronizer()
    : mElapsedTimer(0)
    , mElapsedTimeOffsetUs(0)
    , mAudioLatencyUs(0)
    , mSyncPTS(0)
    , mLastPollingTime(0)
    , mDriftTime(0)
    , mSyncRunning(false)
    , mAudioExists(false)
    , mAudioRunning(false)
    , mStreamingMode(false)
    , mAudioInputBuffer(OMAF_NULL)
    , mFrameOffset(0)
    , mSwitchOffset(0)
    , mSyncSet(false)
{
}

Synchronizer::~Synchronizer()
{
}

void_t Synchronizer::setAudioSyncPoint(int64_t syncPTS)
{
    if (mSyncSet)
    {
        return;
    }

    setSyncPoint(syncPTS);
}

void_t Synchronizer::setVideoSyncPoint(int64_t syncPTS)
{
    if (mAudioExists || mSyncSet)
    {
        return;
    }
    setSyncPoint(syncPTS);
}

void_t Synchronizer::setSyncPoint(int64_t syncPTS)
{
    Spinlock::ScopeLock lock(mSyncLock);

    if (mElapsedTimer.isValid())
    {
        mElapsedTimer.restart();
    }
    else
    {
        startClock();
    }

    mSyncPTS = syncPTS;
    OMAF_LOG_D("Sync set to %lld", mSyncPTS);
    // pause+resume offset is not valid after seek offset is set
    mElapsedTimeOffsetUs = 0;
    mDriftTime = 0;
    mLastPollingTime = 0;
    mSyncSet = true;
}

void_t Synchronizer::reset(bool_t waitAudio)
{
    OMAF_LOG_D("reset");
    stopSyncRunning();

    Spinlock::ScopeLock lock(mSyncLock);
    mElapsedTimeOffsetUs = 0;
    mSyncPTS = 0;
    mDriftTime = 0;
    mAudioLatencyUs = 0;
    mSyncSet = false;
    if (waitAudio)
    {
        mAudioExists = true;
    }
    else
    {
        mAudioExists = false;
    }
}

void_t Synchronizer::playbackStarted(int64_t bufferingOffsetUs)
{
    Spinlock::ScopeLock lock(mSyncLock);
    if (mElapsedTimer.isValid())
    {
        OMAF_LOG_D("audio playback started but ignoring since we are playing already");
        return;  // Ignore
    }
    OMAF_LOG_D("audio playback started %lld", bufferingOffsetUs);

    mAudioLatencyUs = bufferingOffsetUs;
    mAudioRunning = true;

    startClock();
}

void_t Synchronizer::startClock()
{
    if (mElapsedTimer.isValid())
    {
        mElapsedTimer.invalidate();
    }

    OMAF_LOG_D("startClock");
    // with video, frame offset set to 33ms, this should make the video run one frame earlier than the audio. With still
    // images better to have it as 0
    mFrameOffset = 33000;
    mElapsedTimer.start();
}

uint64_t Synchronizer::getElapsedTimeUs() const
{
    Spinlock::ScopeLock lock(mSyncLock);
    return getElapsedTimeUsInternal();
}

uint64_t Synchronizer::getFrameOffset() const
{
    return mFrameOffset;
}

uint64_t Synchronizer::getElapsedTimeUsInternal() const
{
    if (mElapsedTimer.isValid())
    {
        // audio HW can pass audio to loudspeakers faster than video path executes => elapsed time is more than the
        // timer indicates
        // or mAudioLatency > 0, i.e. buffering in audio is longer than for video => elapsed time is less than the timer
        // indicates
        int64_t time = mElapsedTimer.elapsedUs() + mElapsedTimeOffsetUs + mSwitchOffset;

        if (mAudioExists && mAudioInputBuffer != OMAF_NULL && mAudioInputBuffer->getState() == AudioState::PLAYING &&
            (abs(time - (int64_t) mLastPollingTime) > POLLING_FREQUENCY))
        {
            mLastPollingTime = time;

            int64_t audioTime = mAudioInputBuffer->getElapsedTimeUs();
            // if we get here but audioTime == 0, there is some audio but it should not be used for drift calculations
            if (audioTime > 0)
            {
                mDriftTime = time - audioTime;

                OMAF_LOG_D("%p: elapsedTime %lld, audio %lld, drift %lld, mediaTime: %lld ms", (void *) this, time,
                           audioTime, mDriftTime, (mSyncPTS + time) / 1000);
            }
        }

        time -= mDriftTime;

        time += mSyncPTS + mAudioLatencyUs + mFrameOffset;

        if (time > 0)
        {
            return time;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        int64_t elapsed = (int64_t)(mElapsedTimeOffsetUs + mSyncPTS + mFrameOffset) + mSwitchOffset;
        if (elapsed > 0)
        {
            return (uint64_t) elapsed;
        }
        else
        {
            return 0;
        }
    }
}

bool_t Synchronizer::needToWaitForAudio()
{
    if (mAudioExists)
    {
        if (mAudioRunning)
        {
            // audio is running already
            return false;
        }
        else
        {
            // audio exists, but has not started yet
            return true;
        }
    }
    else
    {
        // no audio, no need to wait
        return false;
    }

    return false;
}

void_t Synchronizer::setAudioSource(AudioInputBuffer *audioInputBuffer)
{
    mAudioInputBuffer = audioInputBuffer;
}

void_t Synchronizer::setSyncRunning(bool_t waitAudio)
{
    Spinlock::ScopeLock lock(mSyncLock);

    if (mSyncRunning)
    {
        return;
    }

    // OMAF_LOG_D("setSyncRunning");

    mSyncRunning = true;

    if (mAudioInputBuffer != OMAF_NULL && mAudioInputBuffer->isInitialized())
    {
        mAudioExists = true;
    }
    else
    {
        mAudioExists = false;
    }

    if (!mAudioExists || !waitAudio)
    {
        startClock();
    }
}

void_t Synchronizer::stopSyncRunning()
{
    Spinlock::ScopeLock lock(mSyncLock);

    if (!mSyncRunning)
    {
        return;
    }

    // OMAF_LOG_D("stopSyncRunning");

    mSyncRunning = false;

    if (mAudioInputBuffer != OMAF_NULL && mAudioInputBuffer->isInitialized())
    {
        mAudioExists = true;
    }
    else
    {
        mAudioExists = false;
    }

    if (mAudioExists)
    {
        // reset audio elapsed time counter
        uint64_t audioTime = mAudioInputBuffer->getElapsedTimeUs();
        if (audioTime > 0)
        {
            mElapsedTimeOffsetUs = audioTime;
        }
        else
        {
            mElapsedTimeOffsetUs = getElapsedTimeUsInternal() - mSyncPTS;
        }
        mAudioRunning = false;
        // OMAF_LOG_D("elapsedTimeOffset %lld",mElapsedTimeOffsetUs);
    }
    else
    {
        // if we did not have the audio, lost the player,
        // setElapsedTime was called while running (scary!),
        // or something,
        // just use the timer value
        mElapsedTimeOffsetUs = getElapsedTimeUsInternal() - mSyncPTS;
    }

    mElapsedTimer.invalidate();
}
OMAF_NS_END
