
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
#define _CRT_SECURE_NO_WARNINGS
//Disable windows macro versions of min/max.
#define NOMINMAX
//#define MIRROR_SIDE_BY_SIDE //< render eyes next to each other, instead of only drawing left eye. Note! it is still not stereo

#include <math.h>
#include <Windows.h>
#include <API/OMAFPlayer.h>
#include <API/OMAFPlayerPlatformParameters.h>
#include <Math/OMAFMatrix44.h>
#include <gl/GL.h>
#include "gl/glext.h"
#include "wgl/wglext.h"
#include <stdio.h>
#pragma comment(lib,"opengl32.lib")
#pragma comment(lib,"OMAFPlayer.lib")

HWND g_hWnd;
HDC g_hDC;
HGLRC g_hGlrc;

OMAF::IOMAFPlayer* gOMAFPlay = NULL;
OMAF::IPlaybackControls* gPlaybackControl = NULL;
OMAF::IRenderer* gRenderer = NULL;
OMAF::IAudio* gAudio = NULL;
float gYaw = 0.0f;
int gOldX = 0;
bool gNext = false;

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
    GLuint mHandle; //< FBO handle (OpenGL)
    GLuint mTid;    //< texture handle (OpenGL)
    GLuint mDid;    //< depth texture handle (OpenGL)
};

//! create render target with a projection matrix
MyRT CreateRT(OMAF::EyePosition::Enum eye)
{
    MyRT res;
    memset(&res, 0, sizeof(res));
    res.mSurface.viewport.x = res.mSurface.viewport.y = 0;
    res.mSurface.viewport.width = 1024;
    res.mSurface.viewport.height = 1024;

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
    if (gOMAFPlay)
    {
        OMAF::RenderTextureDesc colorTextureDesc;
        colorTextureDesc.format = OMAF::RenderTextureFormat::R8G8B8A8;
        colorTextureDesc.width = res.mSurface.viewport.width;
        colorTextureDesc.height = res.mSurface.viewport.height;
        colorTextureDesc.ptr = &res.mTid;

        OMAF::RenderTextureDesc depthTextureDesc;
        depthTextureDesc.format = OMAF::RenderTextureFormat::D32F;
        depthTextureDesc.width = res.mSurface.viewport.width;
        depthTextureDesc.height = res.mSurface.viewport.height;
        depthTextureDesc.ptr = &res.mDid;

        res.mSurface.handle = gOMAFPlay->getRenderer()->createRenderTarget(&colorTextureDesc, &depthTextureDesc);
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

    // Create the window.
    g_hWnd = CreateWindowEx(
        0,
        CLASS_NAME,
        TEXT("OMAFPlayer SDK Example"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 768,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (g_hWnd == NULL)
    {
        return -1;
    }

    return 0;
}

int createOpenGLContext(int nCmdShow, LPRECT rect)
{
    g_hDC = GetDC(g_hWnd);

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
    ShowWindow(g_hWnd, nCmdShow);

    GetClientRect(g_hWnd, rect);
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
    // Setup application Window and OpenGL rendering context
    int initRes = 0;
    initRes = createWindow(hInstance);
    if (initRes != 0)
        return initRes;
    RECT rect;
    initRes = createOpenGLContext(nCmdShow, &rect);
    if (initRes != 0)
        return initRes;

    int windowWidth = rect.right;
    int windowHeight = rect.bottom;

    // Create and initialize OMAF Player Engine
    OMAF::PlatformParameters params;
    params.hWnd = g_hWnd;
    params.hInstance = hInstance;
    params.assetPath = "";
    params.storagePath = "";

    gOMAFPlay = OMAF::IOMAFPlayer::create(OMAF::GraphicsAPI::OPENGL, &params);
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
    }
    else
    {
        //TODO Replace the path and filename with your own mp4 file
        res = gPlaybackControl->loadVideo("storage://video.mp4");
        // or DASH
        //res = gPlaybackControl->loadVideo("http://localhost/video.mpd");
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
        //process Windows messages
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                break;
            }
        }
        else
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
                const OMAF::RenderSurface* surfaces[] = { &leftRT.mSurface,&rightRT.mSurface };

                OMAF::RenderingParameters renderingParameters;
                renderingParameters.displayWaterMark = false;
                gRenderer->renderSurfaces(head, surfaces, 2, renderingParameters);
            }

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
        }
    }

    CoUninitialize();
    return 0;
}
