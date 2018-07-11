
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

#include "Platform/OMAFPlatformDetection.h"
#include "VideoDecoder/NVRVideoDecoderHW.h"
#include "Graphics/NVRRendererType.h"
#include "Graphics/NVRDependencies.h"
#include "Graphics/NVRTextureFormat.h"

//forward declarations of DX interfaces.
struct IMFTransform;
struct IMFSample;
struct IMFMediaType;
struct IMFMediaType;
struct IMFDXGIDeviceManager;
struct ID3D11Device;
struct ID3D11Texture2D;

typedef HRESULT(WINAPI* CreateDXGIDeviceManagerPROC)(UINT* reset_token, IMFDXGIDeviceManager** device_manager);

OMAF_NS_BEGIN

#if OMAF_GRAPHICS_API_D3D11
struct DecoderFrameDX11
: public DecoderFrame
{
    TextureFormat::Enum mYformat;   //always TextureFormat::R8
    TextureID mNativeY;
    TextureFormat::Enum mUVformat;  //TextureFormat::RG8 OR TextureFormat::BGRA8 depending on support...
    TextureID mNativeUV;
    ID3D11Query* mQuery;
    ID3D11Texture2D *mTextureNV12;      //Used by decoder.(copy of a frame from the internal array texture) (used if NV12 supported)
    ID3D11Texture2D *mTextureY;      //Used by decoder.(copy of a frame from the internal array texture)    (used if NV12 not supported)
    ID3D11Texture2D *mTextureUV;      //Used by decoder.(copy of a frame from the internal array texture)   (used if NV12 not supported)
    ID3D11Resource *mSharedTextureNV12; //Used by renderer.(shared handle to decoder texture)   (used if NV12 supported)
    ID3D11Resource *mSharedTextureY; //Used by renderer.(shared handle to decoder texture)      (used if NV12 not supported)
    ID3D11Resource *mSharedTextureUV; //Used by renderer.(shared handle to decoder texture)     (used if NV12 not supported)
    ID3D11Texture2D* mStagingNV12;//cpu write   (used if NV12 supported)
    ID3D11Texture2D* mStagingY;//cpu write      (used if NV12 not supported)
    ID3D11Texture2D* mStagingUV;//cpu write     (used if NV12 not supported)
};
#endif

struct DecoderFrameOpenGL
: public DecoderFrame
{
    unsigned int pboId;
    uint32_t pboSize;
    void *pboDataPtr;
};

    class MediaFoundationDecoderHW : public VideoDecoderHW
    {
    public:

        MediaFoundationDecoderHW(FrameCache& frameCache);

        virtual ~MediaFoundationDecoderHW();

        virtual Error::Enum createInstance(const MimeType& mimeType);

        const DecoderConfig& getConfig() const;

        void_t setStreamId(streamid_t streamId);

        virtual Error::Enum initialize(const DecoderConfig& config);

        virtual void_t flush();

        virtual void_t deinitialize();

        virtual void_t destroyInstance();

        virtual void setInputEOS();

        virtual bool_t isEOS();

        virtual DecodeResult::Enum decodeFrame(streamid_t stream, MP4VRMediaPacket* packet, bool_t seeking);
        virtual void_t consumedFrame(DecoderFrame* frame);

        //TODO: Internal implementation, share the decoder device with framecache. needs a better way.
        static ID3D11Device* getDecoderDevice();
        static void releaseDecoderDevice();
        static void InitContexts();
        static void ReleaseContexts();



    private:

        OMAF_NO_COPY(MediaFoundationDecoderHW);
        OMAF_NO_ASSIGN(MediaFoundationDecoderHW);

    private:

        bool_t isFrameDecoded();

        Error::Enum decode(uint64_t pts, uint64_t duration, void_t* sourceData, size_t sourceLength);
        void_t handleDecodedFrame(int64_t timestamp, int64_t duration, void_t* data, IMFSample*);

        void_t releaseDecoderResources();

        static ID3D11DeviceContext* lockDecoderContext();
        static void unlockDecoderContext(ID3D11DeviceContext*);

    private:

        DecoderHWState::Enum mState;
        MimeType mMimeType;
        DecoderConfig mDecoderConfig;

        bool mInputEOS;

        IMFTransform* mCodec;
        IMFSample* mOutputSample;
        IMFMediaType* mInputType;
        IMFMediaType* mOutputType;

        static Mutex sContextMutex;
        static HMODULE sMFModule;
        static CreateDXGIDeviceManagerPROC sCreateDXGIDeviceManager;
        static IMFDXGIDeviceManager* sDevMan;
        static ID3D11Device* sDev;
        static ID3D11DeviceContext* sContext;
        static ID3D11Debug* sDebug;
        static ID3D10Multithread* sMt;
        static uint32_t sResetToken;
        static uint32_t sInstanceCount;


#if OMAF_GRAPHICS_API_D3D11
        void copyFrame2(DecoderFrameDX11* dx11Frame, D3D11_BOX &box, BYTE* src, int pWidth, int pHeight,int pPitch);
        void copyFrame(DecoderFrameDX11* dx11Frame, D3D11_BOX &box, BYTE* src, int pWidth, int pHeight, int pPitch);
#endif

        FrameCache& mFrameCache;
        uint32_t mQueuedFrameCount;

        DecoderFrame* mUnusedInputBuffer;

        RendererType::Enum mGraphicsBackendType;
        int64_t mFirstPTS;
        LONG mStride;

        bool mNeedMoreData;
    };
OMAF_NS_END
