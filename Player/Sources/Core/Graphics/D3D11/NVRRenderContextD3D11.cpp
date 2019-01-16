
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
#include "Graphics/D3D11/NVRRenderContextD3D11.h"

#include "Graphics/D3D11/NVRD3D11Utilities.h"
#include "Graphics/D3D11/NVRD3D11Error.h"
#include "Math/OMAFMathFunctions.h"

#include "Foundation/NVRLogger.h"

#include <cvt/wstring>
#include <codecvt>

OMAF_NS_BEGIN

    // https://blogs.msdn.microsoft.com/chuckw/2012/11/30/direct3d-sdk-debug-layer-tricks/
    // https://blogs.msdn.microsoft.com/vcblog/2015/03/31/visual-studio-2015-and-graphics-tools-for-windows-10/

    OMAF_LOG_ZONE(RenderContextD3D11);

    static const char_t* getFeatureLevelStr(D3D_FEATURE_LEVEL level)
    {
        switch (level)
        {
            case D3D_FEATURE_LEVEL_9_1:     return "Direct3D11 device feature level 9.1";
            case D3D_FEATURE_LEVEL_9_2:     return "Direct3D11 device feature level 9.2";
            case D3D_FEATURE_LEVEL_9_3:     return "Direct3D11 device feature level 9.3";

            case D3D_FEATURE_LEVEL_10_0:    return "Direct3D11 device feature level 10.0";
            case D3D_FEATURE_LEVEL_10_1:    return "Direct3D11 device feature level 10.1";

            case D3D_FEATURE_LEVEL_11_0:    return "Direct3D11 device feature level 11.0";
            case D3D_FEATURE_LEVEL_11_1:    return "Direct3D11 device feature level 11.1";

            /*NOTE: these enums are not defined in older SDK's*/
            case /*D3D_FEATURE_LEVEL_12_0*/0xc000:    return "Direct3D11 device feature level 12.0";
            case /*D3D_FEATURE_LEVEL_12_1*/0xc100:    return "Direct3D11 device feature level 12.1";

            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        return OMAF_NULL;
    }

    static const char_t* getShaderModelStr(D3D_FEATURE_LEVEL level)
    {
        // https://msdn.microsoft.com/en-us/library/windows/desktop/ff476876(v=vs.85).aspx

        switch (level)
        {
            case D3D_FEATURE_LEVEL_9_1:     return "Shader Model 2.0";
            case D3D_FEATURE_LEVEL_9_2:     return "Shader Model 2.0";
            case D3D_FEATURE_LEVEL_9_3:     return "Shader Model 2.0";

            case D3D_FEATURE_LEVEL_10_0:    return "Shader Model 4.0";
            case D3D_FEATURE_LEVEL_10_1:    return "Shader Model 4.1";

            case D3D_FEATURE_LEVEL_11_0:    return "Shader Model 5.0";
            case D3D_FEATURE_LEVEL_11_1:    return "Shader Model 5.0";

            /*NOTE: these enums are not defined in older SDK's*/
            case /*D3D_FEATURE_LEVEL_12_0*/0xc000:    return "Shader Model 5.1";
            case /*D3D_FEATURE_LEVEL_12_1*/0xc100:    return "Shader Model 5.1";

        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
        }

        return OMAF_NULL;
    }

    RenderContextD3D11::RenderContextD3D11(RendererType::Enum type, RenderBackend::Parameters& parameters)
    : IRenderContext(type, parameters)
    , mDevice(NULL)
    , mDeviceContext(NULL)
#if OMAF_DEBUG_BUILD
    , mAnnotation(NULL)
#endif
    , mCurrentColor(NULL)
    , mCurrentDepthStencil(NULL)
    , mRasterizerState(NULL)
    , mBlendState(NULL)
    , mDepthStencilState(NULL)
    , mMSAAEnabled(false) // TODO: Placeholder for proper MSAA support
#if WAIT_FOR_FRAME
    , mQuery(NULL)
#endif
    {
        ZeroMemory(&mRasterizerDesc, sizeof(mRasterizerDesc));
        ZeroMemory(&mBlendDesc, sizeof(mBlendDesc));
        ZeroMemory(&mDepthStencilDesc, sizeof(mDepthStencilDesc));

        mDevice = (ID3D11Device*)parameters.d3dDevice;
    }

    RenderContextD3D11::~RenderContextD3D11()
    {
    }

    bool_t RenderContextD3D11::create()
    {
        mVertexBuffers.resize(OMAF_MAX_VERTEX_BUFFERS);
        mIndexBuffers.resize(OMAF_MAX_INDEX_BUFFERS);
        mShaders.resize(OMAF_MAX_SHADERS);
        mShaderConstants.resize(OMAF_MAX_SHADER_CONSTANTS);
        mTextures.resize(OMAF_MAX_TEXTURES);
        mRenderTargets.resize(OMAF_MAX_RENDER_TARGETS);
        mActiveComputeResources.resize(OMAF_MAX_COMPUTE_BINDINGS);

        // Get immediate context
        if (mDevice == NULL)
        {
            return false;
        }

        mDevice->GetImmediateContext(&mDeviceContext);

#if OMAF_DEBUG_BUILD
        // Query debug annotation interface
        if (!SUCCEEDED(mDeviceContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&mAnnotation)))
        {
            return false;
        }
#endif

        // Query generic capabilities & memory statistics
        bool_t instancedSupport = false;
        bool_t computeSupport = false;

        D3D_FEATURE_LEVEL featureLevel = mDevice->GetFeatureLevel();

        // Instancing is fully supported on 9.3+, but partially / optionally supported at lower levels
        if (featureLevel >= D3D_FEATURE_LEVEL_9_3)
        {
            instancedSupport = true;
        }
        else
        {
            struct D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT
            {
                BOOL SimpleInstancingSupported;
            };

            D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT featureData;
            HRESULT hr = mDevice->CheckFeatureSupport(D3D11_FEATURE(11) /*D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT*/, &featureData, sizeof(D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT));
            
            if (SUCCEEDED(hr) && featureData.SimpleInstancingSupported)
            {
                instancedSupport = true;
            }
        }

        // Compute support is optional on 10.0 and 10.1 targets
        if (featureLevel == D3D_FEATURE_LEVEL_10_0 || featureLevel == D3D_FEATURE_LEVEL_10_1)
        {
            struct D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS
            {
                BOOL ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;
            };

            D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS featureData;
            HRESULT hr = mDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &featureData, sizeof(D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS));

            if (SUCCEEDED(hr) && featureData.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x)
            {
                computeSupport = true;
            }
        }
        else if (featureLevel >= D3D_FEATURE_LEVEL_11_0)
        {
            computeSupport = true;
        }

        IDXGIDevice* dxgiDevice = NULL;

        if (SUCCEEDED(mDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice))))
        {
            IDXGIAdapter* adapter = NULL;

            if (SUCCEEDED(dxgiDevice->GetAdapter(&adapter)))
            {
                DXGI_ADAPTER_DESC desc;

                if (SUCCEEDED(adapter->GetDesc(&desc)))
                {
                    stdext::cvt::wstring_convert<std::codecvt_utf8<wchar_t>> convert;
                    std::string tmp = convert.to_bytes(desc.Description);
                    const char* cstr = tmp.c_str();
                    
                    mCapabilities.vendorStr.appendFormat("VendorID: %x, DeviceID: %x, SubSysID: %x, Revision %x", desc.VendorId, desc.DeviceId, desc.SubSysId, desc.Revision);
                    mCapabilities.deviceStr = cstr;
                    mCapabilities.memoryInfo.appendFormat("%llu (Dedicated Video Memory), %llu (Dedicated System Memory), %llu (Shared System Memory)", desc.DedicatedVideoMemory, desc.DedicatedSystemMemory, desc.SharedSystemMemory);
                }
                adapter->Release();
            }
        }
        dxgiDevice->Release();

        mCapabilities.rendererType = getRendererType();
        mCapabilities.apiVersionStr = getFeatureLevelStr(featureLevel);
        mCapabilities.apiShaderVersionStr = getShaderModelStr(featureLevel);

        // Resource limits: https://msdn.microsoft.com/en-us/library/windows/desktop/ff819065(v=vs.85).aspx

        mCapabilities.numTextureUnits = min(D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT, OMAF_MAX_TEXTURE_UNITS);

        if (featureLevel <= D3D_FEATURE_LEVEL_9_2)
        {
            mCapabilities.maxTexture2DSize = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            mCapabilities.maxTexture3DSize = D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            mCapabilities.maxTextureCubeSize = D3D_FL9_1_REQ_TEXTURECUBE_DIMENSION;
            mCapabilities.maxRenderTargetAttachments = min(D3D_FL9_1_SIMULTANEOUS_RENDER_TARGET_COUNT, OMAF_MAX_RENDER_TARGET_ATTACHMENTS);
        }
        else if (featureLevel == D3D_FEATURE_LEVEL_9_3)
        {
            mCapabilities.maxTexture2DSize = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            mCapabilities.maxTexture3DSize = D3D_FL9_1_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            mCapabilities.maxTextureCubeSize = D3D_FL9_3_REQ_TEXTURECUBE_DIMENSION;
            mCapabilities.maxRenderTargetAttachments = min(D3D_FL9_3_SIMULTANEOUS_RENDER_TARGET_COUNT, OMAF_MAX_RENDER_TARGET_ATTACHMENTS);
        }
        else
        {
            mCapabilities.maxTexture2DSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            mCapabilities.maxTexture3DSize = D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION;
            mCapabilities.maxTextureCubeSize = D3D11_REQ_TEXTURECUBE_DIMENSION;
            mCapabilities.maxRenderTargetAttachments = min(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, OMAF_MAX_RENDER_TARGET_ATTACHMENTS);
        }

        mCapabilities.instancedSupport = instancedSupport;
        mCapabilities.computeSupport = computeSupport;

        mCapabilities.maxComputeWorkGroupCount[0] = D3D11_CS_THREAD_GROUP_MAX_X;
        mCapabilities.maxComputeWorkGroupCount[1] = D3D11_CS_THREAD_GROUP_MAX_Y;
        mCapabilities.maxComputeWorkGroupCount[2] = D3D11_CS_THREAD_GROUP_MAX_Z;

        mCapabilities.maxComputeWorkGroupSize[0] = D3D11_CS_THREAD_GROUP_MAX_X;
        mCapabilities.maxComputeWorkGroupSize[1] = D3D11_CS_THREAD_GROUP_MAX_Y;
        mCapabilities.maxComputeWorkGroupSize[2] = D3D11_CS_THREAD_GROUP_MAX_Z;

        mCapabilities.maxComputeWorkGroupInvocations = D3D11_CS_THREAD_GROUP_MAX_THREADS_PER_GROUP;
        
        // Query supported texture formats
        for (size_t f = 0; f < TextureFormat::COUNT; ++f)
        {
            TextureFormat::Enum textureFormat = (TextureFormat::Enum)f;
            const TextureFormatInfo& textureInfo = getTextureFormatInfoD3D(textureFormat);

            if (DXGI_FORMAT_UNKNOWN != textureInfo.textureFormat)
            {
                struct D3D11_FEATURE_DATA_FORMAT_SUPPORT
                {
                    DXGI_FORMAT InFormat;
                    UINT OutFormatSupport;
                };

                D3D11_FEATURE_DATA_FORMAT_SUPPORT featureData;
                featureData.InFormat = textureInfo.textureFormat;

                HRESULT hr = mDevice->CheckFeatureSupport(D3D11_FEATURE_FORMAT_SUPPORT, &featureData, sizeof(featureData));

                if (SUCCEEDED(hr))
                {
                    mCapabilities.textureFormats[f] = 0 != (featureData.OutFormatSupport & (D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_TEXTURE3D | D3D11_FORMAT_SUPPORT_TEXTURECUBE | featureData.OutFormatSupport&D3D11_FORMAT_SUPPORT_SHADER_SAMPLE));
                }
            }
        }

        resetDefaults();

        createClearStates();
#if WAIT_FOR_FRAME
        D3D11_QUERY_DESC qdesc = { D3D11_QUERY_EVENT };
        HRESULT hr;
        hr = mDevice->CreateQuery(&qdesc, &mQuery);
        if (FAILED(hr))
        {
            OMAF_LOG_E("Could not create synchronization object for rendering.");
        }
#endif
        return true;
    }

    void_t RenderContextD3D11::destroy()
    {
        destroyClearStates();

        mVertexBuffers.clear();
        mIndexBuffers.clear();
        mShaders.clear();
        mShaderConstants.clear();
        mTextures.clear();
        mRenderTargets.clear();
        mActiveComputeResources.clear();

        mCurrentColor = NULL;
        mCurrentDepthStencil = NULL;

        OMAF_DX_RELEASE(mRasterizerState, 0);
        OMAF_DX_RELEASE(mBlendState, 0);
        OMAF_DX_RELEASE(mDepthStencilState, 0);

#if OMAF_DEBUG_BUILD
        OMAF_DX_RELEASE(mAnnotation, 0);
#endif

        if (mDeviceContext != NULL)
        {
            mDeviceContext->ClearState();
            mDeviceContext->Flush();
        }

        OMAF_DX_RELEASE(mDeviceContext, 0);
    }

    void_t RenderContextD3D11::activate()
    {
        setBlendState(mActiveBlendState, true);
        setDepthStencilState(mActiveDepthStencilState, true);
        setRasterizerState(mActiveRasterizerState, true);

        setViewport(mViewport, true);
        setScissors(mScissors, true);
    }

    void_t RenderContextD3D11::deactivate()
    {
        // Unbind active resources
        bindShader(ShaderHandle::Invalid);
        bindIndexBuffer(IndexBufferHandle::Invalid);
        bindVertexBuffer(VertexBufferHandle::Invalid);
        bindRenderTarget(RenderTargetHandle::Invalid);

        for (uint16_t textureUnit = 0; textureUnit < mCapabilities.numTextureUnits; textureUnit++)
        {
            if (mActiveTexture[textureUnit].isValid())
            {
                TextureD3D11& activeTexture = mTextures[mActiveTexture[textureUnit].index];
                activeTexture.unbind(mDeviceContext, textureUnit);

                mActiveTexture[textureUnit] = TextureHandle::Invalid;
            }
        }
#if WAIT_FOR_FRAME
        //Use query to wait for the rendering to complete.
        if (mQuery)
        {
            mDeviceContext->End(mQuery);
            int loops = 0;
            for (;;)
            {
                BOOL result = FALSE;
                HRESULT res = mDeviceContext->GetData(mQuery, &result, sizeof(result), 0);
                if (FAILED(res))
                {
                    //Bad things(tm) device lost or...
                    break;
                }
                if (res == S_OK)
                {
                    if (result == TRUE)
                        break;
                    //OMAF_LOG_D("Waiting S_OK but result not TRUE");
                }
                loops++;
                Sleep(1);// SwitchToThread();//sleep abit, wait for the copy to complete..
            }
/*            if (loops > 2)
            {
                OMAF_LOG_D("Waiting for rendering to finish, sleeped:%d times", loops);
            }*/
        }
#endif

    }

    RenderBackend::Window RenderContextD3D11::getWindow() const
    {
#if OMAF_PLATFORM_UWP

#pragma message ( "RenderContextD3D11::getWindow() not implemented!")
        RenderBackend::Window window;
        window.width = 0;
        window.height = 0;

        return window;

#elif OMAF_PLATFORM_WINDOWS

        RECT clientRect;
        GetClientRect((HWND)mParameters.hWnd, &clientRect);

        RenderBackend::Window window;
        window.width = clientRect.right - clientRect.left;
        window.height = clientRect.bottom - clientRect.top;

        return window;

#else

        OMAF_ASSERT_NOT_IMPLEMENTED();

#endif
    }

    void_t RenderContextD3D11::getCapabilities(RenderBackend::Capabilities& capabitilies) const
    {
        capabitilies = mCapabilities;
    }

    void_t RenderContextD3D11::resetDefaults()
    {
        IRenderContext::resetDefaults();

        setBlendState(mActiveBlendState, true);
        setDepthStencilState(mActiveDepthStencilState, true);
        setRasterizerState(mActiveRasterizerState, true);
        
        bindShader(ShaderHandle::Invalid);
        bindIndexBuffer(IndexBufferHandle::Invalid);
        bindVertexBuffer(VertexBufferHandle::Invalid);
        bindRenderTarget(RenderTargetHandle::Invalid);

        for (uint16_t textureUnit = 0; textureUnit < mCapabilities.numTextureUnits; textureUnit++)
        {
            if (mActiveTexture[textureUnit].isValid())
            {
                TextureD3D11& activeTexture = mTextures[mActiveTexture[textureUnit].index];
                activeTexture.unbind(mDeviceContext, textureUnit);

                mActiveTexture[textureUnit] = TextureHandle::Invalid;
            }
        }

        setViewport(Viewport(0, 0, 0, 0), true);
        setScissors(Scissors(0, 0, 0, 0), true);

        mCurrentColor = NULL;
        mCurrentDepthStencil = NULL;
    }

    bool_t RenderContextD3D11::supportsTextureFormat(TextureFormat::Enum format)
    {
        return false;
    }

    bool_t RenderContextD3D11::supportsRenderTargetFormat(TextureFormat::Enum format)
    {
        return false;
    }

    void_t RenderContextD3D11::setRasterizerState(const RasterizerState& rasterizerState, bool_t forced)
    {
        if (!equals(mActiveRasterizerState, rasterizerState) || forced)
        {
            D3D11_RASTERIZER_DESC desc;
            desc.FillMode = getFillModeD3D(rasterizerState.fillMode);
            desc.CullMode = getCullModeD3D(rasterizerState.cullMode);
            desc.FrontCounterClockwise = (rasterizerState.frontFace == FrontFace::CCW) ? true : false;
            desc.DepthBias = 0;
            desc.DepthBiasClamp = 0.0f;
            desc.SlopeScaledDepthBias = 0.0f;
            desc.DepthClipEnable = FALSE;
            desc.ScissorEnable = rasterizerState.scissorTestEnabled;
            desc.MultisampleEnable = mMSAAEnabled ? TRUE : FALSE;
            desc.AntialiasedLineEnable = FALSE;

            // TODO: Cache state with LRU cache
            OMAF_DX_RELEASE(mRasterizerState, 0);

            ID3D11RasterizerState* rs = NULL;
            OMAF_DX_CHECK(mDevice->CreateRasterizerState(&desc, &rs));

            mDeviceContext->RSSetState(rs);

            mRasterizerState = rs;

            mActiveRasterizerState = rasterizerState;
        }
    }

    void_t RenderContextD3D11::setBlendState(const BlendState& blendState, bool_t forced)
    {
        if (!equals(mActiveBlendState, blendState) || forced)
        {
            D3D11_BLEND_DESC desc;
            desc.AlphaToCoverageEnable = blendState.alphaToCoverageEnabled;
            desc.IndependentBlendEnable = FALSE;
            desc.RenderTarget[0].BlendEnable = blendState.blendEnabled;
            desc.RenderTarget[0].SrcBlend = getBlendFunctionD3D(blendState.blendFunctionSrcRgb);
            desc.RenderTarget[0].DestBlend = getBlendFunctionD3D(blendState.blendFunctionDstRgb);
            desc.RenderTarget[0].BlendOp = getBlendEquationD3D(blendState.blendEquationRgb);
            desc.RenderTarget[0].SrcBlendAlpha = getBlendFunctionD3D(blendState.blendFunctionSrcAlpha);
            desc.RenderTarget[0].DestBlendAlpha = getBlendFunctionD3D(blendState.blendFunctionDstAlpha);
            desc.RenderTarget[0].BlendOpAlpha = getBlendEquationD3D(blendState.blendEquationAlpha);
            desc.RenderTarget[0].RenderTargetWriteMask = getBlendWriteMaskD3D(blendState.blendWriteMask);
            
            mBlendFactor[0] = (blendState.blendFactor[0]) / 255.0f;
            mBlendFactor[1] = (blendState.blendFactor[1]) / 255.0f;
            mBlendFactor[2] = (blendState.blendFactor[2]) / 255.0f;
            mBlendFactor[3] = (blendState.blendFactor[3]) / 255.0f;

            UINT sampleMask = 0xffffffff;

            // TODO: Cache state with LRU cache
            OMAF_DX_RELEASE(mBlendState, 0);

            ID3D11BlendState* bs = NULL;
            OMAF_DX_CHECK(mDevice->CreateBlendState(&desc, &bs));

            mDeviceContext->OMSetBlendState(bs, mBlendFactor, sampleMask);

            mBlendState = bs;

            mActiveBlendState = blendState;
        }
    }

    void_t RenderContextD3D11::setDepthStencilState(const DepthStencilState& depthStencilState, bool_t forced)
    {
        if (!equals(mActiveDepthStencilState, depthStencilState) || forced)
        {
            D3D11_DEPTH_STENCIL_DESC desc;
            desc.DepthEnable = depthStencilState.depthTestEnabled;
            desc.DepthWriteMask = getDepthWriteMaskD3D(depthStencilState.depthWriteMask);
            desc.DepthFunc = getDepthFunctionD3D(depthStencilState.depthFunction);
            desc.StencilEnable = depthStencilState.stencilTestEnabled;
            desc.StencilReadMask = depthStencilState.stencilReadMask;
            desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
            desc.FrontFace.StencilFailOp = getStencilOperationD3D(depthStencilState.frontStencilFailOperation);
            desc.FrontFace.StencilDepthFailOp = getStencilOperationD3D(depthStencilState.frontStencilZFailOperation);
            desc.FrontFace.StencilPassOp = getStencilOperationD3D(depthStencilState.frontStencilZPassOperation);
            desc.FrontFace.StencilFunc = getStencilFunctionD3D(depthStencilState.frontStencilFunction);
            desc.BackFace.StencilFailOp = getStencilOperationD3D(depthStencilState.backStencilFailOperation);
            desc.BackFace.StencilDepthFailOp = getStencilOperationD3D(depthStencilState.backStencilZFailOperation);
            desc.BackFace.StencilPassOp = getStencilOperationD3D(depthStencilState.backStencilZPassOperation);
            desc.BackFace.StencilFunc = getStencilFunctionD3D(depthStencilState.backStencilFunction);

            UINT stencilReference = depthStencilState.stencilReference;

            // TODO: Cache state with LRU cache
            OMAF_DX_RELEASE(mDepthStencilState, 0);

            ID3D11DepthStencilState* dss = NULL;
            OMAF_DX_CHECK(mDevice->CreateDepthStencilState(&desc, &dss));

            mDeviceContext->OMSetDepthStencilState(dss, stencilReference);

            mDepthStencilState = dss;

            mActiveDepthStencilState = depthStencilState;
        }
    }

    void_t RenderContextD3D11::setScissors(const Scissors& scissors, bool_t forced)
    {
        if (!equals(mScissors, scissors) || forced)
        {
            mScissors = scissors;
            Scissors tmp=scissors;
            //Fix the coordinate system.. (from bottom-left (opengl) to top-left (d3d)
            if (mActiveRenderTarget != RenderTargetHandle::Invalid)
            {
                RenderTargetD3D11& renderTarget = mRenderTargets[mActiveRenderTarget.index];
                tmp.y = (renderTarget.getHeight() - tmp.y) - tmp.height;
            }

            D3D11_RECT rect;
            rect.left = tmp.x;
            rect.top = tmp.y;
            rect.right = tmp.x + tmp.width;
            rect.bottom = tmp.y + tmp.height;

            mDeviceContext->RSSetScissorRects(1, &rect);
        }
    }

    void_t RenderContextD3D11::setViewport(const Viewport& viewport, bool_t forced)
    {
        if (!equals(mViewport, viewport) || forced)
        {
            mViewport = viewport;
            
            D3D11_VIEWPORT vp;
            vp.TopLeftX = viewport.x;
            vp.TopLeftY = viewport.y;
            vp.Width = viewport.width;
            vp.Height = viewport.height;
            vp.MinDepth = 0.0f;
            vp.MaxDepth = 1.0f;
            //Fix the coordinate system.. (from bottom-left (opengl) to top-left (d3d)
            if (mActiveRenderTarget != RenderTargetHandle::Invalid)
            {
                RenderTargetD3D11& renderTarget = mRenderTargets[mActiveRenderTarget.index];
                vp.TopLeftY = (renderTarget.getHeight() - vp.TopLeftY) - vp.Height;
            }

            mDeviceContext->RSSetViewports(1, &vp);
        }
    }

    void_t RenderContextD3D11::clearColor(uint32_t color)
    {
        clear(ClearMask::COLOR_BUFFER, color, 0.f, 0);
    }

    void_t RenderContextD3D11::clearDepth(float32_t value)
    {
        clear(ClearMask::DEPTH_BUFFER, 0, value, 0);
    }

    void_t RenderContextD3D11::clearStencil(int32_t value)
    {
        clear(ClearMask::STENCIL_BUFFER, 0, 0.f, value);
    }

    void_t RenderContextD3D11::destroyClearStates()
    {
        mQuadState.mShader.destroy();
        OMAF_DX_RELEASE(mQuadState.mNullState, 0);
        OMAF_DX_RELEASE(mQuadState.mColorState , 0);
        OMAF_DX_RELEASE(mQuadState.mKeepState, 0);
        OMAF_DX_RELEASE(mQuadState.mDepthState, 0);
        OMAF_DX_RELEASE(mQuadState.mStencilState, 0);
        OMAF_DX_RELEASE(mQuadState.mDepthStencilState, 0);
        OMAF_DX_RELEASE(mQuadState.mScissorDisabled, 0);
        OMAF_DX_RELEASE(mQuadState.mScissorEnabled, 0);
        mQuadState.mClearColor.destroy();
        mQuadState.mClearDepth.destroy();
    }

    void_t RenderContextD3D11::createClearStates()
    {
        //create shader for quad clearing.
        const char* vshader = R"(
        cbuffer VS_CONSTANT_BUFFER : register(b0)
        {
            uniform float4 uClearDepth;
            uniform float4 uClearColor;
        };
        struct VSoutput
        {
            float4 pos : SV_POSITION;
            nointerpolation float4 clr : COLOR0;
        };

        VSoutput mainVS(uint id : SV_VertexID)
        {
            static const float2 pts[]=
            {
            float2((0 << 1) & 2, 0 & 2) * float2(2., -2.) + float2(-1., 1.),
            float2((1 << 1) & 2, 1 & 2) * float2(2., -2.) + float2(-1., 1.),
            float2((2 << 1) & 2, 2 & 2) * float2(2., -2.) + float2(-1., 1.),
            float2((3 << 1) & 2, 3 & 2) * float2(2., -2.) + float2(-1., 1.)
            };
            VSoutput ret;
            ret.pos=float4( pts[id], uClearDepth[0], 1.);
            ret.clr=uClearColor;
            return ret;
        }
        )";

        const char* pshader = R"(
        struct VSoutput
        {
            float4 pos : SV_POSITION;
            float4 clr : COLOR0;
        };

        float4 mainPS(VSoutput input) : SV_TARGET
        {
            return input.clr;
        })";

        HRESULT result;
        mQuadState.mShader.create(mDevice, vshader, pshader);   
        mQuadState.mClearColor.create("uClearColor",ShaderConstantType::FLOAT4);
        mQuadState.mClearDepth.create("uClearDepth", ShaderConstantType::FLOAT4);

        //Scissor states.

        D3D11_RASTERIZER_DESC rsdesc;
        rsdesc.FillMode = D3D11_FILL_SOLID;
        rsdesc.CullMode = D3D11_CULL_NONE;
        rsdesc.FrontCounterClockwise = FALSE;
        rsdesc.DepthBias = 0;
        rsdesc.DepthBiasClamp = 0.0f;
        rsdesc.SlopeScaledDepthBias = 0.0f;
        rsdesc.DepthClipEnable = FALSE;
        rsdesc.ScissorEnable = TRUE;
        rsdesc.MultisampleEnable = FALSE;
        rsdesc.AntialiasedLineEnable = FALSE;

        result = mDevice->CreateRasterizerState(&rsdesc, &mQuadState.mScissorEnabled);

        rsdesc.ScissorEnable = FALSE;
        result = mDevice->CreateRasterizerState(&rsdesc, &mQuadState.mScissorDisabled);


        D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

        //Always write depth and stencil
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.StencilEnable = TRUE;
        depthStencilDesc.StencilReadMask = depthStencilDesc.StencilWriteMask = 0xFF;
        depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = depthStencilDesc.BackFace.StencilDepthFailOp = depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, &mQuadState.mDepthStencilState);


        //Always write depth (keep stencil)
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.StencilEnable = FALSE;
        depthStencilDesc.StencilReadMask = depthStencilDesc.StencilWriteMask = 0x00;
        depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = depthStencilDesc.BackFace.StencilDepthFailOp = depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, &mQuadState.mDepthState);


        //Always write stencil (keep depth)
        depthStencilDesc.DepthEnable = TRUE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.StencilEnable = TRUE;
        depthStencilDesc.StencilReadMask = depthStencilDesc.StencilWriteMask = 0xFF;
        depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = depthStencilDesc.BackFace.StencilDepthFailOp = depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, &mQuadState.mStencilState);

        //Keep stencil and depth
        depthStencilDesc.DepthEnable = FALSE;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.StencilEnable = FALSE;
        depthStencilDesc.StencilReadMask = depthStencilDesc.StencilWriteMask = 0x00;
        depthStencilDesc.FrontFace.StencilFailOp = depthStencilDesc.FrontFace.StencilDepthFailOp = depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        depthStencilDesc.BackFace.StencilFailOp = depthStencilDesc.BackFace.StencilDepthFailOp = depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        result = mDevice->CreateDepthStencilState(&depthStencilDesc, &mQuadState.mKeepState);


        D3D11_BLEND_DESC desc = { 0 };
        //ZeroMemory(&desc, sizeof(desc));
        //keep color
        mDevice->CreateBlendState(&desc, &mQuadState.mNullState);

        //write color
        for (int i = 0; i<8; i++)
            desc.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        mDevice->CreateBlendState(&desc, &mQuadState.mColorState);
    }
    void_t RenderContextD3D11::clippedClear(uint16_t clearMask, float clearColor[],float clearDepth,uint32_t stencil)
    {
        float cd[4]={ clearDepth,0,0,0 };
        
        //set our state..
        mQuadState.mShader.setUniform(mDeviceContext, ShaderConstantType::FLOAT4, mQuadState.mClearColor.getHandle(), clearColor);
        mQuadState.mShader.setUniform(mDeviceContext, ShaderConstantType::FLOAT4, mQuadState.mClearDepth.getHandle(), cd);
        ID3D11BlendState* blendState = NULL;
        if (clearMask && ClearMask::COLOR_BUFFER)
        {
            blendState = mQuadState.mColorState;
        }
        else
        {
            blendState = mQuadState.mNullState;
        }
        mDeviceContext->OMSetBlendState(blendState, NULL, 0xffffffff);

        ID3D11DepthStencilState* depthState = NULL;
        if ((clearMask && ClearMask::DEPTH_BUFFER)&&(clearMask && ClearMask::STENCIL_BUFFER))
        {
            depthState = mQuadState.mDepthStencilState;
        }
        else if ((clearMask && ClearMask::DEPTH_BUFFER))
        {
            depthState = mQuadState.mDepthState;
        }
        else if ((clearMask && ClearMask::STENCIL_BUFFER))
        {
            depthState = mQuadState.mStencilState;
        }
        else /*if (!clearDepth && !clearStencil)*/
        {
            depthState = mQuadState.mKeepState;
        }
        mDeviceContext->OMSetDepthStencilState(depthState, stencil);

        if (mActiveRasterizerState.scissorTestEnabled)
        {
            mDeviceContext->RSSetState(mQuadState.mScissorEnabled);
        }
        else
        {
            mDeviceContext->RSSetState(mQuadState.mScissorDisabled);
        }

        /*mDeviceContext->VSSetShader(mQuadState.mVertexShader, nullptr, 0);
        mDeviceContext->VSSetConstantBuffers(0, 1, &mQuadState.mConstantBuffer);
        mDeviceContext->PSSetShader(mQuadState.mPixelShader, nullptr, 0);*/
        mQuadState.mShader.bind(mDeviceContext);
        //context->IASetInputLayout(NULL);
        //UINT stride = 0;
        //UINT offset = 0;
        //context->IASetVertexBuffers(0,0,NULL,&stride,&offset);
        //context->IASetIndexBuffer(NULL,DXGI_FORMAT_R16_UINT,0);
        mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        mQuadState.mShader.flushUniforms(mDeviceContext);
        mDeviceContext->Draw(3, 0);
        //restore state
        mDeviceContext->RSSetState(mRasterizerState);
        UINT sampleMask = 0xffffffff;
        mDeviceContext->OMSetBlendState(mBlendState, mBlendFactor, sampleMask);
        UINT stencilReference = mActiveDepthStencilState.stencilReference;
        mDeviceContext->OMSetDepthStencilState(mDepthStencilState, stencilReference);        
        if (mActiveShader.isValid())
        {
            ShaderD3D11& activeShader = mShaders[mActiveShader.index];
            activeShader.bind(mDeviceContext);
        }
        else
        {
            mQuadState.mShader.unbind(mDeviceContext);
        }
    }

    void_t RenderContextD3D11::clear(uint16_t clearMask, uint32_t color, float32_t depth, int32_t stencil)
    {
        if (mActiveRenderTarget == RenderTargetHandle::Invalid)
        {
            //we dont have a render target to clear...
            return;
        }
        //Since Clear*View does not respect current viewport/scissors
        //we draw a quad (or fullviewport triangle) and fill the values in... (IFF scissor/viewport size does not match rendertarget size!
        RenderTargetD3D11& renderTarget = mRenderTargets[mActiveRenderTarget.index];
        bool_t fullclear = true;

        //Actually only scissors should affect clear. (if we want to match OpenGL)
#if 0
        //Does viewport match render target size?
        if (!equals(mViewport, Viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight())))
        {
            //No.
            fullclear = false;
        }
#endif
        //If scissors enabled
        if (mActiveRasterizerState.scissorTestEnabled)
        {
            //Does scissor area match render target size?
            if (!equals(mScissors, Scissors(0, 0, renderTarget.getWidth(), renderTarget.getHeight())))
            {
                fullclear = false;
            }
        }

        if (!fullclear)
        {
            //Okay, do clipped clear with rendering.
            FLOAT c[4];
            unpackColor(color, c[0], c[1], c[2], c[3]);
            //need to make sure the viewport covers the whole render target..
            Viewport save = mViewport;
            setViewport(Viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight()),false);
            clippedClear(clearMask, c, depth, stencil);
            setViewport(save,false);
        }
        else
        {
            //We can do the fast full surface clear.
            if (mCurrentColor)
            {
                if (clearMask & ClearMask::COLOR_BUFFER)
                {
                    OMAF_ASSERT_NOT_NULL(mCurrentColor);

                    FLOAT c[4];
                    unpackColor(color, c[0], c[1], c[2], c[3]);

                    mDeviceContext->ClearRenderTargetView(mCurrentColor, c);
                }
            }
            if (mCurrentDepthStencil)
            {
                UINT clearFlags = 0;
                if (clearMask & ClearMask::DEPTH_BUFFER)
                {
                    clearFlags |= D3D11_CLEAR_DEPTH;
                }
                if (clearMask & ClearMask::STENCIL_BUFFER)
                {
                    clearFlags |= D3D11_CLEAR_STENCIL;
                }
                mDeviceContext->ClearDepthStencilView(mCurrentDepthStencil, clearFlags, (FLOAT)depth, (UINT8)stencil);
            }
        }
    }

    void_t RenderContextD3D11::bindVertexBuffer(VertexBufferHandle handle)
    {
        if (mActiveVertexBuffer != handle)
        {
            if (handle != VertexBufferHandle::Invalid)
            {
                VertexBufferD3D11& vertexBuffer = mVertexBuffers[handle.index];
                vertexBuffer.bind(mDeviceContext);
            }
            else
            {
                if (mActiveVertexBuffer.isValid())
                {
                    VertexBufferD3D11& activeVertexBuffer = mVertexBuffers[mActiveVertexBuffer.index];
                    activeVertexBuffer.unbind(mDeviceContext);
                }
            }

            mActiveVertexBuffer = handle;
        }
    }

    void_t RenderContextD3D11::bindIndexBuffer(IndexBufferHandle handle)
    {
        if (mActiveIndexBuffer != handle)
        {
            if (handle != IndexBufferHandle::Invalid)
            {
                IndexBufferD3D11& indexBuffer = mIndexBuffers[handle.index];
                indexBuffer.bind(mDeviceContext);
            }
            else
            {
                if (mActiveIndexBuffer.isValid())
                {
                    IndexBufferD3D11& activeIndexBuffer = mIndexBuffers[mActiveIndexBuffer.index];
                    activeIndexBuffer.unbind(mDeviceContext);
                }
            }

            mActiveIndexBuffer = handle;
        }
    }

    void_t RenderContextD3D11::bindShader(ShaderHandle handle)
    {
        if (mActiveShader != handle)
        {
            if (handle != ShaderHandle::Invalid)
            {
                ShaderD3D11& shader = mShaders[handle.index];
                shader.bind(mDeviceContext);
            }
            else
            {
                if (mActiveShader.isValid())
                {
                    ShaderD3D11& activeShader = mShaders[mActiveShader.index];
                    activeShader.unbind(mDeviceContext);
                }
            }

            mActiveShader = handle;
        }
    }

    void_t RenderContextD3D11::bindTexture(TextureHandle handle, uint16_t textureUnit)
    {
        if (mActiveTexture[textureUnit] != handle)
        {
            if (handle != TextureHandle::Invalid)
            {
                TextureD3D11& texture = mTextures[handle.index];
                texture.bind(mDeviceContext, textureUnit);
            }
            else
            {
                if (mActiveTexture[textureUnit].isValid())
                {
                    TextureD3D11& activeTexture = mTextures[mActiveTexture[textureUnit].index];
                    activeTexture.unbind(mDeviceContext, textureUnit);
                }
            }

            mActiveTexture[textureUnit] = handle;
        }
    }

    void_t RenderContextD3D11::bindRenderTarget(RenderTargetHandle handle)
    {
        if (handle == RenderTargetHandle::Invalid) // Settings default framebuffer is special case
        {
            mActiveRenderTarget = RenderTargetHandle::Invalid;

            setViewport(Viewport(0, 0, 0, 0), true);            

            mDeviceContext->OMSetRenderTargets(0, NULL, NULL);

            mCurrentColor = NULL;
            mCurrentDepthStencil = NULL;
        }
        else if (mActiveRenderTarget != handle) // Normal render target switching...
        {
            RenderTargetD3D11& renderTarget = mRenderTargets[handle.index];
            renderTarget.bind(mDeviceContext);

            mActiveRenderTarget = handle;

            mCurrentColor = renderTarget.getRenderTargetViews()[0];
            mCurrentDepthStencil = renderTarget.getDepthStencilView();

            setViewport(Viewport(0, 0, renderTarget.getWidth(), renderTarget.getHeight()), true);            
            setScissors(mScissors, true);//Need to reset, due to topleft/bottomleft d3d/ogl thing.
        }
    }

    void_t RenderContextD3D11::setShaderConstant(ShaderConstantHandle handle, const void_t* values, uint32_t numValues)
    {
        OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

        if (mActiveShader.isValid())
        {
            ShaderConstantD3D11& shaderConstant = mShaderConstants[handle.index];
            ShaderD3D11& shaderProgram = mShaders[mActiveShader.index];

            bool_t result = shaderProgram.setUniform(mDeviceContext, shaderConstant.getType(), shaderConstant.getHandle(), values, numValues);
            OMAF_ASSERT(result, "Shader constant bind failed");
        }
    }

    void_t RenderContextD3D11::setShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const void_t* values, uint32_t numValues)
    {
        ShaderConstantD3D11& shaderConstant = mShaderConstants[constantHandle.index];
        ShaderD3D11& shaderProgram = mShaders[shaderHandle.index];

        bool_t result = shaderProgram.setUniform(mDeviceContext, shaderConstant.getType(), shaderConstant.getHandle(), values, numValues);
        OMAF_ASSERT(result, "Shader constant bind failed");
    }

    void_t RenderContextD3D11::setSamplerState(TextureHandle handle, const SamplerState& samplerState)
    {
        TextureD3D11& texture = mTextures[handle.index];

        texture.setSamplerState(samplerState.addressModeU,
                                samplerState.addressModeV,
                                samplerState.addressModeW,
                                samplerState.filterMode);
    }

    void_t RenderContextD3D11::draw(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
    {
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
        OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

        mShaders[mActiveShader.index].flushUniforms(mDeviceContext);

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = getPrimitiveTypeD3D(primitiveType);
        mDeviceContext->IASetPrimitiveTopology(primitiveTopology);

        UINT numVertices = (UINT)((count == OMAF_UINT32_MAX) ? 0 : count);
        mDeviceContext->Draw((UINT)numVertices, (UINT)offset);
    }

    void_t RenderContextD3D11::drawIndexed(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t count)
    {
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
        OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

        mShaders[mActiveShader.index].flushUniforms(mDeviceContext);

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = getPrimitiveTypeD3D(primitiveType);
        mDeviceContext->IASetPrimitiveTopology(primitiveTopology);

        UINT numIndices = (UINT)((count == OMAF_UINT32_MAX) ? 0 : count);
        mDeviceContext->DrawIndexed((UINT)numIndices, (UINT)offset, 0);
    }

    void_t RenderContextD3D11::drawInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t vertexCount, uint32_t instanceCount)
    {
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
        OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");

        mShaders[mActiveShader.index].flushUniforms(mDeviceContext);

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = getPrimitiveTypeD3D(primitiveType);
        mDeviceContext->IASetPrimitiveTopology(primitiveTopology);

        mDeviceContext->DrawInstanced((UINT)vertexCount,
                                      (UINT)instanceCount,
                                      (UINT)offset,
                                      (UINT)0);
    }

    void_t RenderContextD3D11::drawIndexedInstanced(PrimitiveType::Enum primitiveType, uint32_t offset, uint32_t indexCount, uint32_t instanceCount)
    {
        OMAF_ASSERT(primitiveType != PrimitiveType::INVALID, "");
        OMAF_ASSERT(mActiveShader != ShaderHandle::Invalid, "");
        
        mShaders[mActiveShader.index].flushUniforms(mDeviceContext);

        D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = getPrimitiveTypeD3D(primitiveType);
        mDeviceContext->IASetPrimitiveTopology(primitiveTopology);

        mDeviceContext->DrawIndexedInstanced((UINT)indexCount,
                                             (UINT)instanceCount,
                                             (UINT)offset,
                                             (UINT)0,
                                             (UINT)0);
    }

    void_t RenderContextD3D11::bindComputeImage(uint16_t stage, TextureHandle handle, ComputeBufferAccess::Enum access)
    {
        TextureHandle activeStageHandle = mActiveComputeResources[stage].handle;

        if (activeStageHandle != handle)
        {
            if (handle != TextureHandle::Invalid)
            {
                TextureD3D11& texture = mTextures[handle.index];
                texture.bindCompute(mDeviceContext, stage, access);
            }
            else
            {
                if (activeStageHandle.isValid())
                {
                    TextureD3D11& activeTexture = mTextures[activeStageHandle.index];
                    activeTexture.unbindCompute(mDeviceContext, stage);
                }
            }

            ComputeResourceBinding binding;
            binding.type = ComputeResourceType::IMAGE_BUFFER;
            binding.handle = handle;

            mActiveComputeResources[stage] = binding;
        }
    }

    void_t RenderContextD3D11::bindComputeVertexBuffer(uint16_t stage, VertexBufferHandle handle, ComputeBufferAccess::Enum access)
    {
        VertexBufferHandle activeStageHandle = mActiveComputeResources[stage].handle;

        if (activeStageHandle != handle)
        {
            if (handle != VertexBufferHandle::Invalid)
            {
                VertexBufferD3D11& vertexBuffer = mVertexBuffers[handle.index];
                vertexBuffer.bindCompute(mDeviceContext, stage, access);
            }
            else
            {
                if (activeStageHandle.isValid())
                {
                    VertexBufferD3D11& activeVertexBuffer = mVertexBuffers[activeStageHandle.index];
                    activeVertexBuffer.unbindCompute(mDeviceContext, stage);
                }
            }

            ComputeResourceBinding binding;
            binding.type = ComputeResourceType::VERTEX_BUFFER;
            binding.handle = handle;

            mActiveComputeResources[stage] = binding;
        }
    }

    void_t RenderContextD3D11::bindComputeIndexBuffer(uint16_t stage, IndexBufferHandle handle, ComputeBufferAccess::Enum access)
    {
        IndexBufferHandle activeStageHandle = mActiveComputeResources[stage].handle;

        if (activeStageHandle != handle)
        {
            if (handle != IndexBufferHandle::Invalid)
            {
                IndexBufferD3D11& indexBuffer = mIndexBuffers[handle.index];
                indexBuffer.bindCompute(mDeviceContext, stage, access);
            }
            else
            {
                if (activeStageHandle.isValid())
                {
                    IndexBufferD3D11& activeIndexBuffer = mIndexBuffers[activeStageHandle.index];
                    activeIndexBuffer.unbindCompute(mDeviceContext, stage);
                }
            }

            ComputeResourceBinding binding;
            binding.type = ComputeResourceType::INDEX_BUFFER;
            binding.handle = handle;

            mActiveComputeResources[stage] = binding;
        }
    }

    void_t RenderContextD3D11::dispatchCompute(ShaderHandle handle, uint16_t numGroupsX, uint16_t numGroupsY, uint16_t numGroupsZ)
    {
        bindShader(handle);

        mDeviceContext->Dispatch(numGroupsX, numGroupsY, numGroupsZ);

        // Reset all active compute resources
        for (uint8_t i = 0; i < mActiveComputeResources.getSize(); ++i)
        {
            ComputeResourceBinding& binding = mActiveComputeResources[i];

            if (binding.type == ComputeResourceType::IMAGE_BUFFER)
            {
                TextureHandle handle = binding.handle;

                if (handle.isValid())
                {
                    TextureD3D11& activeTexture = mTextures[handle.index];
                    activeTexture.unbindCompute(mDeviceContext, i);

                    ComputeResourceBinding binding;
                    binding.type = ComputeResourceType::INVALID;
                    binding.handle = TextureHandle::Invalid;

                    mActiveComputeResources[i] = binding;
                }
            }
            else if (binding.type == ComputeResourceType::INDEX_BUFFER)
            {
                IndexBufferHandle handle = binding.handle;

                if (handle.isValid())
                {
                    IndexBufferD3D11& activeIndexBuffer = mIndexBuffers[handle.index];
                    activeIndexBuffer.unbindCompute(mDeviceContext, i);

                    ComputeResourceBinding binding;
                    binding.type = ComputeResourceType::INVALID;
                    binding.handle = IndexBufferHandle::Invalid;

                    mActiveComputeResources[i] = binding;
                }
            }
            else if (binding.type == ComputeResourceType::VERTEX_BUFFER)
            {
                VertexBufferHandle handle = binding.handle;

                if (handle.isValid())
                {
                    VertexBufferD3D11& activeVertexBuffer = mVertexBuffers[handle.index];
                    activeVertexBuffer.unbindCompute(mDeviceContext, i);

                    ComputeResourceBinding binding;
                    binding.type = ComputeResourceType::INVALID;
                    binding.handle = VertexBufferHandle::Invalid;

                    mActiveComputeResources[i] = binding;
                }
            }
        }
    }

    void_t RenderContextD3D11::submitFrame()
    {
    }

    bool_t RenderContextD3D11::createVertexBuffer(VertexBufferHandle handle, const VertexBufferDesc& desc)
    {
        VertexBufferD3D11& vertexBuffer = mVertexBuffers[handle.index];

        return vertexBuffer.create(mDevice,
                                   desc.declaration,
                                   desc.access,
                                   desc.data,
                                   desc.size);
    }

    bool_t RenderContextD3D11::createIndexBuffer(IndexBufferHandle handle, const IndexBufferDesc& desc)
    {
        IndexBufferD3D11& indexBuffer = mIndexBuffers[handle.index];

        return indexBuffer.create(mDevice,
                                  desc.access,
                                  desc.data,
                                  desc.size);
    }

    bool_t RenderContextD3D11::createShader(ShaderHandle handle, const char_t* vertexShader, const char_t* fragmentShader)
    {
        ShaderD3D11& shader = mShaders[handle.index];

        return shader.create(mDevice, vertexShader, fragmentShader);
    }

    bool_t RenderContextD3D11::createShader(ShaderHandle handle, const char_t* computeShader)
    {
        ShaderD3D11& shader = mShaders[handle.index];

        return shader.create(mDevice, computeShader);
    }

    bool_t RenderContextD3D11::createShaderConstant(ShaderHandle shaderHandle, ShaderConstantHandle constantHandle, const char_t* name, ShaderConstantType::Enum type)
    {
        ShaderConstantD3D11& shaderConstant = mShaderConstants[constantHandle.index];

        return shaderConstant.create(name, type);
    }

    bool_t RenderContextD3D11::createTexture(TextureHandle handle, const TextureDesc& desc)
    {
        TextureD3D11& texture = mTextures[handle.index];

        return texture.create(mDevice, mDeviceContext, desc);
    }

    bool_t RenderContextD3D11::createNativeTexture(TextureHandle handle, const NativeTextureDesc& desc)
    {
        TextureD3D11& texture = mTextures[handle.index];

        return texture.createNative(mDevice, mDeviceContext, desc);
    }

    bool_t RenderContextD3D11::createRenderTarget(RenderTargetHandle handle,
                                                  TextureHandle* attachments,
                                                  uint8_t numAttachments,
                                                  int16_t discardMask)
    {
        RenderTargetD3D11& renderTarget = mRenderTargets[handle.index];

        TextureD3D11* textures[OMAF_MAX_RENDER_TARGET_ATTACHMENTS];

        for (uint8_t i = 0; i < numAttachments; ++i)
        {
            TextureHandle attachmentHandle = attachments[i];
            textures[i] = &mTextures[attachmentHandle.index];
        }

        return renderTarget.create(mDevice,
                                   mDeviceContext,
                                   textures,
                                   numAttachments,
                                   discardMask);
    }

    void_t RenderContextD3D11::updateVertexBuffer(VertexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
    {
        VertexBufferD3D11& vertexBuffer = mVertexBuffers[handle.index];
        vertexBuffer.bufferData(mDeviceContext, offset, buffer->data + buffer->offset, buffer->size);
    }

    void_t RenderContextD3D11::updateIndexBuffer(IndexBufferHandle handle, uint32_t offset, const MemoryBuffer* buffer)
    {
        IndexBufferD3D11& indexBuffer = mIndexBuffers[handle.index];
        indexBuffer.bufferData(mDeviceContext, offset, buffer->data + buffer->offset, buffer->size);
    }

    void_t RenderContextD3D11::destroyVertexBuffer(VertexBufferHandle handle)
    {
        VertexBufferD3D11& vertexBuffer = mVertexBuffers[handle.index];
        vertexBuffer.destroy();

        if (mActiveVertexBuffer == handle)
        {
            mActiveVertexBuffer = VertexBufferHandle::Invalid;
        }
    }

    void_t RenderContextD3D11::destroyIndexBuffer(IndexBufferHandle handle)
    {
        IndexBufferD3D11& indexBuffer = mIndexBuffers[handle.index];
        indexBuffer.destroy();

        if (mActiveIndexBuffer == handle)
        {
            mActiveIndexBuffer = IndexBufferHandle::Invalid;
        }
    }

    void_t RenderContextD3D11::destroyShader(ShaderHandle handle)
    {
        ShaderD3D11& shader = mShaders[handle.index];
        shader.destroy();

        if (mActiveShader == handle)
        {
            mActiveShader = ShaderHandle::Invalid;
        }
    }

    void_t RenderContextD3D11::destroyShaderConstant(ShaderConstantHandle handle)
    {
        ShaderConstantD3D11& shaderConstant = mShaderConstants[handle.index];
        shaderConstant.destroy();
    }

    void_t RenderContextD3D11::destroyTexture(TextureHandle handle)
    {
        TextureD3D11& texture = mTextures[handle.index];
        texture.destroy();

        for (size_t textureIndex = 0; textureIndex < OMAF_MAX_TEXTURE_UNITS; ++textureIndex)
        {
            if (mActiveTexture[textureIndex] == handle)
            {
                mActiveTexture[textureIndex] = TextureHandle::Invalid;
            }
        }
    }

    void_t RenderContextD3D11::destroyRenderTarget(RenderTargetHandle handle)
    {
        RenderTargetD3D11& renderTarget = mRenderTargets[handle.index];
        renderTarget.destroy();

        if (mActiveRenderTarget == handle)
        {
            mActiveRenderTarget = RenderTargetHandle::Invalid;
        }
    }

    void_t RenderContextD3D11::pushDebugMarker(const char_t* name)
    {
#if OMAF_DEBUG_BUILD

        WCHAR wString[OMAF_MAX_DEBUG_MARKER_NAME];
        MultiByteToWideChar(CP_ACP, 0, name, -1, wString, 512);

        mAnnotation->BeginEvent(wString);

#endif
    }

    void_t RenderContextD3D11::popDebugMarker()
    {
#if OMAF_DEBUG_BUILD

        mAnnotation->EndEvent();

#endif
    }

    UINT RenderContextD3D11::getMSAASampleCount(DXGI_FORMAT format, UINT sampleCount)
    {
        OMAF_ASSERT_NOT_NULL(mDevice);

        UINT qualityLevels = 0;
        mDevice->CheckMultisampleQualityLevels(format, sampleCount, &qualityLevels);

        while (qualityLevels == 0 || sampleCount > 1)
        {
            sampleCount /= 2;

            mDevice->CheckMultisampleQualityLevels(format, sampleCount, &qualityLevels);
        }

        return sampleCount;
    }
OMAF_NS_END
