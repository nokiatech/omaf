
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
#pragma comment(lib,"winmm.lib")
#include "Platform/OMAFPlatformDetection.h"
#define NOMINMAX
#define INITGUID
#include <mfapi.h>
#include <mftransform.h>
#include <Mferror.h>
#include <Mfidl.h>
#include <wmcodecdsp.h>
#include <codecapi.h>
#include <d3d11.h>
#include <d3d9.h>
#include <evr.h>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mfuuid.lib")
#pragma comment(lib,"d3d11.lib")
#pragma comment(lib,"Mf.lib")
#include <NVRGraphics.h>
#include "VideoDecoder/Windows/NVRMediaFoundationDecoderHW.h"
//#define OMAF_ENABLE_LOG 1
//#define ENABLE_FPS_LOGGING 0
#define ENABLE_FPS_LOGGING 1

#include "Foundation/NVRLogger.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRDeviceInfo.h"

//static const uint32_t BUFFERING_THRESHOLD = 15;
//static const int64_t SECOND_AS_MICROSECONDS = 1000000LL; // 1 sec.
//static const int64_t PTS_ADAPTATION_THRESHOLD_DIFFERENCE_US = SECOND_AS_MICROSECONDS;

OMAF_NS_BEGIN
OMAF_LOG_ZONE(VideoDecoderMediaFoundation)
//#define OMAF_DUMP_DECODER_INFO 1
#if OMAF_DUMP_DECODER_INFO
#include "KnownGuids.h"
#endif

Mutex MediaFoundationDecoderHW::sContextMutex;
IMFDXGIDeviceManager* MediaFoundationDecoderHW::sDevMan = OMAF_NULL;
CreateDXGIDeviceManagerPROC MediaFoundationDecoderHW::sCreateDXGIDeviceManager = OMAF_NULL;
ID3D11Device* MediaFoundationDecoderHW::sDev = OMAF_NULL;
UINT MediaFoundationDecoderHW::sResetToken = 0;
uint32_t MediaFoundationDecoderHW::sInstanceCount = 0;
HMODULE MediaFoundationDecoderHW::sMFModule = OMAF_NULL;
ID3D11DeviceContext* MediaFoundationDecoderHW::sContext = OMAF_NULL;
#if OMAF_DEBUG_BUILD
ID3D11Debug* MediaFoundationDecoderHW::sDebug = OMAF_NULL;
#endif
ID3D10Multithread* MediaFoundationDecoderHW::sMt = OMAF_NULL;
ID3D11Device* MediaFoundationDecoderHW::getDecoderDevice()
{
    MediaFoundationDecoderHW::InitContexts();
    return sDev;
}
void MediaFoundationDecoderHW::releaseDecoderDevice()
{
    MediaFoundationDecoderHW::ReleaseContexts();
}
ID3D11DeviceContext* MediaFoundationDecoderHW::lockDecoderContext()
{
    if (sMt) sMt->Enter();
    sContext->AddRef();
    return sContext;
}
void MediaFoundationDecoderHW::unlockDecoderContext(ID3D11DeviceContext* context)
{
    OMAF_ASSERT(context == sContext, "invalid Decodercontext");
    sContext->Release();
    if (sMt) sMt->Leave();
}
MediaFoundationDecoderHW::MediaFoundationDecoderHW(FrameCache& frameCache)
    : mState(DecoderHWState::INVALID)
    , mInputEOS(false)
    , mCodec(OMAF_NULL)
    , mOutputSample(OMAF_NULL)
    , mInputType(OMAF_NULL)
    , mOutputType(OMAF_NULL)
    , mFirstPTS(-1)
    , mFrameCache(frameCache)
    , mUnusedInputBuffer(OMAF_NULL)
    , mQueuedFrameCount(0)
    , mNeedMoreData(true)
{
    mGraphicsBackendType = RenderBackend::getRendererType();
}

MediaFoundationDecoderHW::~MediaFoundationDecoderHW()
{
    OMAF_ASSERT(mCodec == OMAF_NULL, "Shutdown not called");
}

bool_t MediaFoundationDecoderHW::isEOS()
{
    if (mState == DecoderHWState::STARTED && mInputEOS)
    {
        isFrameDecoded();
    }
    return mState == DecoderHWState::EOS;
}

const DecoderConfig& MediaFoundationDecoderHW::getConfig() const
{
    return mDecoderConfig;
}

void_t MediaFoundationDecoderHW::setStreamId(streamid_t streamId)
{
    mDecoderConfig.streamId = streamId;
}

Error::Enum MediaFoundationDecoderHW::createInstance(const MimeType& mimeType)
{
    OMAF_ASSERT(mState == DecoderHWState::INVALID, "Incorrect state");
    mMimeType = mimeType;
    mState = DecoderHWState::IDLE;
    return Error::OK;
}

Error::Enum MediaFoundationDecoderHW::initialize(const DecoderConfig& config)
{
    //With D3D11 we MUST have the d3d device.
    if ((sDev == OMAF_NULL) && (mGraphicsBackendType == RendererType::D3D11))
    {
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }
    OMAF_ASSERT(mState == DecoderHWState::IDLE, "Incorrect state");
    OMAF_ASSERT(config.mimeType == mMimeType, "Incorrect mimetype");
    mDecoderConfig = config;
    HRESULT res;

    GUID format;
    //IID decoder;
    if (mMimeType == VIDEO_H264_MIME_TYPE)
    {
        format = MFVideoFormat_H264;
        //decoder = __uuidof(CMSH264DecoderMFT);
    }
    else if (mMimeType == VIDEO_HEVC_MIME_TYPE)
    {
        format = MFVideoFormat_HEVC;
        //the CLSID_s should match.. but since it might not be defined (and it is not if building against 8.x or older sdk.)
        //decoder = CLSID_MSH265DecoderMFT;
        //class __declspec(uuid("{420A51A3-D605-430C-B4FC-45274FA6C562}")) _CLSID_CMSH265DecoderMFT;
        //const CLSID& CLSID_CMSH265DecoderMFT = __uuidof(_CLSID_CMSH265DecoderMFT);
        //decoder = CLSID_CMSH265DecoderMFT;
    }
    else
    {
        OMAF_LOG_E("INVALID MIME_TYPE");
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }

    //res = CoCreateInstance(decoder, OMAF_NULL, CLSCTX_INPROC, IID_PPV_ARGS(&mCodec));
    MFT_REGISTER_TYPE_INFO InputTypeInformation = { MFMediaType_Video, format };
    IMFActivate** ppActivates;
    UINT32 nActivateCount = 0;
    //fetch hardware MFT's
    //This flag applies to video codecs and video processors that perform their work entirely in hardware. It does not apply to software decoders that use DirectX Video Acceleration to assist decoding.
    res = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, MFT_ENUM_FLAG_HARDWARE | MFT_ENUM_FLAG_SORTANDFILTER, &InputTypeInformation, NULL, &ppActivates, &nActivateCount);
    if (nActivateCount == 0)
    {
        //okay, fetch async software MFT's
        res = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, MFT_ENUM_FLAG_ASYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER, &InputTypeInformation, NULL, &ppActivates, &nActivateCount);
    }
    if (nActivateCount == 0)
    {
        //last ditch effort, fetch sync software MFT's
        res = MFTEnumEx(MFT_CATEGORY_VIDEO_DECODER, MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_SORTANDFILTER, &InputTypeInformation, NULL, &ppActivates, &nActivateCount);
        //AAAND it seems this is what we get on NVIDIA...
        //this might still be DX video acceleration..
    }

    if (nActivateCount == 0)
    {
        OMAF_LOG_E("No decoders available for specified format!");
        releaseDecoderResources();
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }
#if OMAF_DEBUG_BUILD && OMAF_DUMP_DECODER_INFO
    for (UINT32 n = 0; n < nActivateCount; ++n)
    {
        IMFAttributes* attr;
        ppActivates[n]->QueryInterface(&attr);
        DumpType(attr);
        /*UINT32 length;
        attr->GetStringLength(MFT_FRIENDLY_NAME_Attribute, &length);
        std::wstring name(64+length + 1, '\0');
        attr->GetString(MFT_FRIENDLY_NAME_Attribute, &name[0], length + 1, nullptr);
        wprintf(L"Found %d : %s\n", n, name.c_str());
        OutputDebugStringW(name.c_str());*/
        attr->Release();
    }
#endif
    res=ppActivates[0]->ActivateObject(IID_PPV_ARGS(&mCodec));// __uuidof(IMFTransform), (VOID**)&mCodec);

    for (UINT32 i = 0; i < nActivateCount; i++)
    {
        ppActivates[i]->Release();
    }
    CoTaskMemFree(ppActivates);

    if (FAILED(res))
    {
        //very bad things.
        OMAF_LOG_E("Failed to create decoder instance %d", res);
        releaseDecoderResources();
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }
    //TODO HEVC decoder on GTX 980 HW seem to cause deadlock / jam with tile merging streams (980 does not have full HEVC support on chipset). Seem to work OK at least on GTX 1080.
    const FixedString256& gpu = DeviceInfo::getDevicePlatformId();
    if (sDevMan != OMAF_NULL && (gpu.findFirst("GTX 9") == Npos || (mMimeType == VIDEO_H264_MIME_TYPE)))
    {
        res = mCodec->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, reinterpret_cast<ULONG_PTR>(sDevMan));//this needs to be set to get HW decoding.
        OMAF_LOG_D("Create HW decoder");
    }
    else
    {
        OMAF_LOG_D("Create SW decoder");
    }

    IMFAttributes* codecattrb;
    res = mCodec->GetAttributes(&codecattrb);

    if (format == MFVideoFormat_H264)
    {
        UINT32 MaxW, MaxH;
        res = codecattrb->GetUINT32(CODECAPI_AVDecVideoMaxCodedWidth, &MaxW);
        res = codecattrb->GetUINT32(CODECAPI_AVDecVideoMaxCodedHeight, &MaxH);
        res = codecattrb->SetUINT32(CODECAPI_AVDecVideoAcceleration_H264, TRUE);
    }

    //Need to define this ourselves. (since it's actually only supported on win8+
    const GUID LOW_LATENCY = { 0x9c27891a, 0xed7a, 0x40e1, { 0x88, 0xe8, 0xb2, 0x27, 0x27, 0xa0, 0x24, 0xee } };
    // low latency is preferred with multiple-streams case as it gives more frames in the output to select from
    res = codecattrb->SetUINT32(LOW_LATENCY, true);

    //create input
    res = MFCreateMediaType(&mInputType);
    res = mInputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    res = mInputType->SetGUID(MF_MT_SUBTYPE, format);
    //        res = inputtype->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_MixedInterlaceOrProgressive);
    //        res = MFSetAttributeRatio(inputtype, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);//ummm..
    //        res = MFSetAttributeRatio(inputtype, MF_MT_FRAME_RATE, 1, format->frameRate());
    res = MFSetAttributeSize(mInputType, MF_MT_FRAME_SIZE, mDecoderConfig.width, mDecoderConfig.height);
    res = mCodec->SetInputType(0, mInputType, 0);//yeah well.

    if (res != S_OK)
    {
        //hmm.. failed. try to fall back to SW then.
        OMAF_LOG_V("Trying SW fallback since input was not accepted! %d", res);
        if (sDevMan)
        {
            res = mCodec->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER, OMAF_NULL);//Disconnect from HW.
            res = mCodec->SetInputType(0, mInputType, 0);//yeah well.
        }
    }
    // Doesn't work even with SW fallback, go to error state
    if (res != S_OK)
    {
        OMAF_LOG_E("Neither HW or SW accepted the input! %d", res);
        releaseDecoderResources();
        mState = DecoderHWState::ERROR_STATE;
        return Error::NOT_SUPPORTED;
    }

    //create output
    res = MFCreateMediaType(&mOutputType);
    res = mOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    res = mOutputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);
    res = MFSetAttributeSize(mOutputType, MF_MT_FRAME_SIZE, mDecoderConfig.width, mDecoderConfig.height);
    res = mCodec->SetOutputType(0, mOutputType, 0);//yeah well.

    //Check if codec provides...
    MFT_OUTPUT_STREAM_INFO infoO;
    res = mCodec->GetOutputStreamInfo(0, &infoO);
    if (infoO.dwFlags& (MFT_OUTPUT_STREAM_PROVIDES_SAMPLES | MFT_OUTPUT_STREAM_CAN_PROVIDE_SAMPLES))
    {
        //no need to create samples our selves..
        mOutputSample = OMAF_NULL;
    }
    else
    {
        //create output sample buffer...
        res = MFCreateSample(&mOutputSample);
        IMFMediaBuffer* OutputBuffer;
        if (infoO.cbAlignment == 0)
        {
            res = MFCreateMemoryBuffer(infoO.cbSize, &OutputBuffer);
        }
        else
        {
            res = MFCreateAlignedMemoryBuffer(infoO.cbSize, infoO.cbAlignment, &OutputBuffer);
        }
        res = mOutputSample->AddBuffer(OutputBuffer);
        OutputBuffer->Release();
    }

    res = mCodec->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0);
    res = mCodec->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);

    res = MFGetStrideForBitmapInfoHeader(MFVideoFormat_NV12.Data1, mDecoderConfig.width, &mStride);

    decode(0, 0, mDecoderConfig.configInfoData, mDecoderConfig.configInfoSize);
    mQueuedFrameCount = 0;
    mState = DecoderHWState::STARTED;
    mNeedMoreData = true;

    if (FAILED(res))
    {
        OMAF_LOG_E("Could not initialize decoder! %d", res);
        //very bad things.
        releaseDecoderResources();
        mState = DecoderHWState::ERROR_STATE;
        return Error::OPERATION_FAILED;
    }
    return Error::OK;
}

void MediaFoundationDecoderHW::InitContexts()
{
    Mutex::ScopeLock lock(sContextMutex);
    sInstanceCount++;
    if (sInstanceCount == 1)
    {
        sDevMan = OMAF_NULL;
        sDev = OMAF_NULL;
        sCreateDXGIDeviceManager = OMAF_NULL;
#if OMAF_DEBUG_BUILD
        sDebug = OMAF_NULL;
#endif
        sMt = OMAF_NULL;

        CoInitializeEx(OMAF_NULL, COINIT_MULTITHREADED);
        HMODULE mfplat = GetModuleHandle("MFPlat.dll");
        OMAF_ASSERT(mfplat != OMAF_NULL, "MFPlat not available!");//The app wont even start if MFPlat not installed, windows will pop up a dialog for the missing dll way before we come here.
        sCreateDXGIDeviceManager = (CreateDXGIDeviceManagerPROC)GetProcAddress(mfplat, "MFCreateDXGIDeviceManager");
        if (sCreateDXGIDeviceManager == OMAF_NULL)
        {
            //Okay must be win7 or something. since MFPlat.dll does not have the function.
            //try mshtmlmedia.dll then..
            FreeLibrary(sMFModule);
            sMFModule = LoadLibrary("mshtmlmedia.dll");
            if (sMFModule)
            {
                sCreateDXGIDeviceManager = (CreateDXGIDeviceManagerPROC)GetProcAddress(sMFModule, "MFCreateDXGIDeviceManager");
            }
            if (sCreateDXGIDeviceManager == OMAF_NULL)
            {
                //Okay no way to use HW then...
                if (sMFModule)
                {
                    FreeLibrary(sMFModule);
                }
                sMFModule = OMAF_NULL;
            }
        }
        HRESULT res;
        res = MFStartup(MF_VERSION, MFSTARTUP_LITE);
        if (FAILED(res))
        {
            //Could not startup Mediafoundation.
            return;
        }
        UINT Flags = D3D11_CREATE_DEVICE_VIDEO_SUPPORT;

        //Hmm.. it seems that 9.3 featurelevel is fastest for our video decoding uses?!?!?!
        //but... on win7 i get TDR's and GPU crashes constantly.. (that is when using d3d backend... with OGL we are fine...)
        //and on win7 we cant get videodecoder for 11.+... hrmph
        //And to be exact 9.3 device should not work at ALL, since IMFDXGIDeviceManager requires a 11.x device...
        //there is a different IDirect3DDeviceManager9 for dx9 devices...
        //but then again, we have a d3d11 device, only with featurelevel 9.3...
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
            //D3D_FEATURE_LEVEL_9_3 // (apparently sharing between FL 11.x and FL 9.3 fails badly..cause of crashes)
        };
        int count = sizeof(featureLevels) / sizeof(featureLevels[0]);
        D3D_FEATURE_LEVEL level;
        if (sCreateDXGIDeviceManager)
        {
            res = D3D11CreateDevice(OMAF_NULL, D3D_DRIVER_TYPE_HARDWARE, OMAF_NULL, Flags, featureLevels, count, D3D11_SDK_VERSION, &sDev, &level, &sContext);
        }
        else
        {
            res = E_NOINTERFACE;
        }

        if (FAILED(res))
        {
            //Oh well. create a regular device anyway. decode with software.
            Flags &= ~D3D11_CREATE_DEVICE_VIDEO_SUPPORT;
            res = D3D11CreateDevice(OMAF_NULL, D3D_DRIVER_TYPE_HARDWARE, OMAF_NULL, Flags, featureLevels, count, D3D11_SDK_VERSION, &sDev, &level, &sContext);
        }

        if (SUCCEEDED(res))
        {
            res = sDev->QueryInterface(__uuidof(sMt), (void**)&sMt);
            if (res == S_OK)
            {
                sMt->SetMultithreadProtected(true);
            }

            if (Flags & D3D11_CREATE_DEVICE_VIDEO_SUPPORT)
            {
                res = sCreateDXGIDeviceManager(&sResetToken, &sDevMan);
                if (SUCCEEDED(res))
                {
                    res = sDevMan->ResetDevice(sDev, sResetToken);
                    if (FAILED(res))
                    {
                        sDevMan->Release();
                        sDevMan = OMAF_NULL;
                    }
                }
            }
        }

#if OMAF_DEBUG_BUILD
        if (sDev)
        {
            // Initialize debug layer
            if (SUCCEEDED(sDev->QueryInterface(__uuidof(ID3D11Debug), (void**)&sDebug)))
            {
                ID3D11InfoQueue* infoQueue = OMAF_NULL;

                if (SUCCEEDED(sDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&infoQueue)))
                {
                    // Define severity levels
                    infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
                    infoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
                    // Define filters
                    D3D11_MESSAGE_ID hide[] =
                    {
                        D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                        // Add more message IDs here as needed
                    };

                    D3D11_INFO_QUEUE_FILTER infoQueueFilter;
                    ZeroMemory(&infoQueueFilter, sizeof(infoQueueFilter));
                    infoQueueFilter.DenyList.NumIDs = _countof(hide);
                    infoQueueFilter.DenyList.pIDList = hide;

                    infoQueue->AddStorageFilterEntries(&infoQueueFilter);

                    infoQueue->Release();
                    infoQueue = OMAF_NULL;
                }
            }
        }
#endif

    }
}

#if OMAF_GRAPHICS_API_D3D11
void MediaFoundationDecoderHW::copyFrame2(DecoderFrameDX11* frameDX11, D3D11_BOX &box, BYTE* src, int pWidth, int pHeight, int pitch)
{
    HRESULT hr;
    D3D11_MAPPED_SUBRESOURCE Y, UV;
    hr = sContext->Map(frameDX11->mStagingY, 0, D3D11_MAP_WRITE, 0, &Y);
    hr = sContext->Map(frameDX11->mStagingUV, 0, D3D11_MAP_WRITE, 0, &UV);

    int h = frameDX11->height;
    if (h > pHeight) h = pHeight;
    int w = frameDX11->width;
    if (w > pWidth) w = pWidth;
    unsigned char* sr = src;
    unsigned char* ds = (unsigned char*)Y.pData;
    for (int y = 0; y < h; y++)
    {
        memcpy(ds, sr, w);
        sr += pitch;
        ds += Y.RowPitch;
    }
    //UV is quarter sized..
    w /= 2;
    h /= 2;
    ds = (unsigned char*)UV.pData;
    sr = (unsigned char*)src + (pitch *pHeight);
    for (int y = 0; y < h; y++)
    {
        unsigned char* src = (unsigned char*)sr;
        unsigned int* dst = (unsigned int*)ds;
        if (frameDX11->mUVformat == TextureFormat::BGRA8)
        {
            //this is worst case..
            //expand to 32bits and position correctly (U = R and V=G)
            for (int x = 0; x < w; x++)
            {
                unsigned int c;
                c = (*src) << 16; src++;
                c |= (*src) << 8; src++;
                *dst = c;
                dst++;
            }
        }
        else if (frameDX11->mUVformat == TextureFormat::RG8)
        {
            //nice just copy.
            memcpy(dst, src, w * 2);
        }
        sr += pitch;
        ds += UV.RowPitch;
    }

    sContext->Unmap(frameDX11->mStagingY, 0);
    sContext->Unmap(frameDX11->mStagingUV, 0);
    sContext->CopyResource(frameDX11->mTextureY, frameDX11->mStagingY);
    sContext->CopyResource(frameDX11->mTextureUV, frameDX11->mStagingUV);
}

void MediaFoundationDecoderHW::copyFrame(DecoderFrameDX11* frameDX11, D3D11_BOX &box, BYTE* src, int pWidth, int pHeight, int pitch)
{
    //TODO: handle negative pitch. (inverted image). also use in opengl too?
    HRESULT hr;
    //pWidth/pHeight are source sizes.
    //pStride is source stride.
    //and the staging texture is mDecoderConfig.width * mDecoderConfig.height
    //box has the dst texture size.
    //we only need to fill the staging texture to that size..
    int dstWidth = mDecoderConfig.width;// box.right - box.left;
    int dstHeight = mDecoderConfig.height;// box.bottom - box.top;
    if (dstWidth > pWidth) dstWidth = pWidth;
    if (dstHeight > pHeight) dstHeight = pHeight;
    if (frameDX11->mStagingNV12)
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = sContext->Map(frameDX11->mStagingNV12, 0, D3D11_MAP_WRITE, 0, &mappedResource);
        if (SUCCEEDED(hr))
        {
            unsigned int po, po2;
            po = mappedResource.RowPitch*dstHeight;   //offset to staging UV plane
            po2 = pitch*pHeight;                   //offset to source UV plane
            if (pitch == mappedResource.RowPitch)
            {
                //same stride/pitch/rowlength. so we can do a simple copy.
                //TODO: i somehow remember seeing a comment about not touching the data between rows (ie. from width to stride).... but cant find it now.
                //ie. this might be invalid, even though works for me (tm)
                if (pHeight == dstHeight)
                {
                    //best case
                    memcpy(((BYTE*)mappedResource.pData), src, (dstHeight*pitch * 3) / 2);
                }
                else
                {
                    //need to skip padding between planes..
                    //Copy Y
                    memcpy(((BYTE*)mappedResource.pData), src, dstHeight*pitch);
                    //Copy UV
                    memcpy(((BYTE*)mappedResource.pData) + po, src + po2, (dstHeight / 2)*pitch);
                }
            }
            else
            {
                //need to copy row by row, since the row lengths/strides are different.
                //copy Y
                for (int y = 0; y < dstHeight; y++)
                {
                    memcpy(((BYTE*)mappedResource.pData) + y*mappedResource.RowPitch, src + y*pitch, dstWidth);
                }
                //copy UV
                for (int y = 0; y < dstHeight / 2; y++)
                {
                    memcpy(((BYTE*)mappedResource.pData) + po + (y*mappedResource.RowPitch), src + po2 + (y*pitch), dstWidth);
                }
            }
            sContext->Unmap(frameDX11->mStagingNV12, 0);
            sContext->CopySubresourceRegion(frameDX11->mTextureNV12, 0, 0, 0, 0, frameDX11->mStagingNV12, 0, &box);
        }
    }
    else
    {
        OMAF_LOG_D("Staging texture not created!");
    }
}
#endif

void MediaFoundationDecoderHW::ReleaseContexts()
{
    Mutex::ScopeLock lock(sContextMutex);
    OMAF_ASSERT(sInstanceCount > 0, "");
    sInstanceCount--;
    if (sInstanceCount == 0)
    {
        HRESULT res;
        if (sDevMan)
        {
            sDevMan->Release();
            sDevMan = OMAF_NULL;
        }
        if (sMt)
        {
            sMt->Release();
            sMt = OMAF_NULL;
        }
        if (sContext)
        {
            sContext->Release();
            sContext = OMAF_NULL;
        }
        if (sDev)
        {
            sDev->Release();
            sDev = OMAF_NULL;
        }
#if OMAF_DEBUG_BUILD
        if (sDebug)
        {
            sDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
            sDebug->Release();
            sDebug = OMAF_NULL;
        }
#endif
        res = MFShutdown();
        if (sMFModule)
        {
            FreeLibrary(sMFModule);
            sMFModule = OMAF_NULL;
        }
        sCreateDXGIDeviceManager = OMAF_NULL;
        CoUninitialize();
    }
}

void_t MediaFoundationDecoderHW::consumedFrame(DecoderFrame* frame)
{
    OMAF_ASSERT(frame != OMAF_NULL, "null frame in consumedFrame");
    OMAF_ASSERT(frame->consumed == false, "consumed frame in consumedFrame");
    frame->consumed = true; //since we dont actually care about it yet, and consume SHOULD only be called once per texture anyway.
#if OMAF_GRAPHICS_API_D3D11
    if (mGraphicsBackendType == RendererType::D3D11)
    {
        DecoderFrameDX11* dx11Frame = (DecoderFrameDX11*)frame;
        //OMAF_LOG_I("Consumed %p %p", dx11Frame->texture, dx11Frame->sharedTexture);
        if (dx11Frame->mQuery)
        {
            //wait for the copying to complete. (not sure if this is actually needed... but wont hurt i guess)
            int loops = 0;
            ID3D11DeviceContext* context;
            for (;;)
            {
                BOOL result = FALSE;
                context = MediaFoundationDecoderHW::lockDecoderContext();
                HRESULT res = context->GetData(dx11Frame->mQuery, &result, sizeof(result), 0);
                MediaFoundationDecoderHW::unlockDecoderContext(context);
                if (FAILED(res))
                {
                    //Bad things(tm) device lost or...
                    break;
                }
                if (res == S_OK)
                {
                    if (result == TRUE)
                        break;
                    OMAF_LOG_D("Waiting S_OK but result not TRUE");
                }
                loops++;
                Sleep(1);//sleep abit, wait for the copy to complete..                    
            }
            if (loops > 1) OMAF_LOG_D("Waiting for copy to finish, sleeped:%d times", loops); //only seems to wait on FIRST upload.. :)
        }
    }
#endif
}

void_t MediaFoundationDecoderHW::handleDecodedFrame(int64_t timestamp, int64_t duration, void_t* data, IMFSample* pSample)
{
    bool found = false;
    //find the matching decode request..

    if (mUnusedInputBuffer == OMAF_NULL)
    {
        OMAF_LOG_D("Got picture, but our decode queue is empty. Reset/Flush in progress? [%lld]", timestamp);
        return;
    }

    //Copy data
    IMFMediaBuffer* buf = (IMFMediaBuffer*)data;
    mUnusedInputBuffer->streamId = mDecoderConfig.streamId;
    mUnusedInputBuffer->decoder = this;
    mUnusedInputBuffer->consumed = false;
    mUnusedInputBuffer->pts = timestamp;
    mUnusedInputBuffer->width = mDecoderConfig.width;
    mUnusedInputBuffer->height = mDecoderConfig.height;
    mUnusedInputBuffer->duration = duration;
    //TODO: fill with correct info.
    mUnusedInputBuffer->colorInfo.matrixCoefficients = VideoMatrixCoefficients::ITU_R_BT_709;
    mUnusedInputBuffer->colorInfo.colorRange = VideoColorRange::LIMITED;//TV range 16-253
    mUnusedInputBuffer->colorInfo.colorPrimaries = VideoColorPrimaries::ITU_R_BT_709;
    mUnusedInputBuffer->colorInfo.transferCharacteristics = VideoTransferCharacteristics::ITU_R_BT_709;

    if (sMt) sMt->Enter();
#if PROFILE_COPYING
    DWORD t = timeGetTime();
#endif
#if OMAF_GRAPHICS_API_D3D11
    if (mGraphicsBackendType == RendererType::D3D11)
    {
        DecoderFrameDX11* frameDX11 = (DecoderFrameDX11*)mUnusedInputBuffer;
        //OMAF_LOG_I("Filling %p %p", frameDX11->texture, frameDX11->sharedTexture);
        //DumpType(mOutputType);
        ID3D11Asynchronous* async = OMAF_NULL;

        //define box, since dst might be smaller than src. (alignments/padding (1920x1088 -> 1920x1080 etc))
        D3D11_BOX box = { 0 };
        box.right = frameDX11->width;
        box.bottom = frameDX11->height;
        box.back = 1;
        bool b2dLock = false;
        bool bufLock = false;
        ID3D11Texture2D* pTextureArray = OMAF_NULL;
        IMFDXGIBuffer* pMFDXGIBuffer = OMAF_NULL;
        IMF2DBuffer* b2d = OMAF_NULL;
        HRESULT hr = E_NOINTERFACE;
        if (frameDX11->mTextureNV12)
        {
            //use the DXGI buffer only if we have NV12 textures..
            hr = buf->QueryInterface(__uuidof(IMFDXGIBuffer), (LPVOID *)(&pMFDXGIBuffer));
        }        
        if (SUCCEEDED(hr))
        {
            //Okay, gpu-gpu copy is possible. (this is the fastest path)
            UINT index;
            hr = pMFDXGIBuffer->GetSubresourceIndex(&index);//Get the index to texture array.
            if (SUCCEEDED(hr))
            {
                hr = pMFDXGIBuffer->GetResource(__uuidof(ID3D11Texture2D), (LPVOID *)(&pTextureArray));
                if (SUCCEEDED(hr))
                {
                    //copy the texture from array to our own texture.
                    sContext->CopySubresourceRegion(frameDX11->mTextureNV12, 0, 0, 0, 0, pTextureArray, index, &box);
                }
            }
        }
        else
        {
            //no DXGI buffer EEK! must be running software decoder.
            //Lock buffer, and update the texture. (cpu->gpu copy)
            DWORD ignore, len2;
            BYTE* src;
            UINT32 pWidth, pHeight;
            MFGetAttributeSize(mOutputType, MF_MT_FRAME_SIZE, &pWidth, &pHeight);

            if (SUCCEEDED(buf->QueryInterface(&b2d)))
            {
                //SW decoder does not seem to provide this.. but HW decoder does. so we should not come here. i think.
                LONG pitch;
                if (SUCCEEDED(b2d->Lock2D(&src, &pitch)))
                {
                    if (frameDX11->mStagingNV12)
                    {
                        copyFrame(frameDX11, box, src, pWidth, pHeight, pitch);
                    }
                    else if (frameDX11->mStagingY)
                    {
                        copyFrame2(frameDX11, box, src, pWidth, pHeight, pitch);
                    }
                    b2dLock = true;
                }
            }
            else
            {
                //on win10 this is slow ~13 ms.. on win7 this is ~2ms... (on laptop around 4-9ms)
                if (SUCCEEDED(buf->Lock(&src, &ignore, &len2)))//most likely copies internally to contiguous buffer, and locks that for us.
                {
                    if (frameDX11->mStagingNV12)
                    {
                        copyFrame(frameDX11, box, src, pWidth, pHeight, mStride);
                    }
                    else if (frameDX11->mStagingY)
                    {
                        copyFrame2(frameDX11, box, src, pWidth, pHeight, mStride);
                    }
                    bufLock = true;
                }
            }
        }
        if (b2dLock) b2d->Unlock2D();
        if (b2d) b2d->Release();
        if (bufLock) buf->Unlock();
        if (pTextureArray) pTextureArray->Release();
        if (pMFDXGIBuffer) pMFDXGIBuffer->Release();
        if (frameDX11->mQuery)
        {
            //make a syncpoint for the copy.. if requested.
            sContext->End(frameDX11->mQuery);
        }        
        //OMAF_LOG_I("Filled %p %p", frameDX11->texture, frameDX11->sharedTexture);
    }
#endif

#if OMAF_GRAPHICS_API_OPENGL
    if (mGraphicsBackendType == RendererType::OPENGL)
    {
        //TODO: Requires that frameOpenGL size (width*height) matches the sizeof decoder output frame.
        //which is NOT the same as viewable area size.. ie (1920x1080->1920x1088 and 220x124 -> 224x128)

        DecoderFrameOpenGL* frameOpenGL = (DecoderFrameOpenGL*)mUnusedInputBuffer;
        IMF2DBuffer* b2d;
        //IMF2DBuffer2* b2d;   //perhaps IMF2DBuffer2 with it's read only locks for better performace?
        UINT32 pWidth, pHeight;
        MFGetAttributeSize(mOutputType, MF_MT_FRAME_SIZE, &pWidth, &pHeight);
#if 0
        //Well, it seems that this is painfully slow so ... (against MS's documentation..)
        if (SUCCEEDED(buf->QueryInterface(&b2d)))
        {
            BYTE* src;
            LONG pitch;
            HRESULT res;
            res = b2d->Lock2D(&src, &pitch);
            /*BYTE* src2;
            DWORD len;
            res = b2d->Lock2DSize(MF2DBuffer_LockFlags_Read, &src2, &pitch, &src, &len);*/
            //TODO: if pitch<0 image is upside down...
            if (pitch == (((mDecoderConfig.width + 15) / 16) * 16))
            {
                //Fast path nice!
                memcpy((BYTE*)frameOpenGL->pboDataPtr, src, frameOpenGL->pboSize);
                res = b2d->Unlock2D();
            }
            else
            {
                //Need to do it a bit slower then...
                // OMAF_LOG_D("Stride of IMF2DBuffer(%d) not what we expected(%d).",pitch, (((mWidth + 15) / 16) * 16));
                res = b2d->ContiguousCopyTo((BYTE*)frameOpenGL->pboDataPtr, frameOpenGL->pboSize);
                res = b2d->Unlock2D();
            }
            res = b2d->Release();
        }
        else
#endif
        {
            DWORD ignore, len2;
            BYTE* src;
            if (SUCCEEDED(buf->Lock(&src, &ignore, &len2)))
            {
                if (frameOpenGL->pboDataPtr)
                {
                    //OMAF_ASSERT(len2 == frameOpenGL->pboSize, "OutputSample size does not match mPboSize in buf->Lock");
                    //memcpy(frameOpenGL->pboDataPtr, src, frameOpenGL->pboSize);
                    unsigned char* Y = (unsigned char*)frameOpenGL->pboDataPtr;
                    unsigned char* UV = Y + (frameOpenGL->width*frameOpenGL->height);
                    int RowPitch = frameOpenGL->width;
                    int h = frameOpenGL->height;
                    if (h > pHeight) h = pHeight;
                    int w = frameOpenGL->width;
                    if (w > pWidth) w = pWidth;
                    unsigned char* sr = src;
                    unsigned char* ds = (unsigned char*)Y;
                    for (int y = 0; y < h; y++)
                    {
                        memcpy(ds, sr, w);
                        sr += mStride;
                        ds += RowPitch;
                    }
                    //UV is quarter sized..
                    w /= 2;
                    h /= 2;
                    ds = (unsigned char*)UV;
                    sr = (unsigned char*)src + (mStride *pHeight);
                    for (int y = 0; y < h; y++)
                    {
                        unsigned char* src = (unsigned char*)sr;
                        unsigned int* dst = (unsigned int*)ds;
                        memcpy(dst, src, w * 2);
                        sr += mStride;
                        ds += RowPitch;
                    }
                }
                buf->Unlock();
            }
        }
    }
#endif
#if PROFILE_COPYING==1
    OMAF_LOG_V("Copy time:%d", (int32_t)timeGetTime() - t);
#endif
    if (sMt) sMt->Leave();
    mFrameCache.addDecodedFrame(mUnusedInputBuffer);
    mUnusedInputBuffer = OMAF_NULL;
    return;
}

Error::Enum MediaFoundationDecoderHW::decode(uint64_t pts, uint64_t duration, void_t* sourceData, size_t sourceLength)
{
    if (!mNeedMoreData && mQueuedFrameCount > 5)
    {
        // In HEVC the decoder seem to take in as many frames as the client has, so we need a limiter
        return Error::OPERATION_FAILED;
    }
    DWORD status;
    HRESULT res;
    Error::Enum result = Error::OPERATION_FAILED;
    res = mCodec->GetInputStatus(0, &status);
    if (status == MFT_INPUT_STATUS_ACCEPT_DATA) //Decoder can accept more data.
    {
        IMFSample* sample;
        res = MFCreateSample(&sample);
        if (res == S_OK)
        {
            sample->SetSampleTime(pts);//The presentation time, in 100-nanosecond units.
            sample->SetSampleDuration(duration);//The duration, in 100-nanosecond units.  (not actually used anywhere, and the duration is "approximate" anyway.
            //res=sample->SetUINT64(MFSampleExtension_DecodeTimestamp, dts);
            IMFMediaBuffer* buf;
            res = MFCreateMemoryBuffer((DWORD)sourceLength, &buf);
            if (res == S_OK)
            {
                res = sample->AddBuffer(buf);
                if (res == S_OK)
                {
                    BYTE* dst;
                    res = buf->Lock(&dst, OMAF_NULL, OMAF_NULL);
                    memcpy(dst, sourceData, (DWORD)sourceLength);
                    buf->SetCurrentLength((DWORD)sourceLength);//should not fail, since we created the buffer with max_len of len.
                    res = buf->Unlock();
                    res = mCodec->ProcessInput(0, sample, 0);
                    if (res == S_OK)
                    {
                        //Frame is being processed by codec now.
                        mQueuedFrameCount++;
                        result = Error::OK;
                    }
                    else
                    {
                        if (res == MF_E_NOTACCEPTING)
                        {
                            /*  The transform cannot process more input at this time.*/
                            //this is error is non fatal. it means, try again.
                            isFrameDecoded();
                            //OMAF_LOG_D("mDecode.size=%d mPictures.size=%d", mDecode.size(), mPictures.size());
                        }
                        else
                        {
                            // Something went wrong.
                            OMAF_LOG_D("%x\n", res);
                        }
                    }
                }
                buf->Release();
            }
            sample->Release();
        }
    }
    return result;
}

void_t MediaFoundationDecoderHW::deinitialize()
{
    OMAF_ASSERT(mCodec != OMAF_NULL, "Incorrect state");
    flush();
    releaseDecoderResources();
    mState = DecoderHWState::IDLE;
}

void_t MediaFoundationDecoderHW::releaseDecoderResources()
{
    if (mCodec != OMAF_NULL)
    {
        mCodec->Release();
        mCodec = OMAF_NULL;
    }

    if (mOutputSample != OMAF_NULL)
    {
        mOutputSample->Release();
        mOutputSample = OMAF_NULL;
    }
    if (mInputType != OMAF_NULL)
    {
        mInputType->Release();
        mInputType = OMAF_NULL;
    }
    if (mOutputType != OMAF_NULL)
    {
        mOutputType->Release();
        mOutputType = OMAF_NULL;
    }

}

void_t MediaFoundationDecoderHW::destroyInstance()
{
    OMAF_ASSERT(mState == DecoderHWState::IDLE, "Incorrect state");
    mState = DecoderHWState::INVALID;
}

void_t MediaFoundationDecoderHW::flush()
{
    HRESULT res;
    //Request full flush of codec.
    res = mCodec->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
    OMAF_LOG_D("MFT_MESSAGE_COMMAND_FLUSH:%d", res);
    //And start again!
    res = mCodec->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0);
    OMAF_LOG_D("MFT_MESSAGE_NOTIFY_START_OF_STREAM:%d", res);

    OMAF_ASSERT(mUnusedInputBuffer == OMAF_NULL, "For some reason there's still an unused input buffer in flush");

    mQueuedFrameCount = 0;
    mNeedMoreData = true;
    mInputEOS = false;

    if (mState == DecoderHWState::EOS)
    {
        mState = DecoderHWState::STARTED;
    }
}

void_t MediaFoundationDecoderHW::setInputEOS()
{
    if (mInputEOS == true)
    {
        return;
    }
    HRESULT res;
    //Request draining codec. (since stream has ended)
    res = mCodec->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN, 0);
    OMAF_LOG_D("MFT_MESSAGE_COMMAND_DRAIN:%d", res);
    if (SUCCEEDED(res))
    {
        mInputEOS = true;
    }
}

// Returns false if the parser should next time give the same frame again,
// i.e. if packet could not be queued to the codec.
DecodeResult::Enum MediaFoundationDecoderHW::decodeFrame(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking)
{
    OMAF_ASSERT(mInputEOS == false, "");

    // The codec must be configured first by the consumer. Before that is done, startCodec won't return true.
    if (mState != DecoderHWState::STARTED)
    {
        return DecodeResult::NOT_READY;
    }

    isFrameDecoded();

    if (mFirstPTS == -1)
    {
        mFirstPTS = packet->presentationTimeUs();
    }

    if (decode(packet->presentationTimeUs(), packet->durationUs(), packet->buffer(), packet->dataSize()) != Error::OK)
    {
        return DecodeResult::DECODER_FULL;
    }
    return DecodeResult::PACKET_ACCEPTED;
}

bool_t MediaFoundationDecoderHW::isFrameDecoded()
{
    if (mState != DecoderHWState::STARTED)
    {
        return false;
    }
    OMAF_ASSERT(mUnusedInputBuffer == OMAF_NULL, "mUnusedInputBuffer should be NULL here");
    mUnusedInputBuffer = mFrameCache.getFreeFrame(mDecoderConfig.streamId);

    if (mUnusedInputBuffer == OMAF_NULL)
    {
        return false;
    }
    mUnusedInputBuffer->decoder = OMAF_NULL;

    //and now check if codec has pictures for us... (and extract each and everyone of them)
    int got = 0;
    bool done = false;
    //while (!done) //TODO can we use the while-loop here? It may cause random jerkiness, seen at least in the beginning of the clip
    {
        HRESULT res;
        /*DWORD status;
        res = codec->GetOutputStatus(&status);
        if (res != S_OK) DebugBreak();
        if (status&MFT_OUTPUT_STATUS_SAMPLE_READY) //umm.. is always false.. for some reason so ignore this check.
        {
        */
        //we (might) have result!
        MFT_OUTPUT_DATA_BUFFER output = { 0 };
        DWORD stat = 0;
        output.pSample = mOutputSample;
        res = mCodec->ProcessOutput(0, 1, &output, &stat);
        if (output.pEvents)
        {
            output.pEvents->Release();
            output.pEvents = OMAF_NULL;
        }
        if (output.dwStatus != 0)
        {
            stat = stat;
        }
        if (stat != 0)
        {
            if (stat&MFT_PROCESS_OUTPUT_STATUS_NEW_STREAMS)
            {
                //The Media Foundation transform (MFT) has created one or more new output stream
                stat = stat;
            }
        }
        switch (res)
        {
            case MF_E_TRANSFORM_TYPE_NOT_SET:
            case MF_E_TRANSFORM_STREAM_CHANGE:
            {
                //re-configure the decoder. NOTE: i expect that the streams are consistent and wont actually change format in anyway.
                //HW decoders just seem to require setting the same output type again. (SW decoder does not do this request)
                //MFSetAttributeSize(outputtype, MF_MT_FRAME_SIZE, mWidth, mHeight);

                //Okay. lets just request the available output type, this fixes some vids..
                mOutputType->Release();
                mOutputType = OMAF_NULL;
                for (int ind = 0;; ind++)
                {
                    IMFMediaType* type;
                    res = mCodec->GetOutputAvailableType(0, ind, &type);
                    if (res != S_OK)
                    {
                        if (res == MF_E_NO_MORE_TYPES) break;//done.
                        if (res == MF_E_TRANSFORM_TYPE_NOT_SET) break;//need output type first.
                        if (res == E_NOTIMPL) break;//any type will do :D (yeah right)(no preferred output type)
                        if (res == MF_E_INVALIDSTREAMNUMBER) break;//no such stream id!
                        //other error..
                        break;
                    }
                    GUID sub;
                    type->GetGUID(MF_MT_SUBTYPE, &sub);
                    if (sub == MFVideoFormat_NV12)
                    {
                        mOutputType = type;
                        break;
                    }
                    type->Release();
                }
                if (mOutputType == OMAF_NULL)
                {
                    //Could not get valid output format.
                    //fail!
                    mState = DecoderHWState::ERROR_STATE;
                    return false;
                }

                UINT32 pWidth, pHeight;
                MFGetAttributeSize(mOutputType, MF_MT_FRAME_SIZE, &pWidth, &pHeight);
                pHeight = pHeight;
                res = MFGetStrideForBitmapInfoHeader(MFVideoFormat_NV12.Data1, pWidth, &mStride);
                if (FAILED(res))
                {
                    //okay.. let's guess then.
                    mStride = pWidth;
                }

                //TODO: verify that the output format is something we support NV12!
                res = mCodec->SetOutputType(0, mOutputType, 0);//yeah well.
                //https://msdn.microsoft.com/en-us/library/windows/desktop/ee663587(v=vs.85).aspx
                mNeedMoreData = true;
                break;
            }
            case S_OK:
            {
                //Copy the decoded picture.
                IMFMediaBuffer* buf;
                LONGLONG dur, pts;
                res = output.pSample->GetSampleDuration(&dur);
                res = output.pSample->GetSampleTime(&pts);
                // Workaround for a bug where WMF decoder always returns the first frame with the PTS 0
                if (pts == 0)
                {
                    pts = mFirstPTS;
                }
                res = output.pSample->GetBufferByIndex(0, &buf);
                handleDecodedFrame(pts, dur, buf, output.pSample);
                if (mOutputSample)
                {
                    //re-use the sample. set length to zero.
                    res = buf->SetCurrentLength(0);
                }
                else
                {
                    //we need to release the sample.
                    output.pSample->Release();
                }
                res = buf->Release();
                //OMAF_LOG_D("%8lld got frame at                %8lld %4d %4d %4d", Time::getClockTimeMs(), pts,mPboPool.size(),mDecode.size(),mPictures.size());
                got++;
                mQueuedFrameCount--;
                mNeedMoreData = false;
                //OMAF_LOG_D("Decoder full, queued %d", mQueuedFrameCount);
                break;
            }
            case MF_E_TRANSFORM_NEED_MORE_INPUT:
            {
                //Decoder does not have enough data to produce output.
                mNeedMoreData = true;
                //OMAF_LOG_D("Decoder needs more data, queued %d", mQueuedFrameCount);
                done = true;
                if (mInputEOS && mQueuedFrameCount == 0)
                {
                    //End of stream indicated.
                    //no pictures in decode queue
                    //no decoded pictures left.
                    mState = DecoderHWState::EOS;
                    OMAF_LOG_D("mInputEOS set, mDecode and mPictures is empty. All data drained.");
                }
                break;
            }
            //something went wrong.
            default:
            {
                if (res != S_OK) OMAF_LOG_E("Unhandled response from decoder %s:%d %d", __FUNCTION__, __LINE__, res);
                done = true;
                break;
            }
        }
    }

    if (mUnusedInputBuffer != OMAF_NULL)
    {
        mFrameCache.releaseFrame(mUnusedInputBuffer);
        mUnusedInputBuffer = OMAF_NULL;
    }

    if (got == 0)
    {
        //no new frames
        return false;
    }
    /*        if (got >= 1)
    {
    OMAF_LOG_D("Got %d pictures", got);
    got = got;
    }*/
#if ENABLE_FPS_LOGGING
    // Log decoding framerate
    static int frameCount = 0;
    static int64_t lastFPSTime = Time::getClockTimeUs();

    frameCount += got;
    int64_t now = Time::getClockTimeUs();
    if (now - lastFPSTime > 2000000)
    {
        OMAF_LOG_D("Video decode FPS: %.3f", 1000000.0 * frameCount / (now - lastFPSTime));
        frameCount = 0;
        lastFPSTime = now;
    }
#endif
    return true;
}
void SetupDecoder()
{
    MediaFoundationDecoderHW::InitContexts();
}
void CleanupDecoder()
{
    MediaFoundationDecoderHW::ReleaseContexts();
}
OMAF_NS_END
