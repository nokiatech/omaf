
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
#include "Endpoint.h"
#include "Foundation/NVRThread.h"
OMAF_NS_BEGIN
namespace WASAPIImpl
{        
    class NullDevice :public Endpoint
    {
    public:        
        NullDevice(Context*aContext);
        ~NullDevice();
        bool_t init(int32_t aSamplerate, int32_t aChannels);
        int64_t latencyUs();
        void stop();
        void pause();
        uint64_t position();
        bool_t newsamples(AudioRendererAPI* audio, bool_t push);

    protected:

        Context* mContext;
        uint32_t mSampleRate;
        int32_t mChannels;
        bool_t mStarted;
        bool_t mPaused;

        //no device,silent
        int64_t mPosition;
        int64_t mStartTime;
        int64_t mLastTime;
        int64_t mPauseTime;
        int64_t mLastPlayed;//amount of samples "played" on last onNewSamplesArrived.
        int16_t mDummy[2048];//dummy write to buffer where we ask audiorenderer to render data..

        Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);
        Thread mThread;
        HANDLE PauseEvent;

    };
}
OMAF_NS_END
