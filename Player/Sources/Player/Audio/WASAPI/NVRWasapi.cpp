
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
#define WASAPI_LOGS
#include "NVRWasapi.h"
#include <NVRNamespace.h>
#include "Context.h"
#include "Endpoint.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRNew.h"


OMAF_NS_BEGIN

OMAF_LOG_ZONE(WasapiBackend)
using namespace WASAPIImpl;
WASAPIBackend::WASAPIBackend()
    : mContext(OMAF_NULL)
    , mAllocator(*MemorySystem::DefaultHeapAllocator())
{
    write_message("%s", __FUNCTION__);
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    mContext = OMAF_NEW(mAllocator, WASAPIImpl::Context)(mAllocator);
}

WASAPIBackend::~WASAPIBackend()
{
    write_message("%s", __FUNCTION__);
    // stop();
    OMAF_DELETE(mAllocator, mContext);
    CoUninitialize();
}

void WASAPIBackend::init(AudioRendererAPI* renderer, bool_t allowExclusiveMode, const wchar_t* audioDevice)
{
    Mutex::ScopeLock cs(mContext->mMutex);
    write_message("%s", __FUNCTION__);

    // Since changing the audiorenderer while playing might be a "BADish" thing.
    mContext->mSharedMode = !allowExclusiveMode;
    mContext->mAudio = renderer;
    mContext->mCurrentDevice[0] = 0;
    mContext->mRequestedDevice[0] = 0;

    audioDevice = L"{0.0.0.00000000}.{0680192c-0a61-4581-a5a4-9a707b777986}";
    if (audioDevice != NULL)
    {
        wcsncpy(mContext->mRequestedDevice, audioDevice, 512);
    }
}

void WASAPIBackend::shutdown()
{
    write_message("%s", __FUNCTION__);

    // This method is not called from anywhere!
    OMAF_ASSERT_NOT_IMPLEMENTED();
    OMAF_ISSUE_BREAK();
}

void WASAPIBackend::onErrorOccurred(OMAF_NS::Error::Enum err)
{
    write_message("%s", __FUNCTION__);
    // In what case is this called, and what should be done here? stop playback?
    err = err;
}

int64_t WASAPIBackend::onGetPlayedTimeUs()
{
    Mutex::ScopeLock cs(mContext->mMutex);

    // why is onGetPlayedTimeUs called when paused/stopped?
    UINT64 position = 0;

    if (mContext->mEndpoint)
    {
        OMAF_ASSERT(mContext->mEndpoint != NULL, "mContext is NULL in onGetPlayedTimeUs!");
        position = mContext->mEndpoint->position() + mContext->mBaseTime;
    }

    return position;
}

void WASAPIBackend::onFlush()
{
    write_message("%s", __FUNCTION__);
    mContext->pushCommand(WASAPICommandEvent::FLUSH);

    DWORD ret = WaitForSingleObject(mContext->onFlushDone, INFINITE);  // Wait until flush has been completed.
    OMAF_ASSERT(ret == WAIT_OBJECT_0, "WASAPI onFlush wait failed!");
}

void_t WASAPIBackend::onRendererPaused()
{
    write_message("%s", __FUNCTION__);
    mContext->pushCommand(WASAPICommandEvent::PAUSED);
}

void WASAPIBackend::onRendererReady()
{
    write_message("%s", __FUNCTION__);
    mContext->pushCommand(WASAPICommandEvent::READY);
}

void WASAPIBackend::onRendererPlaying()
{
    write_message("%s", __FUNCTION__);
    mContext->pushCommand(WASAPICommandEvent::PLAYING);
}

OMAF_NS_END
