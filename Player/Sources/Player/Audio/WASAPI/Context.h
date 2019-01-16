
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
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRMutex.h"
#include "Foundation/NVRFixedQueue.h"
#include <mmdeviceapi.h>

OMAF_NS_BEGIN
class AudioRendererAPI;

namespace WASAPICommandEvent
{
    enum Enum
    {
        //event signals.
        TERMINATE = WAIT_OBJECT_0,
        DEVICE_CHANGED,
        COMMAND,
        NEED_MORE_SAMPLES,
        //commands.
        READY,
        PLAYING,
        PAUSED,
        FLUSH
};
}

namespace WASAPIImpl
{    
#ifdef WASAPI_LOGS
    void write_message(const char* format, ...);//wasapi logging.
#else
    #define write_message(x, ...)
#endif
    class Endpoint;
    class Context : public IMMNotificationClient
#if 0
        //NOTE: we dont actually use the audiosession events for anything, the code is left in if we actually need it later.
        , public IAudioSessionEvents
#endif
    {
    public:
        Context(MemoryAllocator& allocator);
        virtual ~Context();
        void pushCommand(uint32_t);
        //event listeners..
        //IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvObject);
        ULONG STDMETHODCALLTYPE AddRef(void);
        ULONG STDMETHODCALLTYPE Release(void);
        //IMMNotificationClient
        HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(_In_ LPCWSTR pwstrDeviceId, _In_ DWORD dwNewState);
        HRESULT STDMETHODCALLTYPE OnDeviceAdded(_In_ LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDeviceRemoved(_In_ LPCWSTR pwstrDeviceId);
        HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(_In_ EDataFlow flow, _In_ ERole role, _In_ LPCWSTR pwstrDefaultDeviceId);
        HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(_In_ LPCWSTR pwstrDeviceId, _In_ const PROPERTYKEY key);

#if 0
        //IAudioSessionEvents
        HRESULT STDMETHODCALLTYPE OnDisplayNameChanged(_In_ LPCWSTR NewDisplayName,/* [in] */ LPCGUID EventContext);
        HRESULT STDMETHODCALLTYPE OnIconPathChanged(_In_ LPCWSTR NewIconPath,/* [in] */ LPCGUID EventContext);
        HRESULT STDMETHODCALLTYPE OnSimpleVolumeChanged(_In_ float NewVolume, _In_ BOOL NewMute, LPCGUID EventContext);
        HRESULT STDMETHODCALLTYPE OnChannelVolumeChanged(_In_ DWORD ChannelCount, _In_reads_(ChannelCount) float NewChannelVolumeArray[], _In_ DWORD ChangedChannel, LPCGUID EventContext);
        HRESULT STDMETHODCALLTYPE OnGroupingParamChanged(_In_ LPCGUID NewGroupingParam,/* [in] */ LPCGUID EventContext);
        HRESULT STDMETHODCALLTYPE OnStateChanged(_In_ AudioSessionState NewState);
        HRESULT STDMETHODCALLTYPE OnSessionDisconnected(_In_ AudioSessionDisconnectReason DisconnectReason);
#endif
        MemoryAllocator& mAllocator;
        uint32_t mSampleRate;
        LONG64 mPosition;//number of consumed samples. TOTAL!
        HANDLE mThread;
        HANDLE onTerminate;
        HANDLE onNeedMoreSamples;   //device needs more samples... (wasapi event)
        HANDLE onDeviceChanged;     //device changed/lost
        HANDLE onCommand;
        HANDLE onFlushDone;         //Triggered when flush command has completed.
        Mutex mCommandQueueMutex;
        FixedQueue<uint32_t, 64> mCommandQueue;
        IMMDeviceEnumerator *mEnumerator;
        IMMDevice* mDevice;
        static DWORD WINAPI threadEntry(LPVOID param);
        DWORD EventHandler();
        bool_t mPlaying;
        bool_t mSharedMode;
        Endpoint* mEndpoint;
        Mutex mMutex;
        uint64_t mBaseTime;
        AudioRendererAPI* mAudio;
        wchar_t mRequestedDevice[512];
        wchar_t mCurrentDevice[512];
    };
}
OMAF_NS_END
