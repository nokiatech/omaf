
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
#include "Context.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRLogger.h"
#include "Audio/NVRAudioRendererAPI.h"
#include <Math/OMAFMathFunctions.h>
#include "WasapiDevice.h"
#include <mmdeviceapi.h>
#include <Audiosessiontypes.h>

#include <vector>

//i think these might not be needed anymore. (the accesses are synchronized)
#define AtomicSet(a,b) InterlockedExchange64(a,b);
#define AtomicAdd(a,b) InterlockedAdd64(a,b);
#define AtomicGet(a) InterlockedAdd64(a,0);


OMAF_NS_BEGIN
OMAF_LOG_ZONE(WASAPIDevice)

#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }
#define SAFE_CLOSE(hndl) if (hndl != NULL) {CloseHandle(hndl);hndl=NULL;}
#define SAFE_DELETE(ptr) OMAF_DELETE(mContext->mAllocator,ptr);ptr=NULL;
//WASAPI timeunit is 100nanoseconds.
#define REFTIMES_PER_SEC 10000000
#define REFTIMES_PER_MILLISEC 10000
#define REFTIMES_PER_MICROSEC 10

#ifndef AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
//for some reason this flag is not defined in all SDK versions..
//BUT it is still documented and implemented!
#define AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM 0x80000000
#endif


namespace WASAPIImpl
{
  
    WASAPIDevice::WASAPIDevice(Context* aContext) :
        mAACDecoder(NULL),
        mAudioClient(NULL),
        mRenderClient(NULL),
        mAudioClock(NULL),
        pSessionControl(NULL),
        mContext(aContext),
        mDefaultPeriod(0),
        mMinimumPeriod(0),
        mLatency(0),
        mBufferDuration(0),
        mInputFormat({ 0 }),
        mSharedMixFormat(NULL),
        mPlaybackFormat(NULL),
        //QUESTION: which mode should we use and should it be configurable?
        //mSharedMode(true),//shared mode has more latency but wont block other audio.. (around 22.6ms!!)
        mSharedMode(false),//exclusive mode has potentially a lot lower latency but will block other audio.. (around 6ms)
        mNumFrames(0),
        mFrequency(0),
        mLastPosition(0),
        mStarted(false)
    {
    }

    WASAPIDevice::~WASAPIDevice()
    {
        shutdown();
    }

    void WASAPIDevice::shutdown()
    {
        if (mAudioClient)
        {
            mAudioClient->Stop();
            ResetEvent(mContext->onNeedMoreSamples);
            mAudioClient->Reset();//Resets clock!
        }
        SAFE_RELEASE(mRenderClient);
        SAFE_RELEASE(mAudioClock);
#if 0
        if (pSessionControl)
        {
            pSessionControl->UnregisterAudioSessionNotification(mContext);
        }
        SAFE_RELEASE(pSessionControl);
#endif
        SAFE_RELEASE(mAudioClient);
        if ((mPlaybackFormat != mSharedMixFormat) && (mPlaybackFormat != &mInputFormat))
        {
            CoTaskMemFree(mPlaybackFormat);
            mPlaybackFormat = NULL;
        }
        if (mSharedMixFormat)
        {
            CoTaskMemFree(mSharedMixFormat);
            mSharedMixFormat = NULL;
        }
        if (mAACDecoder) {
            SAFE_DELETE(mAACDecoder);
        }
    }

    bool_t WASAPIDevice::init(int32_t aSampleRate, int32_t aChannels)
    {
        HRESULT hr;
        if (mContext->mDevice == NULL) return false;
        mSharedMode = mContext->mSharedMode;
        mStarted = false;
        hr = mContext->mDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&mAudioClient);
        if (FAILED(hr)) return false;
        hr = mAudioClient->GetMixFormat((WAVEFORMATEX**)&mSharedMixFormat);
        if (FAILED(hr)) return false;
        hr = mAudioClient->GetDevicePeriod(&mDefaultPeriod, &mMinimumPeriod);
        if (FAILED(hr)) return false;
        mBufferDuration = REFTIMES_PER_SEC / 180;
        mBufferDuration = max(mMinimumPeriod, mBufferDuration);

        //https://msdn.microsoft.com/en-us/windows/hardware/drivers/audio/extensible-wave-format-descriptors
        mInputFormat.Format.cbSize = 22;
        mInputFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        mInputFormat.Format.nSamplesPerSec = aSampleRate;
        mInputFormat.Format.nChannels = aChannels;
        mInputFormat.Format.wBitsPerSample = 16;
        mInputFormat.Format.nBlockAlign = (mInputFormat.Format.nChannels * mInputFormat.Format.wBitsPerSample) / 8;
        mInputFormat.Format.nAvgBytesPerSec = (mInputFormat.Format.nSamplesPerSec*mInputFormat.Format.nBlockAlign);
        mInputFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        mInputFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        mInputFormat.Samples.wValidBitsPerSample = 16;

        if (mSharedMode)
        {
            //we should check if our input format is supported, and if not setup playing at mSharedMixFormat and do conversion...
            //but we ignore this, since we use AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM. and let windows do the resampling / format conversion.
            /*
            hr = mAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, (WAVEFORMATEX*)&mInputFormat, (WAVEFORMATEX**)&mPlaybackFormat);
            if (FAILED(hr))
            {
                return false;
            }
            */
            mPlaybackFormat = &mInputFormat;
        }
        else
        {
            hr = mAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&mInputFormat, NULL);
            if (FAILED(hr))
            {
                //if our input format is not supported we need to resample/format convert ourselves, and in exclusive mode windows wont help us.
                //We chose to drop to shared mode where windows will help us.
                mSharedMode = false;
            }
            mPlaybackFormat = &mInputFormat;
        }

        if (mSharedMode)
        {
            hr = mAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, mBufferDuration, 0, (WAVEFORMATEX*)mPlaybackFormat, NULL);
        }
        else
        {
            hr = mAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, mBufferDuration, mBufferDuration, (WAVEFORMATEX*)mPlaybackFormat, NULL);
        }
        if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
        {
            // Get the next aligned frame.
            hr = mAudioClient->GetBufferSize(&mNumFrames);

            mBufferDuration = (REFERENCE_TIME)((10000.0 * 1000 / mPlaybackFormat->Format.nSamplesPerSec * mNumFrames) + 0.5);

            // Release the previous allocations.
            SAFE_RELEASE(mAudioClient);

            //need to activate again...
            hr = mContext->mDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&mAudioClient);
            if (FAILED(hr)) return false;
            if (mSharedMode)
            {
                hr = mAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, mBufferDuration, 0, (WAVEFORMATEX*)mPlaybackFormat, NULL);
            }
            else
            {
                hr = mAudioClient->Initialize(AUDCLNT_SHAREMODE_EXCLUSIVE, AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, mBufferDuration, mBufferDuration, (WAVEFORMATEX*)mPlaybackFormat, NULL);
            }
        }
        if (FAILED(hr)) return false;

#if 0
        hr = mAudioClient->GetService(__uuidof(IAudioSessionControl), (void**)&pSessionControl);
        if (FAILED(hr)) return false;
        hr = pSessionControl->RegisterAudioSessionNotification(mContext);
        if (FAILED(hr)) return false;
#endif
        hr = mAudioClient->SetEventHandle(mContext->onNeedMoreSamples);
        if (FAILED(hr)) return false;

        //requires initialize..
        hr = mAudioClient->GetBufferSize(&mNumFrames);
        if (FAILED(hr)) return false;

        mBufferDuration = (REFERENCE_TIME)((10000.0 * 1000 / mPlaybackFormat->Format.nSamplesPerSec * mNumFrames) + 0.5);

        hr = mAudioClient->GetStreamLatency(&mLatency);
        if (FAILED(hr)) return false;

        hr = mAudioClient->GetService(__uuidof(IAudioRenderClient), (void**)&mRenderClient);
        if (FAILED(hr)) return false;

        hr = mAudioClient->GetService(__uuidof(IAudioClock), (void**)&mAudioClock);
        if (FAILED(hr)) return false;

        hr = mAudioClock->GetFrequency(&mFrequency);
        if (FAILED(hr)) return false;
        mLastPosition = 0;
        return true;
    }

    int64_t WASAPIDevice::latencyUs()
    {
        //return mLatency / REFTIMES_PER_MICROSEC;
        return mBufferDuration / REFTIMES_PER_MICROSEC;
    }

    void WASAPIDevice::stop()
    {
        if (mAudioClient)
        {
            mStarted = false;
            mAudioClient->Stop();
            ResetEvent(mContext->onNeedMoreSamples);
            mAudioClient->Reset();//Resets clock!
            mLastPosition = 0;
            //fill our audio buffer with silence...
            BYTE *pData;
            HRESULT hr = mRenderClient->GetBuffer(mNumFrames, &pData);
            if (SUCCEEDED(hr))
            {
                hr = mRenderClient->ReleaseBuffer(mNumFrames, AUDCLNT_BUFFERFLAGS_SILENT);
            }
        }
        flushDecoder();
    }

    void WASAPIDevice::pause()
    {
        if (mAudioClient)
        {
            mStarted = false;
            mAudioClient->Stop();
            ResetEvent(mContext->onNeedMoreSamples);
        }
    }

    UINT64 WASAPIDevice::position()
    {
        UINT64 position = mLastPosition;
        UINT64 qposition = 0;
        if (mAudioClock)
        {
            HRESULT hr = mAudioClock->GetPosition(&position, &qposition);
            if (SUCCEEDED(hr))
            {
                mLastPosition = position;
            }
        }
        return (position * 1000000) / (mFrequency);
    }

    bool_t WASAPIDevice::newsamples(AudioRendererAPI* audio, bool_t push)
    {
        if (mStarted&&push) return false;
        UINT32 numFramesPadding = 0;
        uint32_t freespace;
        HRESULT hr;
        size_t done = 0;

        if (mSharedMode)
        {
            hr = mAudioClient->GetCurrentPadding(&numFramesPadding); //valid only with shared mode...
        }
        else
        {
            numFramesPadding = 0;// in exclusive mode we need to fill the whole buffer..
        }

        //freespace is amount of freespace in frames (1 frame (16bit stereo = 4 bytes))
        freespace = mNumFrames - numFramesPadding;
        uint32_t pcmFrameSize = mInputFormat.Format.nChannels * 2; // 16bit + channels
        auto bytesToGet = freespace * pcmFrameSize;

        if (freespace > 0)
        {
            BYTE *pData;
            hr = mRenderClient->GetBuffer(freespace, &pData);
            if (SUCCEEDED(hr))
            {
                auto &decoder = getDecoder(audio);                       

                // feed available AAC frames to decoder until we have 
                // enough output or no more input available
                auto ret = AudioReturnValue::OK;
                while (ret != AudioReturnValue::OUT_OF_SAMPLES) {
                    size_t aacFrameSize = 0;
                    ret = audio->fetchAACFrame(aacFrameBuffer, sizeof(aacFrameBuffer), aacFrameSize);
                    OMAF_ASSERT(ret != AudioReturnValue::FAIL, "AAC input frame buffer too small");

                    if (ret == AudioReturnValue::OK) {
                        // feed new AAC frame to decoder and process output directly internal PCM output buffer
                        auto outputBufSize = decoder.feedFrame(std::vector<uint8_t>(aacFrameBuffer, aacFrameBuffer + aacFrameSize));
                        if (outputBufSize > bytesToGet) {
                            break;
                        }
                    }
                } 

                // try to read enough data from decoder to fill the output buffers
                auto pcmSamples = decoder.readOutputBuffer(bytesToGet);
 
                if (pcmSamples.size() > 0) {
                    memcpy(pData, &pcmSamples[0], pcmSamples.size());
                }
                else {
                    //EEK!
                    write_message("EEK, renderSamples returned 0 frames!");
                }

                if (pcmSamples.size() < bytesToGet) {
                    // TODO: need to cause this to happen etc.etc.etc 
                    // we offset basetime according to how much we are missing.
                    // (I have no idea what that comment is supposed to mean t. Mikael)

                    uint32_t framesToFill = (bytesToGet - pcmSamples.size()) / pcmFrameSize;
                    mContext->mBaseTime += framesToFill * 1000 / mInputFormat.Format.nSamplesPerSec;
                    write_message("Audio underflow!");
                    //and fill the rest with silence.
                    int16_t* dst = (int16_t*)pData;
                    for (size_t i = 0; i < framesToFill * mInputFormat.Format.nChannels; i++)
                    {
                        *dst = 0;
                        dst++;
                    }
                }

                pData += pcmSamples.size();

                AtomicAdd(&mContext->mPosition, pcmSamples.size() / pcmFrameSize * 1000 / mInputFormat.Format.nSamplesPerSec);
                hr = mRenderClient->ReleaseBuffer(freespace, 0);//we always release the whole buffer.

                if (FAILED(hr))
                {
                    write_message("ReleaseBuffer:%X", hr);
                }
            }
            else
            {
                write_message("GetBuffer :%X", hr);
            }

        }
        else
        {
            write_message("HW buffer is full (freespace==0)!");
        }

        if (!mStarted)
        {
            ResetEvent(mContext->onNeedMoreSamples);
            hr = mAudioClient->Start();
            mStarted = true;
            return true;
        }
        return false;
    }

    void WASAPIDevice::flushDecoder() {
        if (mAACDecoder != NULL) {
            SAFE_DELETE(mAACDecoder);
            mAACDecoder = NULL;
        }
    }

    WMFDecoder::AACDecoder& WASAPIDevice::getDecoder(AudioRendererAPI* audio) {
    
        // if AAC input has changed, reinitialize decoder
        if (mAACDecoder != NULL && (
            audio->getInputSampleRate() != mAACDecoder->getAACSampleRate() ||
            audio->getInputChannels() != mAACDecoder->getAACChannels())
            ) {
            SAFE_DELETE(mAACDecoder);
            mAACDecoder = NULL;
        }          

        if (mAACDecoder == NULL) {
            mAACDecoder = OMAF_NEW(mContext->mAllocator, WMFDecoder::AACDecoder)(
                WMFDecoder::DecoderInputType::AAC_LC,
                audio->getInputSampleRate(),
                audio->getInputChannels(),
                2);
        }

        return *mAACDecoder;
    }

}
OMAF_NS_END
