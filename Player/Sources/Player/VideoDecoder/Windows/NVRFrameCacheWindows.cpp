
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
#include <NVRGraphics.h>

#include "VideoDecoder/Windows/NVRFrameCacheWindows.h"
#include "VideoDecoder/NVRVideoDecoderFrame.h"
#include "Foundation/NVRLogger.h"

#if OMAF_GRAPHICS_API_OPENGL
#include "Graphics/OpenGL/Windows/NVRGLFunctions.h"
#endif

#if OMAF_GRAPHICS_API_D3D11
#include "VideoDecoder/Windows/NVRMediaFoundationDecoderHW.h"
#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(FrameCacheWindows);

FrameCacheWindows::FrameCacheWindows()
{

#if OMAF_GRAPHICS_API_OPENGL
    while (mTextureHandles.getSize() != MAX_STREAM_COUNT)
    {
        mTextureHandles.add(TextureHandles());
    }
#endif
#if OMAF_GRAPHICS_API_D3D11
    //NOTE: We are using one ID3D11Device and IMFDXGIDeviceManager for ALL decoder instances.
    mDecoderDevice=MediaFoundationDecoderHW::getDecoderDevice();
#endif
}

FrameCacheWindows::~FrameCacheWindows()
{
    destroyInstance();
#if OMAF_GRAPHICS_API_D3D11
    MediaFoundationDecoderHW::releaseDecoderDevice();
#endif
}

void dump(UINT nv12_format_support)
{
#define TSEK(a) if (a&nv12_format_support) OutputDebugStringA(#a"\n");
    TSEK(D3D11_FORMAT_SUPPORT_BUFFER)
        TSEK(D3D11_FORMAT_SUPPORT_IA_VERTEX_BUFFER)
        TSEK(D3D11_FORMAT_SUPPORT_IA_INDEX_BUFFER)
        TSEK(D3D11_FORMAT_SUPPORT_SO_BUFFER)
        TSEK(D3D11_FORMAT_SUPPORT_TEXTURE1D)
        TSEK(D3D11_FORMAT_SUPPORT_TEXTURE2D)
        TSEK(D3D11_FORMAT_SUPPORT_TEXTURE3D)
        TSEK(D3D11_FORMAT_SUPPORT_TEXTURECUBE)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_LOAD)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_SAMPLE)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_COMPARISON)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_SAMPLE_MONO_TEXT)
        TSEK(D3D11_FORMAT_SUPPORT_MIP)
        TSEK(D3D11_FORMAT_SUPPORT_MIP_AUTOGEN)
        TSEK(D3D11_FORMAT_SUPPORT_RENDER_TARGET)
        TSEK(D3D11_FORMAT_SUPPORT_BLENDABLE)
        TSEK(D3D11_FORMAT_SUPPORT_DEPTH_STENCIL)
        TSEK(D3D11_FORMAT_SUPPORT_CPU_LOCKABLE)
        TSEK(D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE)
        TSEK(D3D11_FORMAT_SUPPORT_DISPLAY)
        TSEK(D3D11_FORMAT_SUPPORT_CAST_WITHIN_BIT_LAYOUT)
        TSEK(D3D11_FORMAT_SUPPORT_MULTISAMPLE_RENDERTARGET)
        TSEK(D3D11_FORMAT_SUPPORT_MULTISAMPLE_LOAD)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_GATHER)
        TSEK(D3D11_FORMAT_SUPPORT_BACK_BUFFER_CAST)
        TSEK(D3D11_FORMAT_SUPPORT_TYPED_UNORDERED_ACCESS_VIEW)
        TSEK(D3D11_FORMAT_SUPPORT_SHADER_GATHER_COMPARISON)
        TSEK(D3D11_FORMAT_SUPPORT_DECODER_OUTPUT)
        TSEK(D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_OUTPUT)
        TSEK(D3D11_FORMAT_SUPPORT_VIDEO_PROCESSOR_INPUT)
        TSEK(D3D11_FORMAT_SUPPORT_VIDEO_ENCODER)
}
void OpenShared(ID3D11Texture2D*& tex, ID3D11Resource*& shtex )
{
    HRESULT hr;
    IDXGIResource* pOtherResource(NULL);
    hr = tex->QueryInterface(__uuidof(IDXGIResource), (void**)&pOtherResource);
    OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture IDXGIResource interface query failed");
    HANDLE sharedHandle;
    hr = pOtherResource->GetSharedHandle(&sharedHandle); //this should not be closed or released ever.. 
    OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture GetSharedHandle failed");
    const RenderBackend::Parameters& parms = RenderBackend::getParameters();
    hr = ((ID3D11Device*)parms.d3dDevice)->OpenSharedResource(sharedHandle, __uuidof(ID3D11Resource), (void**)&shtex);
    OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture OpenSharedHandle failed");
    pOtherResource->Release();

}
DecoderFrame* FrameCacheWindows::createFrame(uint32_t width, uint32_t height)
{
    DecoderFrame* result = OMAF_NULL;

    switch (RenderBackend::getRendererType())
    {
#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            DecoderFrameOpenGL* frame = OMAF_NEW_HEAP(DecoderFrameOpenGL);
            uint32_t w = width;
            uint32_t h = height;
            frame->pboSize = w * h * 3 / 2;
            frame->pboDataPtr = 0;
            frame->pboId = 0;
            frame->width = w;
            frame->height = h;

            glGenBuffers(1, &frame->pboId);
            GLint currentPBO;
            glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentPBO);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, frame->pboId);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, frame->pboSize, 0, GL_STREAM_DRAW);//allocates the buffer.
            frame->pboDataPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentPBO);
            result = frame;
            break;
        }
#endif

#if OMAF_GRAPHICS_API_D3D11
        case RendererType::D3D11:
        {
            DecoderFrameDX11* frame = OMAF_NEW_HEAP(DecoderFrameDX11);
            ZeroMemory(frame, sizeof(*frame));
            HRESULT hr = S_OK;
            UINT nv12_format_support = 0;

            D3D11_QUERY_DESC qdesc = { D3D11_QUERY_EVENT };
            hr = mDecoderDevice->CreateQuery(&qdesc, &frame->mQuery);
            OMAF_ASSERT(SUCCEEDED(hr), "Decoder query creation failed");
            frame->width = width;
            frame->height = height;

            //fun fact.. NV12 texture is supported on win 10 with 9.3 context.. and is faster than on 11.x context...
            //           win7 supports NV12 but not as a shader resource..
            hr = mDecoderDevice->CheckFormatSupport(DXGI_FORMAT_NV12, &nv12_format_support);
            //nv12_format_support = 0;//dump(nv12_format_support);
            UINT flags = (D3D11_FORMAT_SUPPORT_SHADER_SAMPLE | D3D11_FORMAT_SUPPORT_TEXTURE2D);
            if ((nv12_format_support&flags)==flags)
            {
                //Win8.1+ path
                D3D11_TEXTURE2D_DESC desc;
                ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
                desc.Width = frame->width;
                desc.Height = frame->height;
                desc.MipLevels = 1;
                desc.Format = DXGI_FORMAT_NV12;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.ArraySize = 1;
                desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

                hr = mDecoderDevice->CreateTexture2D(&desc, NULL, &frame->mTextureNV12); //this texture SHOULD be created by the decoder instance, possibly also in the decoder thread...
                OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture creation failed");

                OpenShared(frame->mTextureNV12, frame->mSharedTextureNV12);

                NativeTextureDesc textureDesc;
                textureDesc.type = TextureType::TEXTURE_VIDEO_SURFACE;
                textureDesc.width = (uint16_t)frame->width;
                textureDesc.height = (uint16_t)frame->height;
                textureDesc.format = TextureFormat::R8;
                textureDesc.nativeHandle = frame->mSharedTextureNV12;
                frame->mNativeY = RenderBackend::createNativeTexture(textureDesc);
                textureDesc.width = (uint16_t)frame->width / 2;
                textureDesc.height = (uint16_t)frame->height / 2;
                textureDesc.format = TextureFormat::RG8;
                textureDesc.nativeHandle = frame->mSharedTextureNV12;
                frame->mNativeUV = RenderBackend::createNativeTexture(textureDesc);

                //Create staging texture for software fallback. (if we get hw, we just waste memory now)
                //TODO: apparently it might be good to use with HW too? test later.
                D3D11_TEXTURE2D_DESC sdesc;
                ZeroMemory(&sdesc, sizeof(D3D11_TEXTURE2D_DESC));
                sdesc.Width = frame->width;
                sdesc.Height = frame->height;
                sdesc.MipLevels = 1;
                sdesc.Format = DXGI_FORMAT_NV12;
                sdesc.SampleDesc.Count = 1;
                sdesc.Usage = D3D11_USAGE_STAGING;
                sdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                sdesc.ArraySize = 1;
                hr = mDecoderDevice->CreateTexture2D(&sdesc, NULL, &frame->mStagingNV12);
                OMAF_ASSERT(SUCCEEDED(hr), "Decoder staging texture creation failed");
            }
            else
            {
                //Legacy path...
                D3D11_TEXTURE2D_DESC desc;
                ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
                desc.Width = frame->width;
                desc.Height = frame->height;
                desc.MipLevels = 1;
                desc.Format = DXGI_FORMAT_R8_UNORM;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.Usage = D3D11_USAGE_DEFAULT;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.ArraySize = 1;
                desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;                
                DXGI_FORMAT UVFormat;
                TextureFormat::Enum UVFormat2;
                UINT r8g8_format_support = 0;
                hr = mDecoderDevice->CheckFormatSupport(DXGI_FORMAT_R8G8_UNORM, &r8g8_format_support);
                if ((r8g8_format_support&flags) == flags)
                {
                    UVFormat = DXGI_FORMAT_R8G8_UNORM;          //This requires DX11... cant be created as a shared texture on feature level 9.3 (on win7.. works fine on win 10!!) (also works on the thinkpad.... but not on the desktop machine...)
                    UVFormat2 = TextureFormat::RG8;
                }
                else
                {
                    UVFormat = DXGI_FORMAT_B8G8R8A8_UNORM;          //Works.. but i would prefer not to.
                    UVFormat2 = TextureFormat::BGRA8;
                }
                hr = mDecoderDevice->CreateTexture2D(&desc, NULL, &frame->mTextureY); //this texture SHOULD be created by the decoder instance, possibly also in the decoder thread...
                OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture creation failed");
                desc.Width = frame->width / 2;
                desc.Height = frame->height / 2;
                desc.Format = UVFormat;
                hr = mDecoderDevice->CreateTexture2D(&desc, NULL, &frame->mTextureUV); //this texture SHOULD be created by the decoder instance, possibly also in the decoder thread...
                OMAF_ASSERT(SUCCEEDED(hr), "Decoder texture creation failed");

                OpenShared(frame->mTextureY, frame->mSharedTextureY);
                OpenShared(frame->mTextureUV, frame->mSharedTextureUV);

                NativeTextureDesc textureDesc;
                textureDesc.type = TextureType::TEXTURE_VIDEO_SURFACE;
                textureDesc.width = (uint16_t)frame->width;
                textureDesc.height = (uint16_t)frame->height;
                frame->mYformat = textureDesc.format = TextureFormat::R8;
                textureDesc.nativeHandle = frame->mSharedTextureY;
                frame->mNativeY = RenderBackend::createNativeTexture(textureDesc);
                textureDesc.width = (uint16_t)frame->width / 2;
                textureDesc.height = (uint16_t)frame->height / 2;
                frame->mUVformat = textureDesc.format = UVFormat2;
                textureDesc.nativeHandle = frame->mSharedTextureUV;
                frame->mNativeUV = RenderBackend::createNativeTexture(textureDesc);

                //Staging textures
                D3D11_TEXTURE2D_DESC sdesc;
                ZeroMemory(&sdesc, sizeof(D3D11_TEXTURE2D_DESC));
                sdesc.Width = frame->width;
                sdesc.Height = frame->height;
                sdesc.MipLevels = 1;
                sdesc.Format = DXGI_FORMAT_R8_UNORM;
                sdesc.SampleDesc.Count = 1;
                sdesc.Usage = D3D11_USAGE_STAGING;
                sdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                sdesc.ArraySize = 1;
                hr = mDecoderDevice->CreateTexture2D(&sdesc, NULL, &frame->mStagingY);
                sdesc.Width = frame->width / 2;
                sdesc.Height = frame->height / 2;
                sdesc.Format = UVFormat;
                hr = mDecoderDevice->CreateTexture2D(&sdesc, NULL, &frame->mStagingUV);
                OMAF_ASSERT(SUCCEEDED(hr), "Decoder staging texture creation failed");
            }
            result = frame;
            break;
        }
#endif
        default:
        {
            OMAF_ASSERT_UNREACHABLE();
            break;
        }
    }

    OMAF_ASSERT(result != OMAF_NULL, "result==OMAF_NULL in FrameCacheWindows::createFrame");

    return result;
}

void_t FrameCacheWindows::destroyFrame(DecoderFrame *frame)
{
    OMAF_ASSERT(frame != OMAF_NULL, "frame == OMAF_NULL in FrameCacheWindows::destroyFrame");
    switch (RenderBackend::getRendererType())
    {
#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            DecoderFrameOpenGL* openGLFrame = (DecoderFrameOpenGL*)frame;
            GLint currentPBO;
            glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentPBO);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, openGLFrame->pboId);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glDeleteBuffers(1, &(openGLFrame->pboId));
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentPBO);
            break;
        }
#endif

#if OMAF_GRAPHICS_API_D3D11
        case RendererType::D3D11:
        {
            DecoderFrameDX11* d3d11Frame = (DecoderFrameDX11*)frame;
            RenderBackend::destroyTexture(d3d11Frame->mNativeY);
            RenderBackend::destroyTexture(d3d11Frame->mNativeUV);
            if (d3d11Frame->mTextureNV12)
            {
                d3d11Frame->mTextureNV12->Release();
                d3d11Frame->mTextureNV12 = OMAF_NULL;
            }
            if (d3d11Frame->mTextureY)
            {
                d3d11Frame->mTextureY->Release();
                d3d11Frame->mTextureY = OMAF_NULL;
            }
            if (d3d11Frame->mTextureUV)
            {
                d3d11Frame->mTextureUV->Release();
                d3d11Frame->mTextureUV = OMAF_NULL;
            }
            if (d3d11Frame->mSharedTextureNV12)
            {
                d3d11Frame->mSharedTextureNV12->Release();
                d3d11Frame->mSharedTextureNV12 = OMAF_NULL;
            }
            if (d3d11Frame->mSharedTextureY)
            {
                d3d11Frame->mSharedTextureY->Release();
                d3d11Frame->mSharedTextureY= OMAF_NULL;
            }
            if (d3d11Frame->mSharedTextureUV)
            {
                d3d11Frame->mSharedTextureUV->Release();
                d3d11Frame->mSharedTextureUV= OMAF_NULL;
            }
            if (d3d11Frame->mStagingNV12)
            {
                d3d11Frame->mStagingNV12->Release();
                d3d11Frame->mStagingNV12 = OMAF_NULL;
            }
            if (d3d11Frame->mStagingY)
            {
                d3d11Frame->mStagingY->Release();
                d3d11Frame->mStagingY= OMAF_NULL;
            }
            if (d3d11Frame->mStagingUV)
            {
                d3d11Frame->mStagingUV->Release();
                d3d11Frame->mStagingUV= OMAF_NULL;
            }
            if (d3d11Frame->mQuery)
            {
                d3d11Frame->mQuery->Release();
                d3d11Frame->mQuery = OMAF_NULL;
            }
            break;
        }
#endif
        default:
        {
            OMAF_ASSERT_UNREACHABLE();
            break;
        }
    }

    OMAF_DELETE_HEAP(frame);
}

void_t FrameCacheWindows::uploadTexture(DecoderFrame* frame)
{
    OMAF_ASSERT(frame != OMAF_NULL, "Uploading null frame");
    VideoFrame& videoFrame = mCurrentVideoFrames.at(frame->streamId);
    OMAF_ASSERT(videoFrame.format != VideoPixelFormat::INVALID, "Uninitialized video frame");
    videoFrame.pts = frame->pts;
    
    //  OMAF_LOG_D("Current PBO PTS: %lld", mCurPBO.pts);
    switch (RenderBackend::getRendererType())
    {
#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            DecoderFrameOpenGL* openGLFrame = (DecoderFrameOpenGL*)frame;
            TextureHandles& handles = mTextureHandles.at(frame->streamId);
            
            //Save currently bound state.
            GLint currentTexture = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

            GLint currentPBO = 0;
            glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &currentPBO);

            GLint currentRowLength = 0;
            glGetIntegerv(GL_UNPACK_ROW_LENGTH, &currentRowLength);
            
            
            // Activate and unmap the PBO so we can use it.
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, openGLFrame->pboId);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            
            int w = frame->width;
            int h = frame->height;

            // Update Y texture with the PBO
            glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
            glBindTexture(GL_TEXTURE_2D, handles.Y);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width, frame->height, GL_RED, GL_UNSIGNED_BYTE, (const void_t*)0);

            // Update UV texture with the PBO
            glPixelStorei(GL_UNPACK_ROW_LENGTH, w / 2);
            glBindTexture(GL_TEXTURE_2D, handles.UV);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame->width / 2, frame->height / 2, GL_RG, GL_UNSIGNED_BYTE, (const void_t*)(intptr_t)(w * h));

            // Discard/realloc the PBO
            glBufferData(GL_PIXEL_UNPACK_BUFFER, openGLFrame->pboSize, (const void_t*)0, GL_STREAM_DRAW);
            
            // Map the PBO so we can fill it from the other thread
            openGLFrame->pboDataPtr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            
            // Restore old state
            glPixelStorei(GL_UNPACK_ROW_LENGTH, currentRowLength);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, currentPBO);
            glBindTexture(GL_TEXTURE_2D, currentTexture);
            break;
        }
#endif
    
#if OMAF_GRAPHICS_API_D3D11
        case RendererType::D3D11:
        {
            DecoderFrameDX11* dx11Frame = (DecoderFrameDX11*)frame;
            videoFrame.textures[0] = dx11Frame->mNativeY;
            videoFrame.textures[1] = dx11Frame->mNativeUV;
            //OMAF_LOG_I("Rendering %p %p", dx11Frame->texture, dx11Frame->sharedTexture);            
            break;
        }
#endif
        default:
        {
            OMAF_ASSERT_UNREACHABLE();
            break;
        }
    }
    frame->decoder->consumedFrame(frame);
#if ENABLE_FPS_LOGGING
    // Log texture update framerate
    static int frameCount = 0;
    static int64_t lastTime = Time::getClockTimeUs();
    ++frameCount;
    int64_t now = Time::getClockTimeUs();

    if (now - lastTime > 2000000)
    {
        OMAF_LOG_D("Video update FPS: %.3f", 1000000.0 * frameCount / (now - lastTime));
        frameCount = 0;
        lastTime = now;
    }
#endif
}

void_t FrameCacheWindows::createTexture(streamid_t stream, const DecoderConfig& config)
{
    VideoFrame& frame = mCurrentVideoFrames.at(stream);
    
    if (frame.format != VideoPixelFormat::INVALID)
    {
        return;
    }

    OMAF_LOG_D("Creating yuv textures");

    uint32_t width = config.width;
    uint32_t height = config.height;

    switch (RenderBackend::getRendererType())
    {
#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            TextureHandles& handles = mTextureHandles.at(stream);

            GLint currentTexture = 0;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

            glGenTextures(2, &handles.Y);
            glBindTexture(GL_TEXTURE_2D, handles.Y);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

            glBindTexture(GL_TEXTURE_2D, handles.UV);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, width / 2, height / 2, 0, GL_RG, GL_UNSIGNED_BYTE, 0);

            glBindTexture(GL_TEXTURE_2D, currentTexture);

            NativeTextureDesc textureDesc;
            textureDesc.type = TextureType::TEXTURE_VIDEO_SURFACE;
            textureDesc.width = (uint16_t)width;
            textureDesc.height = (uint16_t)height;
            textureDesc.numMipMaps = 1;
            textureDesc.format = TextureFormat::R8;
            textureDesc.nativeHandle = &handles.Y;
            frame.textures[0] = RenderBackend::createNativeTexture(textureDesc);

            textureDesc.width = (uint16_t)width / 2;
            textureDesc.height = (uint16_t)height / 2;
            textureDesc.format = TextureFormat::RG8;
            textureDesc.nativeHandle = &handles.UV;
            frame.textures[1] = RenderBackend::createNativeTexture(textureDesc);

            frame.format = VideoPixelFormat::NV12;
            frame.numTextures = 2;

            frame.width = (uint16_t)width;
            frame.height = (uint16_t)height;

            frame.matrix = FLIPPED_TEXTURE_MATRIX;
            break;
        }
#endif
    
#if OMAF_GRAPHICS_API_D3D11
        case RendererType::D3D11:
        {
            frame.format = VideoPixelFormat::NV12;
            frame.numTextures = 2;
            frame.textures[0] = TextureID::Invalid;
            frame.textures[1] = TextureID::Invalid;

            frame.width = (uint16_t)width;
            frame.height = (uint16_t)height;

            frame.matrix = FLIPPED_TEXTURE_MATRIX;
            break;
        }
#endif

        default:
        {
            OMAF_ASSERT_UNREACHABLE();

            break;
        }
    }
}

void_t FrameCacheWindows::destroyTexture(streamid_t stream)
{
    VideoFrame& frame = mCurrentVideoFrames.at(stream);

    switch (RenderBackend::getRendererType())
    {
#if OMAF_GRAPHICS_API_OPENGL
        case RendererType::OPENGL:
        {
            TextureHandles& handles = mTextureHandles.at(stream);

            if (handles.Y != 0)
            {
                glDeleteTextures(1, &handles.Y);
            }

            if (handles.UV != 0)
            {
                glDeleteTextures(1, &handles.UV);
            }

            handles.Y = handles.UV = 0;

            releaseVideoFrame(frame);
            break;
        }
#endif

#if OMAF_GRAPHICS_API_D3D11
        case RendererType::D3D11:
        {
            //Textures are actually destroyed in destroyFrame.
            //So just mark as invalid.
            frame.numTextures = 0;
            frame.textures[0] = TextureID::Invalid;
            frame.textures[1] = TextureID::Invalid;
            break;
        }
#endif

        default:
        {
            OMAF_ASSERT_UNREACHABLE();
            break;
        }
    }
}

OMAF_NS_END
