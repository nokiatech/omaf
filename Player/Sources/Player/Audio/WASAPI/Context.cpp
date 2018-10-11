
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
#include "Context.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"
#include "Foundation/NVRLogger.h"

#include "Endpoint.h"
#include "Audio/NVRAudioRendererAPI.h"
#include "WasapiDevice.h"
#include "NullDevice.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(Context)


#define SAFE_RELEASE(punk) if ((punk) != NULL) { (punk)->Release(); (punk) = NULL; }
#define SAFE_CLOSE(hndl) if (hndl != NULL) {CloseHandle(hndl);hndl=NULL;}
#define SAFE_DELETE(ptr) OMAF_DELETE(mAllocator,ptr);ptr=NULL;
#include <avrt.h>
#include <Functiondiscoverykeys_devpkey.h>
#pragma comment(lib,"Avrt.lib")

#ifdef WASAPI_LOGS
#include <TimeApi.h>
#endif

namespace WASAPIImpl
{

#ifdef WASAPI_LOGS
    void write_message(const char* format, ...)
    {
        va_list copy;
        va_start(copy, format);
        //re-entrant, but wasteful
        char buf2[1024] = { 0 };
        char buf[sizeof(buf2)] = { 0 };
        ::vsnprintf(buf, sizeof(buf), format, copy);
        static DWORD last = 0;
        DWORD now = timeGetTime();
        if (last == 0) last = now;
        int32_t dlt = now - last;
        _snprintf(buf2, sizeof(buf2) - 1, "%d:[%x] %s\n", dlt, GetCurrentThreadId(), buf);
        OutputDebugStringA(buf2);
        va_end(copy);
        //OMAF_LOG_V(buf);

    }
#endif

    static void NameThread(const char* base, DWORD id)
    {
#ifdef _DEBUG
        //https://msdn.microsoft.com/en-us/library/xcb2z8hs(v=vs.140).aspx
        const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
#pragma pack(pop)
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        char buf[128];
        sprintf(buf, "%s%X", base, id);
        info.szName = buf;
        info.dwThreadID = id;
        info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
        __try {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
        }
#pragma warning(pop)
#endif
    }

Context::Context(MemoryAllocator& allocator) :
    mSampleRate(0),
    mPosition(0),
    mThread(NULL),
    onTerminate(NULL),
    onNeedMoreSamples(NULL),
    onDeviceChanged(NULL),
    onCommand(NULL),
    mEnumerator(NULL),
    mDevice(NULL),
    mPlaying(false),
    mSharedMode(false),
    mEndpoint(NULL),
    mBaseTime(0),
    mAudio(NULL),
    mAllocator(allocator)
{
    mRequestedDevice[0]=mCurrentDevice[0] = 0;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&mEnumerator);
    OMAF_ASSERT(SUCCEEDED(hr), "Creation of DeviceEnumerator failed!");
    //listen to events.
    mEnumerator->RegisterEndpointNotificationCallback(this);
    onTerminate = CreateEvent(NULL, false, false, NULL);// "OnTerminate");
    onNeedMoreSamples = CreateEvent(NULL, false, false, NULL);// "OnNeedMoreSamples");
    onDeviceChanged = CreateEvent(NULL, false, false, NULL);// "onDeviceChanged");
    onCommand = CreateEvent(NULL, false, false, NULL);
    //completion events.
    onFlushDone = CreateEvent(NULL, false, false, NULL);
    DWORD id;
    mThread = CreateThread(NULL, 0, &threadEntry, this, CREATE_SUSPENDED, &id);
    OMAF_ASSERT(mThread != NULL, "Creation of WASAPI event handling thread failed.");
    NameThread("EventThread", id);
    ResumeThread(mThread);
    MemoryZero(mRequestedDevice, sizeof(mRequestedDevice));
}

Context::~Context()
{
    if (mThread)
    {
        SetEvent(onTerminate);
        DWORD res = WaitForSingleObject(mThread, INFINITE);
        OMAF_ASSERT(res == WAIT_OBJECT_0, "WASAPI Event handling thread termination wait failed!");
    }
    OMAF_ASSERT(mEndpoint == NULL, "WASAPI Endpoint not NULL after thread termination.");
    if (mEnumerator)
    {
        mEnumerator->UnregisterEndpointNotificationCallback(this);
    }
    SAFE_RELEASE(mEnumerator);
    SAFE_CLOSE(mThread);
    SAFE_CLOSE(onTerminate);
    SAFE_CLOSE(onNeedMoreSamples);
    SAFE_CLOSE(onDeviceChanged);
    SAFE_CLOSE(onFlushDone);
}

//IUnknown.
ULONG STDMETHODCALLTYPE Context::AddRef()
{
    return 1;
}

ULONG STDMETHODCALLTYPE Context::Release()
{
    return 1;
}

HRESULT STDMETHODCALLTYPE Context::QueryInterface(REFIID riid, _COM_Outptr_ void __RPC_FAR *__RPC_FAR *ppvInterface)
{    
    if (IID_IUnknown == riid)
    {
        AddRef();
        *ppvInterface = (IUnknown*)this;
    }
    else if (__uuidof(IMMNotificationClient) == riid)
    {
        AddRef();
        *ppvInterface = (IMMNotificationClient*)this;
    }
    else
    {
        *ppvInterface = NULL;
        return E_NOINTERFACE;
    }
    return S_OK;
}
#if 0
HRESULT STDMETHODCALLTYPE Context::OnDisplayNameChanged(_In_ LPCWSTR NewDisplayName,/* [in] */ LPCGUID EventContext)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnIconPathChanged(_In_ LPCWSTR NewIconPath,/* [in] */ LPCGUID EventContext)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnSimpleVolumeChanged(_In_ float NewVolume, _In_ BOOL NewMute, LPCGUID EventContext)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnChannelVolumeChanged(_In_ DWORD ChannelCount, _In_reads_(ChannelCount) float NewChannelVolumeArray[], _In_ DWORD ChangedChannel, LPCGUID EventContext)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnGroupingParamChanged(_In_ LPCGUID NewGroupingParam,/* [in] */ LPCGUID EventContext)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnStateChanged(_In_ AudioSessionState NewState)
{
    /*const char *pszState[] = { "inactive","active","expired" };
    if ((NewState >= AudioSessionStateInactive) && (NewState <= AudioSessionStateExpired))
    write_message("New session state = %s\n", pszState[NewState]);
    else
    write_message("New session state = Unknown\n");*/
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnSessionDisconnected(_In_ AudioSessionDisconnectReason DisconnectReason)
{
    /*const char* pszReason[] = { "device removed","server shut down","format changed","user logged off","session disconnected","exclusive-mode override" };
    if ((DisconnectReason >= DisconnectReasonDeviceRemoval) && (DisconnectReason <= DisconnectReasonExclusiveModeOverride))
    write_message("Audio session disconnected (reason: %s)\n", pszReason[DisconnectReason]);
    else
    write_message("Audio session disconnected (reason: Unknown)\n");*/
    return S_OK;
}
#endif
bool validDevice(DWORD aState)
{
    if (aState == DEVICE_STATE_ACTIVE)
    {
        return true;
    }
    return false;
}
HRESULT STDMETHODCALLTYPE Context::OnDeviceStateChanged(_In_ LPCWSTR pwstrDeviceId, _In_ DWORD dwNewState)
{
    if (dwNewState == DEVICE_STATE_ACTIVE)
    {
        if (wcscmp(pwstrDeviceId, mRequestedDevice) == 0)
        {
            //our requested device is now enabled.. trigger device change.. 
            SetEvent(onDeviceChanged);
        }
        else
        {
            if (mCurrentDevice[0] == 0)
            {
                SetEvent(onDeviceChanged);
            }
        }
    }
    if (dwNewState == DEVICE_STATE_DISABLED)
    {
        dwNewState = dwNewState;
        if (wcscmp(pwstrDeviceId, mCurrentDevice) == 0)
        {
            //our requested device is now disabled.. trigger device change.. 
            SetEvent(onDeviceChanged);
        }
    }
    if (dwNewState == DEVICE_STATE_NOTPRESENT)
    {
        dwNewState = dwNewState;
        if (wcscmp(pwstrDeviceId, mCurrentDevice) == 0)
        {
            //our requested device is now disabled.. trigger device change.. 
            SetEvent(onDeviceChanged);
        }
    }
    if (dwNewState == DEVICE_STATE_UNPLUGGED)
    {
        dwNewState = dwNewState;
        if (wcscmp(pwstrDeviceId, mCurrentDevice) == 0)
        {
            //our requested device is now disabled.. trigger device change.. 
            SetEvent(onDeviceChanged);
        }
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnDeviceAdded(_In_ LPCWSTR pwstrDeviceId)
{
    if (wcscmp(pwstrDeviceId, mRequestedDevice) == 0)
    {
        //our requested device got added back trigger device change..
        SetEvent(onDeviceChanged);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnDeviceRemoved(_In_ LPCWSTR pwstrDeviceId)
{    
    if (wcscmp(pwstrDeviceId, mCurrentDevice) == 0)
    {
        //our current device got removed.. trigger device change.. 
        SetEvent(onDeviceChanged);
    }    
    return S_OK;
}

HRESULT STDMETHODCALLTYPE Context::OnPropertyValueChanged(_In_ LPCWSTR pwstrDeviceId, _In_ const PROPERTYKEY key)
{
    return S_OK;
}

static HRESULT _PrintDeviceName(IMMDeviceEnumerator* aEnumerator, LPCWSTR pwstrId)
{
    HRESULT hr = S_OK;
    IMMDevice *pDevice = NULL;
    IPropertyStore *pProps = NULL;
    PROPVARIANT varString;
    CoInitialize(NULL);
    PropVariantInit(&varString);
    hr = aEnumerator->GetDevice(pwstrId, &pDevice);
    if (hr == S_OK)
    {
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
    }
    if (hr == S_OK)
    {
        // Get the endpoint device's friendly-name property.
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
    }
    write_message("----------------------\nDevice name: \"%S\"\n"" Endpoint ID string: \"%S\"\n", (hr == S_OK) ? varString.pwszVal : L"null device", (pwstrId != NULL) ? pwstrId : L"null ID");
    PropVariantClear(&varString);
    SAFE_RELEASE(pProps);
    SAFE_RELEASE(pDevice);
    CoUninitialize();
    return hr;
}

HRESULT STDMETHODCALLTYPE Context::OnDefaultDeviceChanged(_In_ EDataFlow flow, _In_ ERole role, _In_ LPCWSTR pwstrDefaultDeviceId)
{
    //We only use the default device changed events, since we only play through the default device...
    if ((flow == eRender) && (role == eConsole))
    {
        //trigger device change event for our thread.
        SetEvent(onDeviceChanged);        
    }
    return S_OK;
}

void Context::pushCommand(uint32_t aCmd)
{
    static size_t cm = 0;
    mCommandQueueMutex.lock();
    if (mCommandQueue.getSize() == mCommandQueue.getCapacity())
    {
        //CommandQueue is full, spin here until there is space.
        write_message("CommandQueue full BLOCK!");
        mCommandQueueMutex.unlock();
        int spins = 0;
        for (;;)
        {
            spins++;
            mCommandQueueMutex.lock();
            if (mCommandQueue.getSize() < mCommandQueue.getCapacity())
            {
                break;
            }
            mCommandQueueMutex.unlock();
            Sleep(0);
        }
        write_message("CommandQueue full wait done! %d", spins);
    }
    mCommandQueue.push(aCmd);
    if (mCommandQueue.getSize() > cm)
    {
        cm = mCommandQueue.getSize();
        write_message("MaxCommandQueue length %d", cm);
    }
    SetEvent(onCommand);
    mCommandQueueMutex.unlock();
}

DWORD Context::EventHandler()
{
    //commands/events sorted by priority...
    //events will always be handled in this order. (NOT in the order they are sent!)
    //the order of handles in the array MUST match the order of command enum...
    HANDLE handles[] =
    {
        onTerminate,        //terminate request is always handled first
        onDeviceChanged,    //then device change events.
        onCommand,          //then all the commands (stop,pause,play,flush)
        onNeedMoreSamples,  //and finally sample requests by the device.
    };

    DWORD taskIndex = 0;
    HANDLE hTask = NULL;
    hTask = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &taskIndex);
    mBaseTime = 0;
    bool_t Started = false;
    bool_t DontRender = true;
    bool_t Terminate = false;
    for (;;)
    {
        if (Terminate)
        {
            break;
        }

        DWORD ret = WaitForMultipleObjects(OMAF_ARRAY_SIZE(handles), handles, false, INFINITE);
        if (ret == WAIT_FAILED)
        {
            //Terminate the thread on Wait failed. should not happen. EVER.
            DWORD err = GetLastError();
            write_message("Wait failed:%x", err);
            break;
        }

        if (ret == WASAPICommandEvent::COMMAND)
        {
            mCommandQueueMutex.lock();
            OMAF_ASSERT(!mCommandQueue.isEmpty(), "WASAPI CommandQueue is empty!");
            ret = mCommandQueue.front();
            mCommandQueue.pop();
            if (!mCommandQueue.isEmpty())
            {
                //keep the event triggered, since we have more commands to process.
                SetEvent(onCommand);
            }
            mCommandQueueMutex.unlock();
        }

        bool_t start = Started;
        {
            Mutex::ScopeLock cs(mMutex);
            switch (ret)
            {
                case WASAPICommandEvent::NEED_MORE_SAMPLES:
                {
                    //onNeedMoreSamples
                    //write_message("onNeedMoreSamples position(%lld) when mPlaying(%d) mEndpoint(%p) DontRender(%d)",mEndpoint->position(), (int32_t)mPlaying, mEndpoint,(int32_t)DontRender);
                    if (mPlaying && mEndpoint)
                    {
                        //write_message("onNeedMoreSamples:%lld");
                        if (!DontRender)
                        {
                            mEndpoint->newsamples(mAudio, false);
                        }
                        else
                        {
                            write_message("onNeedMoreSamples with DontRender!");
                        }
                    }
                    break;
                }
                case WASAPICommandEvent::PLAYING:
                {
                    //onNewSamplesAvailable.
                    if (mEndpoint)
                    {
                        OMAF_ASSERT(!mPlaying, "Play called while already playing");
                        mPlaying = true;
                        //write_message("onNewSamplesAvailable");
                        OMAF_ASSERT(mEndpoint != NULL, "mEndpoint is NULL in onRendererPlaying!");
                        DontRender = false;
                        if (mEndpoint->newsamples(mAudio, true))
                        {
                            start = true;
                        }
                    }
                    else
                    {
                        //why is onNewSamplesAvailable called when paused/stopped?
                        write_message("onRendererPlaying when mPlaying(%d) mEndpoint(%p)", (int32_t)mPlaying, mEndpoint);
                    }
                    break;
                }
                case WASAPICommandEvent::PAUSED:
                {
                    OMAF_ASSERT(mPlaying, "Pausing while already paused");
                    //onRendererPaused
                    write_message("onRendererPaused mEndpoint(%p)", mEndpoint);
                    if (mEndpoint)
                    {
                        mEndpoint->pause();
                    }
                    mPlaying = false;
                    DontRender = true;
                    break;
                }
                case WASAPICommandEvent::FLUSH:
                {
                    //onFlush
                    OMAF_ASSERT(!mPlaying, "Flushing while playing");
                    write_message("onFlush mEndpoint(%p)", mEndpoint);
                    if (mEndpoint)
                    {
                        mEndpoint->stop();
                    }
                    start = Started = false;
                    mPosition = 0;
                    mBaseTime = 0;
                    DontRender = true;
                    SetEvent(onFlushDone);
                    break;
                }
                case WASAPICommandEvent::TERMINATE:
                {
                    //onTerminate
                    write_message("onTerminate mEndpoint(%p)", mEndpoint);
                    SAFE_DELETE(mEndpoint);
                    SAFE_RELEASE(mDevice);
                    start = Started = false;
                    DontRender = true;
                    Terminate = true;
                    break;
                }
                case WASAPICommandEvent::READY:
                {
                    OMAF_ASSERT(!mPlaying, "READY while playing");
                    if (mEndpoint)
                    {
                        mEndpoint->stop();
                    }
                    SAFE_DELETE(mEndpoint);
                    SAFE_RELEASE(mDevice);
                    start = Started = false;
                    DontRender = true;
                    mEndpoint = NULL;

                    write_message("onRendererReady mEndpoint(%p)", mEndpoint);
                    OMAF_ASSERT(mDevice == NULL, "mDevice != NULL in 'onRendererReady'");
                    if (wcslen(mRequestedDevice) != 0)
                    {
                        mEnumerator->GetDevice(mRequestedDevice, &mDevice);
                        if (mDevice)
                        {
                            DWORD state;
                            mDevice->GetState(&state);
                            if (!validDevice(state))
                            {
                                mDevice->Release();
                                mDevice = NULL;
                            }
                        }
                    }
                    if (mDevice == NULL)
                    {
                        mEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &mDevice);//If no audiodevice "ERROR_NOT_FOUND"
                        if (mDevice)
                        {
                            DWORD state;
                            mDevice->GetState(&state);
                            if (!validDevice(state))
                            {
                                mDevice->Release();
                                mDevice = NULL;
                            }
                        }
                    }
                    if (mDevice == NULL)
                    {
                        mCurrentDevice[0] = 0;
                        mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::NullDevice)(this);
                    }
                    else
                    {                  
                        LPWSTR dev = NULL;
                        mDevice->GetId(&dev);
                        memcpy(mCurrentDevice, dev, (wcslen(dev)+1) * 2);
                        CoTaskMemFree(dev);
                        mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::WASAPIDevice)(this);
                    }
                    mSampleRate = mAudio->getInputSampleRate();

                    if (!mEndpoint->init(mSampleRate, 2))
                    {
                        //okay.. init failed. drop to null device
                        SAFE_DELETE(mEndpoint);
                        SAFE_RELEASE(mDevice);
                        mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::NullDevice)(this);
                        mEndpoint->init(mAudio->getInputSampleRate(), 2);//null device cant fail
                    }

                    start = Started = false;
                    mBaseTime = 0;
                    mPosition = 0;
                    // TODO REMOVE: mAudio->setAudioLatency(mEndpoint->latencyUs());
                    break;
                }
                case WASAPICommandEvent::DEVICE_CHANGED:
                {
                    //device changed..
                    if (mEndpoint)
                    {
                        IMMDevice* newdevice = NULL;
                     //   HRESULT hr = mEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &newdevice);//If no audiodevice "ERROR_NOT_FOUND"
                        HRESULT hr;
                        if (wcslen(mRequestedDevice) != 0)
                        {
                            //we have a device request..
                            //try to get it..
                            hr = mEnumerator->GetDevice(mRequestedDevice, &newdevice);
                            if (newdevice)
                            {
                                //got it, check if it is valid..
                                DWORD state;
                                newdevice->GetState(&state);
                                if (!validDevice(state))
                                {
                                    //no..
                                    newdevice->Release();
                                    newdevice = NULL;
                                }
                            }
                        }
                        if (newdevice == NULL)
                        {
                            hr = mEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &newdevice);//If no audiodevice "ERROR_NOT_FOUND"
                            if (newdevice)
                            {
                                DWORD state;
                                newdevice->GetState(&state);
                                if (!validDevice(state))
                                {
                                    newdevice->Release();
                                    newdevice = NULL;
                                }
                            }
                        }

                        wchar_t dev[512];
                        dev[0] = 0;
                        if (newdevice)
                        {
                            //got device, get it's id..
                            LPWSTR devt = NULL;
                            newdevice->GetId(&devt);
                            memcpy(dev, devt, (wcslen(devt) + 1) * 2);
                            CoTaskMemFree(devt);
                        }
                        
                        if (wcscmp(dev,mCurrentDevice)==0)
                        {
                            //no need to do anything, since the device we got is the device we had...
                            SAFE_RELEASE(newdevice);//we actually dont need it..
                        }
                        else
                        {
                            //save our "current position"
                            if (mDevice == NULL)
                            {
                                //update the final position of the null device..
                                mEndpoint->newsamples(mAudio, true);
                            }

                            LONG64 curRendPosition = mPosition;
                            curRendPosition *= 1000000;
                            curRendPosition /= (int64_t)mSampleRate;
                            LONG64 curPlayPosition = mEndpoint->position();//ignore this..
                            mBaseTime = curRendPosition;// mEndpoint->position();

                            //nuke old device.
                            SAFE_DELETE(mEndpoint);
                            SAFE_RELEASE(mDevice);

                            //create new one.
                            mDevice = newdevice;
                            if (mDevice == NULL)
                            {
                                mCurrentDevice[0] = 0;
                                mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::NullDevice)(this);
                            }
                            else
                            {
                                memcpy(mCurrentDevice, dev, (wcslen(dev) + 1) * 2);
                                mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::WASAPIDevice)(this);
                            }
                            if (!mEndpoint->init(mAudio->getInputSampleRate(), 2))
                            {
                                SAFE_RELEASE(mDevice);
                                mEndpoint = OMAF_NEW(mAllocator, WASAPIImpl::NullDevice)(this);
                                mEndpoint->init(mAudio->getInputSampleRate(), 2);//null device cant fail
                            }
                            // TODO REMOVE: mAudio->setAudioLatency(mEndpoint->latencyUs());
                            if (!DontRender)
                            {
                                if (mPlaying&&mEndpoint->newsamples(mAudio, true))//render samples ONLY if playing..
                                {
                                    start = true;
                                }
                            }
                        }
                    }
                    break;
                }
            }
        }
        if (start)
        {
            //notify audiorenderer that we have started playback now..
            if (!Started)
            {
                Started = true;
                mAudio->playbackStarted();
            }
        }
    }
    OMAF_ASSERT(mEndpoint == NULL, "WASAPI endpoint not null during thread end.");
    OMAF_ASSERT(mDevice == NULL, "WASAPI device not null during thread end.");
    if (hTask)
    {
        AvRevertMmThreadCharacteristics(hTask);
    }
    return 0;
}

DWORD WINAPI Context::threadEntry(LPVOID param)
{
    DWORD ret;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    ret = ((WASAPIImpl::Context*)param)->EventHandler();
    CoUninitialize();
    return ret;
}

}
OMAF_NS_END
