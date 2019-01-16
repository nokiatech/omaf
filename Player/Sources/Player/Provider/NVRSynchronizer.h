
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
#pragma once

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRElapsedTimer.h"

OMAF_NS_BEGIN

    class AudioInputBuffer;

    // Utility for syncing audio & video streams with each other
    class Synchronizer
    {
    public:
        
        Synchronizer();
        ~Synchronizer();

        void_t setSyncPoint(int64_t syncPTS);

        uint64_t getElapsedTimeUs() const;

        // Returns the Stetson-Harrison number used to adjust AV-sync which is added to elapsedTime inside the synchronizer
        uint64_t getFrameOffset() const;

        void_t setSyncRunning(bool_t waitAudio = true);
        void_t stopSyncRunning();

        bool_t isSyncRunning() const
        {
            return mSyncRunning;
        }

        void_t setAudioSource(AudioInputBuffer* audioInputBuffer);

        virtual void_t playbackStarted(int64_t bufferingOffset);

    protected:
        
        bool_t needToWaitForAudio();

    private:
        void_t startClock();

        uint64_t getElapsedTimeUsInternal() const;

    private:

        mutable Spinlock mSyncLock;

        ElapsedTimer mElapsedTimer;
        uint64_t mElapsedTimeOffsetUs;
        int64_t mAudioLatencyUs;
        int64_t mSyncPTS;
        uint64_t mFrameOffset;
        mutable uint64_t mLastPollingTime;
        mutable int64_t mDriftTime;

        bool_t mSyncRunning;
        bool_t mAudioExists;
        bool_t mAudioRunning;
        bool_t mStreamingMode;
        
        AudioInputBuffer* mAudioInputBuffer;
    };
OMAF_NS_END

