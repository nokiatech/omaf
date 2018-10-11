
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
// NOMMIDS Multimedia IDs are not defined
// NONEWWAVE No new waveform types are defined except WAVEFORMATEX
// NONEWRIFF No new RIFF forms are defined
// NOJPEGDIB No JPEG DIB definitions
// NONEWIC No new Image Compressor types are defined
// NOBITMAP No extended bitmap info header definition
#define NOMMIDS
#define NONEWWAVE
#define NONEWRIFF
#define NOJPEGDIB
#define NONEWIC
#define NOBITMAP

#include "endpoint.h"
#include "AACDecoder.h"

#include <Audioclient.h>
#include <AudioPolicy.h>

OMAF_NS_BEGIN
namespace WASAPIImpl
{    
    class WASAPIDevice : public Endpoint
    {
    public:
        WASAPIDevice(Context* aContext);
        ~WASAPIDevice();
        bool_t init(int32_t aSamplerate, int32_t aChannels);
        int64_t latencyUs();
        void stop();
        void pause();
        uint64_t position();
        bool_t newsamples(AudioRendererAPI* audio, bool_t push);
        void shutdown();

    protected:
        // sets decoder to be in fresh state
        void flushDecoder();

        // initializes (if necessary) and returns decoder for audio source
        WMFDecoder::AACDecoder& getDecoder(AudioRendererAPI* audio);

        // input aac buffer size (should be big enough, usually buffers are less that 2kB)
        uint8_t aacFrameBuffer[1024 * 32];

        WMFDecoder::AACDecoder* mAACDecoder;
        IAudioClient* mAudioClient;
        IAudioRenderClient* mRenderClient;
        IAudioClock* mAudioClock;
        IAudioSessionControl* pSessionControl;
        Context* mContext;
        REFERENCE_TIME mDefaultPeriod, mMinimumPeriod, mLatency, mBufferDuration;
        WAVEFORMATEXTENSIBLE mInputFormat;
        PWAVEFORMATEXTENSIBLE mSharedMixFormat;
        PWAVEFORMATEXTENSIBLE mPlaybackFormat;
        bool_t mSharedMode;
        UINT32 mNumFrames;
        UINT64 mFrequency;
        UINT64 mLastPosition;
        bool_t mStarted;
    };
}
OMAF_NS_END
