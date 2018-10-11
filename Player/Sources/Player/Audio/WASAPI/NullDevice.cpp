
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
#define WASAPI_LOGS
#include "NullDevice.h"
#include "Context.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRClock.h"
#include "Audio/NVRAudioRendererAPI.h"
//i think these might not be needed anymore. (the accesses are synchronized)
#define AtomicSet(a,b) InterlockedExchange64(a,b);
#define AtomicAdd(a,b) InterlockedAdd64(a,b);
#define AtomicGet(a) InterlockedAdd64(a,0);

OMAF_NS_BEGIN
OMAF_LOG_ZONE(NullDevice)

namespace WASAPIImpl
{    
    NullDevice::NullDevice(Context* aContext) :
        mContext(aContext)
    {
        init(0,0);
        PauseEvent = CreateEvent(NULL, true, false, NULL);

        Thread::EntryFunction function;
        function.bind<NullDevice, &NullDevice::threadEntry>(this);

        mThread.setPriority(Thread::Priority::LOWEST);
        mThread.setName("NullAudioDevice::PollingThread");
        mThread.start(function);

    }

    NullDevice::~NullDevice()
    {
        stop();
        mThread.stop();
        SetEvent(PauseEvent);
        mThread.join();
    }
    Thread::ReturnValue NullDevice::threadEntry(const Thread& thread, void_t* userData)
    {
        while (!thread.shouldStop())
        {
            WaitForSingleObject(PauseEvent, INFINITE);
            SetEvent(mContext->onNeedMoreSamples);
            Sleep(5);//requst samples every 5 ms...
        }
        return 0;
    }
    bool_t NullDevice::init(int32_t aSampleRate, int32_t aChannels)
    {
        mSampleRate = aSampleRate;
        mChannels = aChannels;
        mStarted = false;
        mPaused = false;
        mPosition = 0;
        mStartTime = 0;
        mPauseTime = 0;
        mLastPlayed = 0;
        return true;
    }

    int64_t NullDevice::latencyUs()
    {
        return 0;
    }

    void NullDevice::stop()
    {
        ResetEvent(PauseEvent);//Stops the polling thread..
        AtomicSet(&mPosition, 0);
        mStartTime = 0;
        mPauseTime = 0;
        mStarted = false;
    }
    void NullDevice::pause()
    {
        //pause playback. but dont reset..
        //(store the pause time, so we can adjust the "starttime".. which is used to count samples to consume etcetc)
        ResetEvent(PauseEvent);//Stops the polling thread..
        if (mPauseTime == 0)
            mPauseTime = Clock::getMilliseconds();
    }
    UINT64 NullDevice::position()
    {
        int64_t ret;
        ret = AtomicGet(&mPosition);
        ret *= 1000000;
        ret /= (int64_t)mSampleRate;
        return ret;
    }

    bool_t NullDevice::newsamples(AudioRendererAPI* audio, bool_t push)
    {
        bool_t ret = false;
        int64_t curtime = Clock::getMilliseconds();
        if (!mStarted)
        {
            mLastTime=mStartTime = curtime;
            mLastPlayed = 0;
            mPauseTime = 0;
            mStarted = true;
            ret = true;
            SetEvent(PauseEvent);
        }
        else
        {
            if (mPauseTime)
            {
                //mStartTime += (curtime - mPauseTime);
                mPauseTime = 0;
                mLastTime = mStartTime = curtime;
                SetEvent(PauseEvent);
            }
        }
        //count how many frames should have been played since the start..
        int64_t timePlayed = curtime-mStartTime;
        //TODO: fixup time if delta is too big. (ie. we are debugging...)
        if ((curtime- mLastTime)> 500)
        {
            mStartTime = curtime - (mLastTime - mStartTime);
            timePlayed = curtime - mStartTime;
        }

        int64_t samplesPlayed = (timePlayed*(int64_t)mSampleRate) / 1000; //frames played.
        //count how many since last call..
        int64_t todo = samplesPlayed - mLastPlayed;
        
        if (todo > 0)
        {
            int64_t consumed = 0;
            //consume required amount of samples..
            size_t maxSamples = sizeof(mDummy) / sizeof(mChannels);
            while (todo)
            {
                size_t request = todo;
                if (request > maxSamples) request = maxSamples;
                size_t received;
                request *= mChannels;

                // TODO: fetch AAC stuff
                // audio->renderSamples(request, (int16_t*)mDummy, received);

                received /= mChannels;
                consumed += received;
                todo -= received;
                if (received == 0) break;
            }
            if (todo != 0)
            {
                //TODO: must adjust the mStartTime here based on the missing sample count..
                //since the play cursor should not move when there is no data....
                //this code might be completely incorrect. not sure how to properly force the case to test it....
                //also the millisecond clock might be too inaccurate.....
                write_message("Underflow!");
                int64_t samplesMissed = todo;
                mStartTime += (samplesMissed * 1000) / (int64_t)mSampleRate;
            }
            AtomicAdd(&mContext->mPosition, consumed);
            AtomicAdd(&mPosition, consumed);
            //write_message("samplesPlayed:%lld %lld", samplesPlayed, mPosition);
        }
        mLastPlayed = samplesPlayed;
        mLastTime = curtime;
        return ret;
    }

}
OMAF_NS_END
