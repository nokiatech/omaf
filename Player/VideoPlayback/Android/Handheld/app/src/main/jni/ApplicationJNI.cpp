
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
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include <memory>
#include <GLES3/gl3.h>
#include <EGL/egl.h>

#include <android/native_window.h> // requires ndk r5 or newer
#include <android/native_window_jni.h> // requires ndk r5 or newer

#include <jni.h>
#include <android/log.h>
#include <android/asset_manager_jni.h>

#include <API/OMAFPlayer.h>
#include <Platform/OMAFDataTypes.h>
#include <API/OMAFPlayerPlatformParameters.h>
#include <Math/OMAFQuaternion.h>
#include <Math/OMAFMatrix44.h>


// In Android SensorManager frame, Z axis points toward the sky. Instead we'd like it to point behind
// So, we need to adjust by rotating the quaternion 90 degrees by the X axis
static const OMAF::Quaternion SENSOR_CALIBRATION = OMAF::makeQuaternion(OMAF::makeVector3(1.0f, 0.0f, 0.0f), -OMAF_PI_DIV_2);

// Also, the rotation vector assumes natural orientation. So, because we are running the application
// in landscape mode, depending on the device type we may need to rotate the picture by 90 degrees
// along the Z axis
static const OMAF::Quaternion DISPLAY_ROTATION = OMAF::makeQuaternion(OMAF::makeVector3(0.0f, 0.0f, 1.0f), -OMAF_PI_DIV_2);

#if DEBUG

    #define LOG_TAG "OMAFPlayerWrapper"

    #define LOG_E(...) __android_log_print( ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__ )
    #define LOG_V(...) __android_log_print( ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__ )

#else

    #define LOG_E(...)
    #define LOG_V(...)

#endif

static const float nearZ = 0.01f;
static const float farZ = 100.0f;

static JavaVM* mJavaVM = NULL;
static jobject mActivityRef = NULL;
static jobject mAssetManagerRef = NULL;
static AAssetManager* mAssetManager = NULL;
static char* mExternalStoragePath = NULL;
static char* mCachePath = NULL;

OMAF::IOMAFPlayer* mOmafPlayer = NULL;
OMAF::IPlaybackControls* mPlaybackControl = NULL;
OMAF::IRenderer* mRenderer = NULL;
OMAF::IAudio* mAudio = NULL;

uint32_t mWindowWidth;
uint32_t mWindowHeight;

OMAF::Quaternion mHeadTransform;

struct RenderSurfaceGL
        : public OMAF::RenderSurface
{
    RenderSurfaceGL()
            : fboHandle(0)
            , depthStencilHandle(0)
            , textureHandle(0)
    {
    }

    GLuint fboHandle;
    GLuint depthStencilHandle;
    GLuint textureHandle;
};

RenderSurfaceGL* mRenderSurfaces[1];

void initialize(JNIEnv* env, JavaVM* javaVM, jobject activity, AAssetManager* assetManager,
                const char* externalStoragePath, const char* cachePath);
void deinitialize();

void play(const char* uri);
void stop();
void pause();
void resume();

void render();
void submitFrame();

void createRenderSurface(RenderSurfaceGL* surface, OMAF::EyePosition::Enum eye, uint32_t width, uint32_t height, float nearZ, float farZ);
void destroyRenderSurface(RenderSurfaceGL* surface);

OMAF::HeadTransform getHeadTransform();
const OMAF::RenderSurface** getRenderSurfaces();

static const char* FILE_URI = "asset://omaf_360.heic";
//static const char* FILE_URI = "asset://forest_omaf.mp4";

#define JNI_METHOD(return_type, method_name) JNIEXPORT return_type JNICALL Java_com_nokia_omaf_sample_videoplayback_handheld_MainActivity_##method_name

static jlong jptr(void* application)
{
    return (intptr_t)application;
}

extern "C"
{
    jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
    {
        mJavaVM = vm;

        JNIEnv* env;

        if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK)
        {
            return -1;
        }

        return JNI_VERSION_1_6;
    }

    void JNI_OnUnload(JavaVM* vm, void* /*reserved*/)
    {
        mJavaVM = NULL;
    }

    JNI_METHOD(jlong, nativeCreateApplication)(JNIEnv* env,
                                               jobject obj,
                                               jobject assetManager,
                                               jobject mainActivity,
                                               jstring externalStoragePath,
                                               jstring cachePath,
                                               jint width,
                                               jint height)
    {
        mActivityRef  = env->NewGlobalRef(mainActivity);
        mAssetManagerRef = env->NewGlobalRef(assetManager);
        mAssetManager = AAssetManager_fromJava(env, mAssetManagerRef);

        const char* nativeStorageString = env->GetStringUTFChars(externalStoragePath, 0);
        mExternalStoragePath = strdup(nativeStorageString);
        env->ReleaseStringUTFChars(externalStoragePath, nativeStorageString);

        const char* nativeCachePath = env->GetStringUTFChars(cachePath, 0);
        mCachePath = strdup(nativeCachePath);
        env->ReleaseStringUTFChars(cachePath, nativeCachePath);

        mWindowWidth = width;
        mWindowHeight = height;
        mHeadTransform = OMAF::QuaternionIdentity;

        initialize(env, mJavaVM, mActivityRef, mAssetManager, mExternalStoragePath, mCachePath);
        mRenderSurfaces[0] = new RenderSurfaceGL();
        createRenderSurface(mRenderSurfaces[0], OMAF::EyePosition::LEFT, mWindowWidth, mWindowHeight, nearZ, farZ);
        play(FILE_URI);
        return jptr(NULL);
    }

    JNI_METHOD(void, nativeDestroyApplication) (JNIEnv* env, jobject obj, jlong /*application*/)
    {
        deinitialize();
        destroyRenderSurface(mRenderSurfaces[0]);
        mRenderSurfaces[0] = NULL;
        env->DeleteGlobalRef(mAssetManagerRef);
        env->DeleteGlobalRef(mActivityRef);
        free(mExternalStoragePath);
        free(mCachePath);
    }

    JNI_METHOD(void, nativeUpdateRotation)(JNIEnv* env, jobject obj, jfloat x, jfloat y, jfloat z, jfloat w)
    {
        mHeadTransform = OMAF::makeQuaternion(x,y,z,w);
        mHeadTransform = concatenate(SENSOR_CALIBRATION, mHeadTransform);
        mHeadTransform = concatenate(mHeadTransform, DISPLAY_ROTATION);
        OMAF::normalize(mHeadTransform);
    }

    JNI_METHOD(void, nativeDraw)(JNIEnv* env, jobject obj, jlong /*application*/)
    {
        render();
    }

    JNI_METHOD(void, nativeOnPause)(JNIEnv* env, jobject obj, jlong /*application*/)
    {
        pause();
    }

    JNI_METHOD(void, nativeOnResume)(JNIEnv* env, jobject obj, jlong /*application*/)
    {
        resume();
    }

    JNI_METHOD(void, nativeOnTapEvent)(JNIEnv* env, jobject obj)
    {
        if (mPlaybackControl) {
            //Try next, if we have a HEIC image. If not, pause/resume video.
            if(mPlaybackControl->next()==OMAF::Result::NOT_SUPPORTED) {
                if (mPlaybackControl->getVideoPlaybackState() == OMAF::VideoPlaybackState::PAUSED)
                    resume();
                else if (mPlaybackControl->getVideoPlaybackState() == OMAF::VideoPlaybackState::PLAYING)
                    pause();
            }
        }
    }
}

void initialize(JNIEnv* env, JavaVM* javaVM, jobject activity, AAssetManager* assetManager, const char* externalStoragePath, const char* cachePath)
{
    assert(mOmafPlayer == NULL);
    assert(mPlaybackControl == NULL);
    assert(mAudio == NULL);
    assert(mRenderer == NULL);

    // Initializing the SDK for Android requires platform specific information
    OMAF::PlatformParameters platformParameters;
    platformParameters.javaVM = mJavaVM;
    platformParameters.activity = mActivityRef;
    platformParameters.assetManager = mAssetManager;
    platformParameters.externalStoragePath = externalStoragePath;
    platformParameters.cachePath = cachePath;

    // Create the Player instance. Since this is Android, we're using OpenGL ES
    mOmafPlayer = OMAF::IOMAFPlayer::create(OMAF::GraphicsAPI::OPENGL_ES, &platformParameters);

    // After creating the player instance, you can access the playback controls, audio and renderer
    mPlaybackControl = mOmafPlayer->getPlaybackControls();
    mAudio = mOmafPlayer->getAudio();
    mRenderer = mOmafPlayer->getRenderer();
}

void deinitialize()
{
    if (mPlaybackControl)
    {
        mPlaybackControl->stop();
        mPlaybackControl = NULL;
    }

    if (mOmafPlayer)
    {
        // Destroy the OMAFPlayer instance by calling the destroy method.
        // Do not call delete directly on the instance or any of the interfaces.
        OMAF::IOMAFPlayer::destroy(mOmafPlayer);

        mOmafPlayer = NULL;
        mPlaybackControl = NULL;
        mAudio = NULL;
        mRenderer = NULL;
    }
}

void play(const char* uri)
{
    OMAF::Result::Enum result;

    // Initializing the audio using direct routing
    result = mAudio->initializeAudioWithDirectRouting();

    // Setting audio gain for the video
    result = mAudio->setGain(0.2f);

    // Load the video and start playback
    if (mPlaybackControl->loadVideo(uri) == OMAF::Result::OK)
    {
        if (mPlaybackControl->play() == OMAF::Result::OK)
        {
            LOG_V("Video playback started");
        }
        else
        {
            LOG_E("Video playback failed to start");
        }
    }
    else
    {
        LOG_E("Video playback failed to load video URI");
    }
}

void stop()
{
    if (mPlaybackControl)
    {
        mPlaybackControl->stop();
        mPlaybackControl = NULL;
    }
}

void pause()
{
    if (mPlaybackControl)
    {
        mPlaybackControl->pause();
    }
}

void resume()
{
    if (mPlaybackControl)
    {
        mPlaybackControl->play();
    }
}

void render()
{
    // Clear the buffers and Render frame with the OMAF player
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    mRenderer->renderSurfaces(getHeadTransform(), getRenderSurfaces(), 1);
    submitFrame();
}

OMAF::HeadTransform getHeadTransform()
{
    OMAF::HeadTransform transform;
    transform.orientation = mHeadTransform;
    transform.source = OMAF::HeadTransformSource::SENSOR;
    return transform;
}

const OMAF::RenderSurface** getRenderSurfaces()
{
    // Cast our rendersurface pointers to OMAF native.
    return (const OMAF::RenderSurface**)mRenderSurfaces;
}

void submitFrame()
{
    // Render the textures to window from the framebuffer
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, mRenderSurfaces[0]->fboHandle);
    glBlitFramebuffer(mRenderSurfaces[0]->viewport.x,
                      mRenderSurfaces[0]->viewport.y,
                      mRenderSurfaces[0]->viewport.width,
                      mRenderSurfaces[0]->viewport.height,
                      0,
                      0,
                      mWindowWidth,
                      mWindowHeight,
                      GL_COLOR_BUFFER_BIT,
                      GL_LINEAR);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glFinish();

}

void createRenderSurface(RenderSurfaceGL* surface, OMAF::EyePosition::Enum eye, uint32_t width, uint32_t height, float nearZ, float farZ)
{

    // Create frame buffer
    GLuint color = 0;
    GLuint depthStencil = 0;
    GLuint fbo = 0;

    glGenTextures(1, &color);
    glBindTexture(GL_TEXTURE_2D, color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glBindTexture(GL_TEXTURE_2D, 0);

    glGenRenderbuffers(1, &depthStencil);
    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    surface->fboHandle = fbo;
    surface->textureHandle = color;
    surface->depthStencilHandle = depthStencil;

    // Create render surface
    OMAF::RenderTextureDesc colorTextureDesc;
    colorTextureDesc.format = OMAF::RenderTextureFormat::R8G8B8A8;
    colorTextureDesc.width = width;
    colorTextureDesc.height = height;
    colorTextureDesc.ptr = &color;
    if (mRenderer != OMAF_NULL)
    {
        surface->handle = mRenderer->createRenderTarget(&colorTextureDesc, NULL);
    }

    surface->viewport.x = 0;
    surface->viewport.y = 0;
    surface->viewport.width = width;
    surface->viewport.height = height;
    surface->eyePosition = eye;

    const float aspectRatio = (float)width / (float)height;
    surface->eyeTransform = OMAF::makeTranslation(0.0f, 0.0f, 0.0f); // No need when single eye.
    surface->projection = OMAF::makePerspectiveRH(OMAF::toRadians(90.0f), aspectRatio, nearZ, farZ); // 90 degrees vertical field of view
}

void destroyRenderSurface(RenderSurfaceGL* surface)
{
    if (mRenderer != OMAF_NULL)
    {
        mRenderer->destroyRenderTarget(surface->handle);
    }

    surface->handle = 0;

    if (surface->fboHandle != 0)
    {
        glDeleteFramebuffers(1, &surface->fboHandle);
        surface->fboHandle = 0;
    }

    if (surface->depthStencilHandle != 0)
    {
        glDeleteRenderbuffers(1, &surface->depthStencilHandle);
        surface->depthStencilHandle = 0;
    }

    if (surface->textureHandle != 0)
    {
        glDeleteTextures(1, &surface->textureHandle);
        surface->textureHandle = 0;
    }
}