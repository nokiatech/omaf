
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
#define _CRT_SECURE_NO_WARNINGS
//Disable windows macro versions of min/max.
#define NOMINMAX
//#define MIRROR_SIDE_BY_SIDE //< render eyes next to each other, instead of only drawing left eye

#include <math.h>
#include <Windows.h>
#include <API/OMAFPlayer.h>
#include <API/OMAFPlayerPlatformParameters.h>
#include <Math/OMAFMatrix44.h>
#include <stdio.h>
#include <pathcch.h>
#include <string>
#include <vector>
#include <fstream>
#include "resource.h"
#pragma comment(lib,"Pathcch.lib")

#define WINDOW_SIZE_EYE 1024

HWND gHwnd;                                         //Application window
OMAF::IOMAFPlayer* gOMAFPlay = NULL;                //OMAF player
OMAF::IPlaybackControls* gPlaybackControl = NULL;   //OMAF playback control interface
OMAF::IRenderer* gRenderer = NULL;                  //OMAF renderer interface
OMAF::IAudio* gAudio = NULL;                        //OMAF audio interface
uint32_t gWindowWidth = 0;                          //App window size
uint32_t gWindowHeight = 0;
float gYaw = 0.0f;                                  //View direction
int gOldX = 0;
bool gNext = false;
bool g_playing = false;
bool g_quit = false;
char g_contentURI[MAX_PATH];
const wchar_t* CONTENT_DIR = L"";
#pragma comment(lib,"OMAFPlayer.lib")

//Select renderer: D3D (better performance)
#define OMAF_USE_D3D11 1
// or default to OpenGL
#ifndef OMAF_USE_D3D11
    #define OMAF_USE_D3D11 0
#endif
#if OMAF_USE_D3D11
    //D3D specifics 
    //D3D error logging macro
    #if OMAF_ENABLE_D3D11_GRAPHICS_DEBUG
        #define DX_CHECK(call)                                                                              \
                {                                                                                           \
                    do                                                                                      \
                    {                                                                                       \
                        call;                                                                               \
                                                                                                            \
                        DWORD errorCode = GetLastError();                                                   \
                        char* errorStr;                                                                     \
                                                                                                            \
                        if (errorCode != 0)                                                                 \
                        {                                                                                   \
                            if(!FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, \
                                                NULL,                                                       \
                                                errorCode,                                                  \
                                                MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),              \
                                                (LPSTR)&errorStr,                                           \
                                                0,                                                          \
                                                NULL))                                                      \
                            {                                                                               \
                                assert(false);                                                              \
                            }                                                                               \
                                                                                                            \
                            LOG_E("D3D11 error:%d %s", int(errorCode), errorStr);                           \
                            LocalFree(errorStr);                                                            \
                                                                                                            \
                            assert(false);                                                                  \
                        }                                                                                   \
                    }                                                                                       \
                    while(false);                                                                           \
                }
    #else
        #define DX_CHECK(call) call
    #endif
    //Convenience macro for releasing D3D components
    #define SAFE_RELEASE(x) if(x != NULL) { x->Release(); x = NULL; }
    #define D3D11_NO_HELPERS
    #include <d3d11_1.h>
    #include <d3dcompiler.h>
    #include <windowsx.h>
    #include <combaseapi.h>
    #pragma comment(lib, "d3d11.lib")
    #pragma comment(lib, "D3DCompiler.lib")
    IDXGISwapChain *gSwapChain;                         // the pointer to the swap chain interface
    ID3D11Device *gD3DDevice;                           // the pointer to our Direct3D device interface
    ID3D11DeviceContext *gD3DDeviceContext;             // the pointer to our Direct3D device context
    ID3D11RenderTargetView* gBackBufferColor;           // backbuffer color view
    ID3D11DepthStencilView* gBackBufferDepthStencil;    // backbuffer depth view 
    ID3D11Texture2D* gDepthStencilBuffer;               // backbuffer depth buffer
    ID3D11VertexShader* mVertexShader;                  // vertex shader
    ID3DBlob* mVertexShaderByteCode;
    ID3D11PixelShader* mPixelShader;                    // pixel shader
    ID3DBlob* mPixelShaderByteCode;

    ID3D11Buffer* mConstantBuffer;                      // shader constants

    // D3D state objects
    ID3D11SamplerState* mSamplerState;
    ID3D11RasterizerState* mRasterizerState;
    ID3D11BlendState* mBlendState;
    ID3D11DepthStencilState* mDepthStencilState;

    struct ConstantBufferData
    {
        OMAF::Matrix44 modelViewProjection;
    };

    struct Vertex
    {
        float x, y, z;
        float u, v;
    };
    ID3D11Buffer* mVertexBuffer;
    ID3D11InputLayout* mInputLayout;

#else // OpenGL renderering
    
    //OpenGL specifics
    #include <gl/GL.h>
    #include <gl/glext.h>
    #include <wgl/wglext.h>
    #pragma comment(lib,"opengl32.lib")

    HDC g_hDC;
    HGLRC g_hGlrc;
#define declare(a,b) a b=NULL;
#include "GlFunctions.inc"
#undef declare

//! define OpenGL extension methods
void initextensions()
{
#define declare(a,b) if (b==NULL) {*((void**)&b)=(void*)wglGetProcAddress(#b);}
#include "GlFunctions.inc"
#undef declare
}

#endif

typedef struct
{
    TCHAR uri[MAX_PATH];
    TCHAR name[MAX_PATH];
} FileURI;
std::vector<FileURI> g_files;

//! handle Windows callbacks
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_MOUSEMOVE:
    {
        int curX = LOWORD(lParam);
        if (wParam&MK_LBUTTON)
        {
            gYaw += (gOldX - curX)*0.0025f;
        }
        gOldX = curX;
        return 0;
    }
    case WM_CREATE:
    {
        return 0;
    }
    case WM_DESTROY:
    {
        // shutdown OMAF Player Engine
        if (gPlaybackControl) gPlaybackControl->stop();
        if (gOMAFPlay) OMAF::IOMAFPlayer::destroy(gOMAFPlay);

        PostQuitMessage(0);

        return 0;
    }

    case WM_PAINT:
    {
        ValidateRect(hwnd, NULL);
        return 0;
    }
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_SPACE:

            // Process the SPACE key. 
            gNext = true;
            break;
        }
        break;
    }
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

//! Simple render target class
class MyRT
{
public:
    OMAF::RenderSurface mSurface; //< render target
#if OMAF_USE_D3D11
    ID3D11Texture2D* colorTexture;                  // color texture
    ID3D11Texture2D* depthStencilTexture;           // depth texture

    // D3D views
    ID3D11ShaderResourceView* shaderResourceView;
    ID3D11RenderTargetView* renderTargetView;
    ID3D11DepthStencilView* depthStencilView;
#else

    GLuint mHandle; //< FBO handle (OpenGL)
    GLuint mTid;    //< texture handle (OpenGL)
    GLuint mDid;    //< depth texture handle (OpenGL)
#endif
};

//! create render target with a projection matrix
MyRT CreateRT(OMAF::EyePosition::Enum eye)
{
    MyRT res;
    memset(&res, 0, sizeof(res));
    res.mSurface.viewport.x = res.mSurface.viewport.y = 0;
    res.mSurface.viewport.width = WINDOW_SIZE_EYE;
    res.mSurface.viewport.height = WINDOW_SIZE_EYE;

    res.mSurface.eyeTransform = OMAF::Matrix44Identity;

    //define eye positions in relation to head
    if (eye == OMAF::EyePosition::LEFT)
    {
        res.mSurface.eyeTransform.m30 = -0.032f;
    }
    else
    {
        res.mSurface.eyeTransform.m30 = 0.032f;
    }

    //create projection matrix with 90 degree field of view
    float fovyInDegrees = 90.f;
    float zFar = 10000.f;
    float zNear = 0.2f;
    float aspect = (float)res.mSurface.viewport.width / (float)res.mSurface.viewport.height;
    const float pi = acosf(0)*2.f;
    float fovy = fovyInDegrees * (pi / 360.f);
    float f = 1.f / tan(fovy);
    res.mSurface.projection = OMAF::Matrix44Identity;
    res.mSurface.projection.m00 = f / aspect;
    res.mSurface.projection.m11 = f;
    res.mSurface.projection.m22 = (zFar + zNear) / (zNear - zFar);
    res.mSurface.projection.m32 = (2 * zFar*zNear) / (zNear - zFar);
    res.mSurface.projection.m23 = -1.f;
    res.mSurface.projection.m33 = 0.f;

#if OMAF_USE_D3D11
    HRESULT result;
    // Color buffer
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
    desc.Width = res.mSurface.viewport.width;
    desc.Height = res.mSurface.viewport.height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
    DX_CHECK(gD3DDevice->CreateTexture2D(&desc, NULL, &res.colorTexture));

    // Shader resource view
    D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
    ZeroMemory(&shaderResourceViewDesc, sizeof(D3D11_SHADER_RESOURCE_VIEW_DESC));
    shaderResourceViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    shaderResourceViewDesc.Texture2D.MipLevels = desc.MipLevels;
    DX_CHECK(gD3DDevice->CreateShaderResourceView(res.colorTexture, &shaderResourceViewDesc, &res.shaderResourceView));

    // Render target view
    D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
    renderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    renderTargetViewDesc.Texture2D.MipSlice = 0;
    DX_CHECK(gD3DDevice->CreateRenderTargetView(res.colorTexture, &renderTargetViewDesc, &res.renderTargetView));

    // Depth stencil buffer
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(D3D11_TEXTURE2D_DESC));
    depthBufferDesc.Width = res.mSurface.viewport.width;
    depthBufferDesc.Height = res.mSurface.viewport.height;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;
    DX_CHECK(gD3DDevice->CreateTexture2D(&depthBufferDesc, NULL, &res.depthStencilTexture));

    // Depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(D3D11_DEPTH_STENCIL_VIEW_DESC));
    depthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;
    DX_CHECK(gD3DDevice->CreateDepthStencilView(res.depthStencilTexture, &depthStencilViewDesc, &res.depthStencilView));

#else
    //create frame buffer
    glGenTextures(1, &res.mTid);
    glBindTexture(GL_TEXTURE_2D, res.mTid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, res.mSurface.viewport.width, res.mSurface.viewport.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glBindTexture(GL_TEXTURE_2D, res.mDid);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, res.mSurface.viewport.width, res.mSurface.viewport.height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenFramebuffers(1, &res.mHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, res.mHandle);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, res.mTid, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, res.mDid, 0);

    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    glClearColor(rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, rand() / (float)RAND_MAX, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif

    if (gOMAFPlay)
    {
        OMAF::RenderTextureDesc colorTextureDesc;
        colorTextureDesc.type = OMAF::TextureType::TEXTURE_2D;
        colorTextureDesc.format = OMAF::RenderTextureFormat::R8G8B8A8;
        colorTextureDesc.width = res.mSurface.viewport.width;
        colorTextureDesc.height = res.mSurface.viewport.height;
#if OMAF_USE_D3D11
        colorTextureDesc.ptr = res.colorTexture;
#else
        colorTextureDesc.ptr = &res.mTid;
#endif
        OMAF::RenderTextureDesc depthTextureDesc;
        depthTextureDesc.type = OMAF::TextureType::TEXTURE_2D;
        depthTextureDesc.format = OMAF::RenderTextureFormat::D32F;
        depthTextureDesc.width = res.mSurface.viewport.width;
        depthTextureDesc.height = res.mSurface.viewport.height;
#if OMAF_USE_D3D11
        depthTextureDesc.ptr = res.depthStencilTexture;
#else
        depthTextureDesc.ptr = &res.mDid;
#endif

        res.mSurface.handle = gOMAFPlay->getRenderer()->createRenderTarget(&colorTextureDesc, &depthTextureDesc);
        int a = 0;
    }
    return res;
}

//! create application Window
int createWindow(HINSTANCE hInstance)
{
    HRESULT hr;
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    // Register the window class.
    const TCHAR CLASS_NAME[] = TEXT("Sample Window Class");

    WNDCLASS wc = {};

    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

#ifndef MIRROR_SIDE_BY_SIDE
    // Create the window.
    gHwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("OMAFPlayer SDK Example"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_SIZE_EYE, WINDOW_SIZE_EYE,
        NULL,
        NULL,
        hInstance,
        NULL
    );
#else
    // Create the window.
    gHwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("OMAFPlayer SDK Example"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_SIZE_EYE*2, WINDOW_SIZE_EYE,
        NULL,
        NULL,
        hInstance,
        NULL
    );
#endif

    if (gHwnd == NULL)
    {
        return -1;
    }

    return 0;
}


#if OMAF_USE_D3D11
int createD3DContext(int nCmdShow, LPRECT rect)
{
    if (gD3DDevice != NULL) return 0;
    GetClientRect(gHwnd, rect);
    gWindowWidth = rect->right - rect->left;
    gWindowHeight = rect->bottom - rect->top;
    // Initialize device
    HRESULT result = S_OK;
    D3D_FEATURE_LEVEL featureLevel;

    result = D3D11CreateDevice(NULL,
        D3D_DRIVER_TYPE_HARDWARE,
        NULL,
        0,
        NULL,
        0,
        D3D11_SDK_VERSION,
        &gD3DDevice,
        &featureLevel,
        &gD3DDeviceContext);
    if (FAILED(result)) return result;

    // Obtain DXGI factory from device
    IDXGIFactory1* dxgiFactory = NULL;
    {
        IDXGIDevice* dxgiDevice = NULL;
        result = gD3DDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
        if (SUCCEEDED(result))
        {
            IDXGIAdapter* adapter = NULL;
            result = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(result))
            {
                result = adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory));
                adapter->Release();
                adapter = NULL;
            }
            dxgiDevice->Release();
            dxgiDevice = NULL;
        }
    }
    if (FAILED(result)) return result;

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc.Width = gWindowWidth;
    swapChainDesc.BufferDesc.Height = gWindowHeight;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = gHwnd;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    result = dxgiFactory->CreateSwapChain(gD3DDevice, &swapChainDesc, &gSwapChain);
    // Prevent full-screen
    dxgiFactory->MakeWindowAssociation(gHwnd, DXGI_MWA_NO_ALT_ENTER);

    dxgiFactory->Release();
    dxgiFactory = NULL;


    if (FAILED(result)) return result;

    // Get color back buffer
    ID3D11Texture2D* colorBuffer;
    result = gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&colorBuffer));

    if (FAILED(result)) return result;

    // Create a render target view
    result = gD3DDevice->CreateRenderTargetView(colorBuffer, NULL, &gBackBufferColor);

    colorBuffer->Release();
    colorBuffer = NULL;

    if (FAILED(result)) return result;

    // Create depth stencil buffer
    D3D11_TEXTURE2D_DESC depthBufferDesc;
    ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));
    depthBufferDesc.Width = gWindowWidth;
    depthBufferDesc.Height = gWindowHeight;
    depthBufferDesc.MipLevels = 1;
    depthBufferDesc.ArraySize = 1;
    depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthBufferDesc.SampleDesc.Count = 1;
    depthBufferDesc.SampleDesc.Quality = 0;
    depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthBufferDesc.CPUAccessFlags = 0;
    depthBufferDesc.MiscFlags = 0;

    result = gD3DDevice->CreateTexture2D(&depthBufferDesc, NULL, &gDepthStencilBuffer);

    if (FAILED(result)) return result;

    // Create depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
    ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
    depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    depthStencilViewDesc.Texture2D.MipSlice = 0;

    result = gD3DDevice->CreateDepthStencilView(gDepthStencilBuffer, &depthStencilViewDesc, &gBackBufferDepthStencil);

    if (FAILED(result)) return result;

    // Create shader
    const char* shader = R"(
        cbuffer ContantBufferData0 : register(b0)
        {
            matrix uModelViewProjection;
        }

        Texture2D Texture0  : register(t0);
        SamplerState Sampler0 : register(s0);

        struct VS_INPUT
        {
            float3 Position : POSITION;
            float2 TexCoord0 : TEXCOORD0;
        };

        struct PS_INPUT
        {
            float4 Position : SV_POSITION;
            float2 TexCoord0 : TEXCOORD0;
        };

        PS_INPUT mainVS(VS_INPUT input)
        {
            float4 position = float4(input.Position, 1.0f);

            PS_INPUT output = (PS_INPUT)0;
            output.Position = mul(uModelViewProjection, position);
            output.TexCoord0 = float2(input.TexCoord0.x, 1.0f - input.TexCoord0.y);

            return output;
        }

        float4 mainPS(PS_INPUT input) : SV_TARGET
        {
            return Texture0.Sample(Sampler0, input.TexCoord0);
        })";

    DWORD shaderFlags = (1 << 15); //D3DCOMPILE_OPTIMIZATION_LEVEL3
    // Create vertex shader
    {
        ID3DBlob* byteCode = NULL;
        ID3DBlob* blobError = NULL;

        HRESULT result = D3DCompile(shader,
            strlen(shader),
            NULL,
            NULL,
            NULL,
            "mainVS",
            "vs_5_0", // https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx
            shaderFlags, // https://msdn.microsoft.com/en-us/library/windows/desktop/gg615083(v=vs.85).aspx
            NULL,
            &byteCode,
            &blobError);

        if (FAILED(result))
        {
            SAFE_RELEASE(blobError);
            SAFE_RELEASE(byteCode);

            return OMAF::Result::OPERATION_FAILED;
        }

        mVertexShaderByteCode = byteCode;
    }
    {
        HRESULT result = gD3DDevice->CreateVertexShader(mVertexShaderByteCode->GetBufferPointer(),
            mVertexShaderByteCode->GetBufferSize(),
            NULL,
            &mVertexShader);

        if (FAILED(result))
        {
            return OMAF::Result::OPERATION_FAILED;
        }
    }
    // Create pixel shader
    {
        ID3DBlob* byteCode = NULL;
        ID3DBlob* blobError = NULL;

        HRESULT result = D3DCompile(shader,
            strlen(shader),
            NULL,
            NULL,
            NULL,
            "mainPS",
            "ps_5_0", // https://msdn.microsoft.com/en-us/library/windows/desktop/jj215820(v=vs.85).aspx
            shaderFlags, // https://msdn.microsoft.com/en-us/library/windows/desktop/gg615083(v=vs.85).aspx
            NULL,
            &byteCode,
            &blobError);

        if (FAILED(result))
        {
            SAFE_RELEASE(blobError);
            SAFE_RELEASE(byteCode);

            return OMAF::Result::OPERATION_FAILED;
        }

        mPixelShaderByteCode = byteCode;
    }
    {
        HRESULT result = gD3DDevice->CreatePixelShader(mPixelShaderByteCode->GetBufferPointer(),
            mPixelShaderByteCode->GetBufferSize(),
            NULL,
            &mPixelShader);

        if (FAILED(result))
        {
            return OMAF::Result::OPERATION_FAILED;
        }
    }

    // Create shader contant buffer
    D3D11_BUFFER_DESC constantBufferDesc;
    ZeroMemory(&constantBufferDesc, sizeof(D3D11_BUFFER_DESC));
    constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    constantBufferDesc.ByteWidth = sizeof(OMAF::Matrix44);
    constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDesc.CPUAccessFlags = 0;

    result = gD3DDevice->CreateBuffer(&constantBufferDesc, NULL, &mConstantBuffer);
    if (FAILED(result)) return result;
    // Create PSOs
    {
        D3D11_RASTERIZER_DESC desc;
        desc.FillMode = D3D11_FILL_SOLID;
        desc.CullMode = D3D11_CULL_NONE;
        desc.FrontCounterClockwise = TRUE;
        desc.DepthBias = 0;
        desc.DepthBiasClamp = 0.0f;
        desc.SlopeScaledDepthBias = 0.0f;
        desc.DepthClipEnable = FALSE;
        desc.ScissorEnable = TRUE;
        desc.MultisampleEnable = FALSE;
        desc.AntialiasedLineEnable = FALSE;

        result = gD3DDevice->CreateRasterizerState(&desc, &mRasterizerState);
        if (FAILED(result)) return result;
    }

    {
        D3D11_BLEND_DESC desc;
        desc.AlphaToCoverageEnable = FALSE;
        desc.IndependentBlendEnable = FALSE;
        desc.RenderTarget[0].BlendEnable = FALSE;
        desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
        desc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        desc.RenderTarget[0].RenderTargetWriteMask = (0 | D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE | D3D11_COLOR_WRITE_ENABLE_ALPHA);

        result = gD3DDevice->CreateBlendState(&desc, &mBlendState);
        if (FAILED(result)) return result;
    }

    {
        D3D11_DEPTH_STENCIL_DESC desc;
        desc.DepthEnable = FALSE;
        desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        desc.DepthFunc = D3D11_COMPARISON_LESS;
        desc.StencilEnable = FALSE;
        desc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        desc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
        desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
        desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
        desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

        result = gD3DDevice->CreateDepthStencilState(&desc, &mDepthStencilState);
        if (FAILED(result)) return result;
    }

    // Create sampler
    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;

    result = gD3DDevice->CreateSamplerState(&samplerDesc, &mSamplerState);
    if (FAILED(result)) return result;

    // Create vertex buffer
    Vertex vertices[6] =
    {
    { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f },
    { 1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
    { -1.0f, 1.0f, 0.0f, 0.0f, 1.0f },

    { -1.0f, 1.0f, 0.0f, 0.0f, 1.0f },
    { 1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
    { 1.0f, 1.0f, 0.0f, 1.0f, 1.0f },
    };

    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.ByteWidth = sizeof(vertices);
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA subresourceData;
    ZeroMemory(&subresourceData, sizeof(D3D11_SUBRESOURCE_DATA));
    subresourceData.pSysMem = vertices;

    result = gD3DDevice->CreateBuffer(&bufferDesc, &subresourceData, &mVertexBuffer);
    if (FAILED(result)) return result;

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    UINT numElements = ARRAYSIZE(layout);

    result = gD3DDevice->CreateInputLayout(layout, numElements, mVertexShaderByteCode->GetBufferPointer(), mVertexShaderByteCode->GetBufferSize(), &mInputLayout);
    if (FAILED(result)) return result;

    ShowWindow(gHwnd, nCmdShow);
    return OMAF::Result::OK;
}

bool destroyD3DContext()
{
    gSwapChain->SetFullscreenState(FALSE, NULL);

    SAFE_RELEASE(mVertexShader);
    SAFE_RELEASE(mPixelShader);
    SAFE_RELEASE(mVertexShaderByteCode);
    SAFE_RELEASE(mPixelShaderByteCode);
    SAFE_RELEASE(mConstantBuffer);
    SAFE_RELEASE(mSamplerState);
    SAFE_RELEASE(mRasterizerState);
    SAFE_RELEASE(mBlendState);
    SAFE_RELEASE(mDepthStencilState);
    SAFE_RELEASE(mVertexBuffer);
    SAFE_RELEASE(mInputLayout);

    SAFE_RELEASE(gBackBufferColor);
    SAFE_RELEASE(gBackBufferDepthStencil);
    SAFE_RELEASE(gDepthStencilBuffer);
    SAFE_RELEASE(gSwapChain);

    if (gD3DDeviceContext != NULL)
    {
        gD3DDeviceContext->ClearState();
        gD3DDeviceContext->Flush();
    }
    SAFE_RELEASE(gD3DDeviceContext);
    SAFE_RELEASE(gD3DDevice);

    return true;
}

void RenderD3D(MyRT* leftRT, MyRT* rightRT)
{
    // Set default back buffer
    DX_CHECK(gD3DDeviceContext->OMSetRenderTargets(1, &gBackBufferColor, NULL));

    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = gWindowWidth;
    viewport.Height = gWindowHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    DX_CHECK(gD3DDeviceContext->RSSetViewports(1, &viewport));
    // Rasterizer state
    DX_CHECK(gD3DDeviceContext->RSSetState(mRasterizerState));

    // Blend state
    FLOAT blendFactor[4];
    blendFactor[0] = 1.0f;
    blendFactor[1] = 1.0f;
    blendFactor[2] = 1.0f;
    blendFactor[3] = 1.0f;

    UINT sampleMask = 0xffffffff;

    DX_CHECK(gD3DDeviceContext->OMSetBlendState(mBlendState, blendFactor, sampleMask));

    // Depth stencil state
    UINT stencilReference = 0;

    DX_CHECK(gD3DDeviceContext->OMSetDepthStencilState(mDepthStencilState, stencilReference));

    // Vertex buffer
    UINT stride = sizeof(Vertex);
    UINT offset = 0;

    DX_CHECK(gD3DDeviceContext->IASetVertexBuffers(0, 1, &mVertexBuffer, &stride, &offset));
    DX_CHECK(gD3DDeviceContext->IASetInputLayout(mInputLayout));

    // Shader
    DX_CHECK(gD3DDeviceContext->VSSetShader(mVertexShader, NULL, 0));
    DX_CHECK(gD3DDeviceContext->PSSetShader(mPixelShader, NULL, 0));

    OMAF::Matrix44 projection = OMAF::makeOrthographic(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

#ifndef MIRROR_SIDE_BY_SIDE
    {
        // Set scissors
        D3D11_RECT rect;
        rect.left = 0;
        rect.top = 0;
        rect.right = gWindowWidth;
        rect.bottom = gWindowHeight;

        DX_CHECK(gD3DDeviceContext->RSSetScissorRects(1, &rect));

        // Shader constant buffer
        OMAF::Matrix44 mvp = projection;

        ConstantBufferData constantBufferData;
        memcpy(&constantBufferData.modelViewProjection, &mvp, sizeof(constantBufferData.modelViewProjection));

        DX_CHECK(gD3DDeviceContext->UpdateSubresource(mConstantBuffer, 0, NULL, &constantBufferData, 0, 0));

        DX_CHECK(gD3DDeviceContext->VSSetConstantBuffers(0, 1, &mConstantBuffer));
        DX_CHECK(gD3DDeviceContext->PSSetConstantBuffers(0, 1, &mConstantBuffer));

        // Texture
        DX_CHECK(gD3DDeviceContext->PSSetShaderResources(0, 1, &leftRT->shaderResourceView));
        DX_CHECK(gD3DDeviceContext->PSSetSamplers(0, 1, &mSamplerState));

        // Draw
        DX_CHECK(gD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
        DX_CHECK(gD3DDeviceContext->Draw(6, 0));
    }
#else
    {
        {
            // Set scissors
            D3D11_RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = gWindowWidth / 2;
            rect.bottom = gWindowHeight;

            DX_CHECK(gD3DDeviceContext->RSSetScissorRects(1, &rect));

            // Shader constant buffer
            OMAF::Matrix44 mvp = projection * OMAF::makeTranslation(-0.5f, 0.0f, 0.0f) * OMAF::makeScale(0.5f, 1.0f, 1.0f);

            ConstantBufferData constantBufferData;
            memcpy(&constantBufferData.modelViewProjection, &mvp, sizeof(constantBufferData.modelViewProjection));

            DX_CHECK(gD3DDeviceContext->UpdateSubresource(mConstantBuffer, 0, NULL, &constantBufferData, 0, 0));

            DX_CHECK(gD3DDeviceContext->VSSetConstantBuffers(0, 1, &mConstantBuffer));
            DX_CHECK(gD3DDeviceContext->PSSetConstantBuffers(0, 1, &mConstantBuffer));

            // Texture
            DX_CHECK(gD3DDeviceContext->PSSetShaderResources(0, 1, &leftRT->shaderResourceView));
            DX_CHECK(gD3DDeviceContext->PSSetSamplers(0, 1, &mSamplerState));

            // Draw
            DX_CHECK(gD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
            DX_CHECK(gD3DDeviceContext->Draw(6, 0));
        }

        {
            // Set scissors
            D3D11_RECT rect;
            rect.left = gWindowWidth / 2;
            rect.top = 0;
            rect.right = gWindowWidth;
            rect.bottom = gWindowHeight;

            DX_CHECK(gD3DDeviceContext->RSSetScissorRects(1, &rect));

            // Shader constant buffer
            OMAF::Matrix44 mvp = projection * OMAF::makeTranslation(0.5f, 0.0f, 0.0f) * OMAF::makeScale(0.5f, 1.0f, 1.0f);

            ConstantBufferData constantBufferData;
            memcpy(&constantBufferData.modelViewProjection, &mvp, sizeof(constantBufferData.modelViewProjection));

            DX_CHECK(gD3DDeviceContext->UpdateSubresource(mConstantBuffer, 0, NULL, &constantBufferData, 0, 0));

            DX_CHECK(gD3DDeviceContext->VSSetConstantBuffers(0, 1, &mConstantBuffer));
            DX_CHECK(gD3DDeviceContext->PSSetConstantBuffers(0, 1, &mConstantBuffer));

            // Texture
            DX_CHECK(gD3DDeviceContext->PSSetShaderResources(0, 1, &rightRT->shaderResourceView));
            DX_CHECK(gD3DDeviceContext->PSSetSamplers(0, 1, &mSamplerState));

            // Draw
            DX_CHECK(gD3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST));
            DX_CHECK(gD3DDeviceContext->Draw(6, 0));
        }
    }
#endif
    // Cleanup
    ID3D11RasterizerState* nullRS = NULL;
    DX_CHECK(gD3DDeviceContext->RSSetState(nullRS));

    ID3D11BlendState* nullBS = NULL;
    DX_CHECK(gD3DDeviceContext->OMSetBlendState(nullBS, blendFactor, sampleMask));

    ID3D11DepthStencilState* nullDS = NULL;
    DX_CHECK(gD3DDeviceContext->OMSetDepthStencilState(nullDS, stencilReference));

    DX_CHECK(gD3DDeviceContext->VSSetShader(NULL, NULL, 0));
    DX_CHECK(gD3DDeviceContext->PSSetShader(NULL, NULL, 0));

    DX_CHECK(gD3DDeviceContext->IASetInputLayout(NULL));

    ID3D11Buffer* nullBuffer = NULL;
    DX_CHECK(gD3DDeviceContext->VSSetConstantBuffers(0, 1, &nullBuffer));
    DX_CHECK(gD3DDeviceContext->PSSetConstantBuffers(0, 1, &nullBuffer));

    ID3D11ShaderResourceView* nullSRV = NULL;
    DX_CHECK(gD3DDeviceContext->PSSetShaderResources(0, 1, &nullSRV));

    ID3D11SamplerState* nullSampler = NULL;
    DX_CHECK(gD3DDeviceContext->PSSetSamplers(0, 1, &nullSampler));

    DX_CHECK(gD3DDeviceContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL));
}

#else
int createOpenGLContext(int nCmdShow, LPRECT rect)
{
    g_hDC = GetDC(gHwnd);

    PIXELFORMATDESCRIPTOR pfd = { 0 };
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_GENERIC_ACCELERATED | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;
    {
        int id = ChoosePixelFormat(g_hDC, &pfd);
        DescribePixelFormat(g_hDC, id, sizeof(pfd), &pfd);
        SetPixelFormat(g_hDC, id, &pfd);
    }

    //First create a dummy context so we can create actual OpenGL 3.2 core context.
    HGLRC dummy = wglCreateContext(g_hDC);
    if (dummy == NULL)
    {
        MessageBoxA(NULL, "Dummy context creation failed!", "Fatal Error!", MB_ICONSTOP | MB_APPLMODAL);
        return -1;
    }
    wglMakeCurrent(g_hDC, dummy);

    int attribs[] =
    {
    WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
    WGL_CONTEXT_MINOR_VERSION_ARB, 3,                               //Version 4.3
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB, //Request CORE profile
    0
    };

    *((void**)&wglCreateContextAttribsARB) = (void*)wglGetProcAddress("wglCreateContextAttribsARB");
    if (wglCreateContextAttribsARB == NULL)
    {
        MessageBoxA(NULL, "Can not create context, wglCreateContextAttribsARB not supported!", "Fatal Error!", MB_ICONSTOP | MB_APPLMODAL);
        return -1;
    }
    g_hGlrc = wglCreateContextAttribsARB(g_hDC, 0, attribs);
    if (g_hGlrc == NULL)
    {
        //4.3 Core context apparently not supported.
        MessageBoxA(NULL, "Could not create GLRC for window!", "Fatal Error!", MB_ICONSTOP | MB_APPLMODAL);
        return -1;
    }
    wglCreateContextAttribsARB = NULL;
    //Delete the dummy context, since we dont need it anymore.
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(dummy);

    //Activate the actual context.
    wglMakeCurrent(g_hDC, g_hGlrc);
    initextensions();
    ShowWindow(gHwnd, nCmdShow);

    GetClientRect(gHwnd, rect);
    return 0;
}
#endif
// Content selection dialog callback

BOOL CALLBACK ItemSelectedProc(HWND hwndDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            HWND hwndList = GetDlgItem(hwndDlg, IDC_LIST1);
            int selectedItemIndex = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);
            int indexOfFile = (int)SendMessage(hwndList, LB_GETITEMDATA, selectedItemIndex, 0);
            if (indexOfFile >= 0 && indexOfFile < g_files.size())
            {
                // Start playback
                int i = wcstombs(g_contentURI, g_files[indexOfFile].uri, MAX_PATH);
                OMAF::Result::Enum res;
                res = gPlaybackControl->loadVideo(g_contentURI);
                if (res == OMAF::Result::OK) res = gPlaybackControl->play();
                if (res == OMAF::Result::OK) { g_playing = true;}
                else { g_quit = true; }
            }
            ShowWindow(gHwnd, SW_SHOW);
            EndDialog(hwndDlg, wParam);
            SetFocus(gHwnd);
            return TRUE;
        }
        case IDCANCEL:
            g_playing = false;
            g_quit = true;
            ShowWindow(gHwnd, SW_SHOW);
            EndDialog(hwndDlg, wParam);
            SetFocus(gHwnd);
            return TRUE;
        }
    }
    return FALSE;
}

/*
    Content selection
*/

// List contents
void ListContents() {
    WIN32_FIND_DATA ffd;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    wchar_t path[MAX_PATH];
    _wgetcwd(path, MAX_PATH);
    PathCchCombineEx(path,MAX_PATH,path,CONTENT_DIR,0);

    wchar_t search_path[MAX_PATH];
    PathCchCombineEx(search_path, MAX_PATH, path, L"*.*", 0);
    std::vector<FileURI> files;

    // For now we just add the .mp4 and .heic files in the Assets folder. Any other content can be added to the vector
    if (!((hFind = FindFirstFile(search_path, &ffd)) == INVALID_HANDLE_VALUE)) {
        while (FindNextFile(hFind, &ffd)) {
            printf("File %s", ffd.cFileName);
            std::wstring s = std::wstring(ffd.cFileName);
            size_t pos = s.find_last_of(L".");
            if (pos == std::wstring::npos)
            {
                continue;
            }
            std::wstring fileType = std::wstring(s.substr(pos, s.length() - pos));
            if (fileType.compare(L".mp4") == 0 || fileType.compare(L".heic") == 0)
            {
                wchar_t fullURI[MAX_PATH];
                PathCchCombineEx(fullURI, MAX_PATH, path, ffd.cFileName, 0);

                FileURI uri;
                lstrcpyn(uri.uri, std::wstring(std::wstring(L"storage://") + fullURI).c_str(), MAX_PATH);
                lstrcpyn(uri.name, s.c_str(), MAX_PATH);
                files.push_back(uri);
            }
            else if (fileType.compare(L".uri") == 0)
            {
                std::string videoUri;
                std::wstring fullName(path);
                fullName += L"\\" + s;
                std::ifstream fstream(fullName);
                if (fstream)
                {
                    std::getline(fstream, videoUri);
                    {
                        if (videoUri.length() > 0 && videoUri.find("://") != std::string::npos)
                        {
                            FileURI uri;
                            std::wstring wsTmp(videoUri.begin(), videoUri.end());
                            lstrcpyn(uri.uri, wsTmp.c_str(), MAX_PATH);
                            lstrcpyn(uri.name, s.c_str(), MAX_PATH);
                            files.push_back(uri);
                        }
                    }
                    fstream.close();
                }
            }

        }
    }
    g_files = files;
    FindClose(hFind);
}


// Content selection dialog

void OpenListSelectionDialog() {
    
     HWND hwnd = CreateDialog(NULL,
        MAKEINTRESOURCE(IDD_DIALOG1),
         gHwnd,
        (DLGPROC)ItemSelectedProc);
    HWND hwndList = GetDlgItem(hwnd, IDC_LIST1);
    for (int i = 0; i < g_files.size(); i++)
    {
        int pos = (int)SendMessage(hwndList, LB_ADDSTRING, 0,
            (LPARAM)&g_files[i].name);
        SendMessage(hwndList, LB_SETITEMDATA, pos, (LPARAM)i);
    }
    ShowWindow(hwnd, SW_SHOW);
    ShowWindow(gHwnd, SW_HIDE);
    SetFocus(hwnd);
}
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    // Setup application Window and OpenGL rendering context
    int initRes = 0;
    initRes = createWindow(hInstance);
    if (initRes != 0)
        return initRes;
    RECT rect;
#if OMAF_USE_D3D11
    initRes = createD3DContext(nCmdShow, &rect);
#else
    initRes = createOpenGLContext(nCmdShow, &rect);
#endif
    if (initRes != 0)
        return initRes;

    int windowWidth = rect.right;
    int windowHeight = rect.bottom;

    // Create and initialize OMAF Player Engine
    OMAF::PlatformParameters params;
    params.hWnd = gHwnd;
    params.hInstance = hInstance;
    params.assetPath = "";
    params.storagePath = "";

#if OMAF_USE_D3D11
    params.d3dDevice = gD3DDevice;
    gOMAFPlay = OMAF::IOMAFPlayer::create(OMAF::GraphicsAPI::D3D11, &params);
#else
    gOMAFPlay = OMAF::IOMAFPlayer::create(OMAF::GraphicsAPI::OPENGL, &params);
#endif
    gPlaybackControl = gOMAFPlay->getPlaybackControls();
    gAudio = gOMAFPlay->getAudio();
    gRenderer = gOMAFPlay->getRenderer();
    OMAF::Result::Enum res;
    res = gAudio->initializeAudioWithDirectRouting();
    res = gAudio->setGain(0.4f);

    // check if there was a filename (local or url) in the command line
    size_t args = wcslen(pCmdLine);
    if (args > 0 && args < 2047)
    {
        char *str = new char[2048];
        wcstombs(str, pCmdLine, args);
        str[args] = '\0';
        res = gPlaybackControl->loadVideo(str);
        if (res == OMAF::Result::OK) { g_playing = true; }
    }
    else
    {
        ListContents();
        OpenListSelectionDialog();
    }
    // Start playback
    res = gPlaybackControl->play();

    //Create rendertargets.
    MyRT leftRT = CreateRT(OMAF::EyePosition::LEFT);
    MyRT rightRT = CreateRT(OMAF::EyePosition::RIGHT);

    MSG msg = {};
    //main loop
    for (;;)
    {
        if (g_quit) break;
        //process Windows messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                g_quit = true;
            }
        }
        else if(g_playing)
        {
            if (gNext)
            {
                gPlaybackControl->next();
                gNext = false;
                continue;
            }
            //rotate only around y-axis for this sample
            OMAF::HeadTransform head = { 0 };
            head.orientation.x = 0;
            head.orientation.y = sin(gYaw / 2.f);
            head.orientation.z = 0;
            head.orientation.w = cos(gYaw / 2.f);

            //Update head transform to rendering
            if (gRenderer)
            {
                //render video for both eye surfaces
                const OMAF::RenderSurface* surfaces[] = { &leftRT.mSurface, &rightRT.mSurface };

                OMAF::RenderingParameters renderingParameters;
                renderingParameters.clearColor = OMAF::makeColor4(0.0f, 0.0f, 0.0f, 1.0f);
                renderingParameters.displayWaterMark = false;
                gRenderer->renderSurfaces(head, surfaces, 2, renderingParameters);
            }
#if OMAF_USE_D3D11
            RenderD3D(&leftRT, &rightRT);
            gD3DDeviceContext->Flush();
            gSwapChain->Present(1, 0);
#else
            //Render the textures to window
            glDisable(GL_SCISSOR_TEST);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#ifdef MIRROR_SIDE_BY_SIDE
            glBindFramebuffer(GL_READ_FRAMEBUFFER, leftRT.mHandle);
            glBlitFramebuffer(0, 0, leftRT.mSurface.viewport.width, leftRT.mSurface.viewport.height, 0, 0, windowWidth / 2, windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, rightRT.mHandle);
            glBlitFramebuffer(0, 0, rightRT.mSurface.viewport.width, rightRT.mSurface.viewport.height, windowWidth / 2, 0, windowWidth, windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#else
            glBindFramebuffer(GL_READ_FRAMEBUFFER, leftRT.mHandle);
            glBlitFramebuffer(0, 0, leftRT.mSurface.viewport.width, leftRT.mSurface.viewport.height, 0, 0, windowWidth, windowHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

            glFinish();
            SwapBuffers(g_hDC);
#endif
        }
    }
#if OMAF_USE_D3D11
    destroyD3DContext();
#endif
    CoUninitialize();
    return 0;
}
