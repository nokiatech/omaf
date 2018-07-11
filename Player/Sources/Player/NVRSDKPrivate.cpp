
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
#include "NVREssentials.h"
#include "Foundation/NVRLogger.h"
#include "Graphics/NVRRenderBackend.h"
#include "Foundation/NVRDeviceInfo.h"

#include "NVRPlayer.h"
#include "buildinfo.hpp"

#include "Provider/NVRVideoProvider.h"
#include "Renderer/NVRRenderingManager.h"
#include "Audio/NVRNullAudioRenderer.h"
#include "Audio/NVRNullAudioBackend.h"
#include "Foundation/NVRHttp.h"

#include "Foundation/NVRBandwidthMonitor.h"
#include "API/OMAFPlayerPlatformParameters.h"

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
void InitializeLibDash();
void DeinitializeLibDash();
#endif

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

OMAF_NS_BEGIN
//initialize decoders
void SetupDecoder();
void CleanupDecoder();
OMAF_NS_END

#if OMAF_PLATFORM_UWP

#include <roapi.h>

#endif

//TODO: Fix this, we should not need to extern define these parameters...
OMAF_NS_BEGIN
    namespace StorageManager
    {
        extern PathName gStoragePath;
    }

    namespace AssetManager
    {
        extern PathName gAssetPath;
    }
OMAF_NS_END

#endif

#if OMAF_PLATFORM_ANDROID
#include "Foundation/Android/NVRAndroid.h"
#endif


OMAF_NS_BEGIN
    OMAF_LOG_ZONE(SDKPrivate)

#if OMAF_ENABLE_FILE_LOGGER

    static const char_t* FILE_LOGGER_FILENAME = "nvr.log";
    static FileLogger sFileLogger(FILE_LOGGER_FILENAME);

#endif

    void_t logSDKInfo();
    void_t logDeviceInfo();
    void_t logGraphicsDeviceInfo();

    namespace SDKState
    {
        enum Enum
        {
            UNDEFINED = -1,

            UNINITIALIZED,
            INITIALIZED,

            COUNT
        };
    }

    static SDKState::Enum sState = SDKState::UNINITIALIZED;

#if OMAF_PLATFORM_ANDROID

    static bool_t javaActivityEnabled = false;

#endif

    Error::Enum initialize(RendererType::Enum rendererType, OMAF::PlatformParameters* platformParameters, MemoryAllocator* clientHeapAllocator)
    {

#if OMAF_PLATFORM_UWP
        Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
#endif
#if OMAF_PLATFORM_ANDROID

        if (platformParameters == NULL)
        {
            return Error::OPERATION_FAILED;
        }

        javaActivityEnabled = (platformParameters->nativeActivity == NULL);

        Private::Android::initialize(platformParameters);

        if (!javaActivityEnabled)
        {
            Private::Android::attachThread();
        }

#endif

        if (sState != SDKState::UNINITIALIZED)
        {
            return Error::INVALID_STATE;
        }

        LogDispatcher::initialize(
#if OMAF_DEBUG_BUILD
            LogDispatcher::LogLevel::Verbose |
            LogDispatcher::LogLevel::Debug | 
#endif
            LogDispatcher::LogLevel::Info |
            LogDispatcher::LogLevel::Warning |
            LogDispatcher::LogLevel::Error
        );

#if OMAF_ENABLE_FILE_LOGGER

        LogDispatcher::mount(sFileLogger);

#endif

        MemorySystem::Create(clientHeapAllocator, MemorySystem::DEFAULT_SCRATCH_ALLOCATOR_SIZE);
        DeviceInfo::initialize();

        FixedString1024 defaultHttpUserAgent;

        defaultHttpUserAgent.appendFormat("OMAFPlayerSDK/%s (%s; %s)", BuildInfo::Version, DeviceInfo::getOSName().getData(), DeviceInfo::getDeviceModel().getData());

        Http::setDefaultUserAgent(defaultHttpUserAgent.getData());

        // Initialize render backend
        RenderBackend::Parameters renderBackendParameters;
        MemoryZero(&renderBackendParameters, OMAF_SIZE_OF(renderBackendParameters));

        // TODO: This shouldn't be here?
#if OMAF_PLATFORM_WINDOWS
        renderBackendParameters.hWnd = platformParameters->hWnd;

        StorageManager::gStoragePath = platformParameters->storagePath;
        AssetManager::gAssetPath = platformParameters->assetPath;
#endif

#if OMAF_PLATFORM_UWP
        StorageManager::gStoragePath = platformParameters->storagePath;
        AssetManager::gAssetPath = platformParameters->assetPath;
#endif

#if OMAF_GRAPHICS_API_D3D11
        renderBackendParameters.d3dDevice = platformParameters->d3dDevice;
#endif

        if (!RenderBackend::create(rendererType, renderBackendParameters))
        {
            OMAF_LOG_E("RenderBackend creation failed");
            return Error::NOT_INITIALIZED;
        }

        logSDKInfo();
        logDeviceInfo();
        logGraphicsDeviceInfo();

        sState = SDKState::INITIALIZED;

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
        InitializeLibDash();
#endif

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        SetupDecoder();        
#endif
        // Create the Bandwidth monitor here to prevent threading issues further down the road
        Private::BandwidthMonitor::getInstance();

        return Error::OK;
    }

    void_t deinitialize()
    {
        Private::BandwidthMonitor::destroyInstance();

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        CleanupDecoder();
#endif

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
        DeinitializeLibDash();
#endif

        RenderBackend::destroy();

        DeviceInfo::shutdown();
        MemorySystem::Destroy();

#if OMAF_ENABLE_FILE_LOGGER

        LogDispatcher::unmount(sFileLogger);

#endif

        LogDispatcher::shutdown();

#if OMAF_PLATFORM_ANDROID

        if (!javaActivityEnabled)
        {
            Private::Android::detachThread();

            javaActivityEnabled = false;
        }

#endif
#if OMAF_PLATFORM_UWP
        Windows::Foundation::Uninitialize();
#endif

        sState = SDKState::UNINITIALIZED;
    }

    RenderingManager* createRenderingManager()
    {
        if (sState == SDKState::INITIALIZED)
        {
            MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();
            RenderingManager* instance = OMAF_NEW(allocator, RenderingManager)();

            return instance;
        }
        else
        {
            OMAF_LOG_E("Can't create RenderingManager because of invalid state");

            return OMAF_NULL;
        }
    }

    void_t destroyRenderingManager(RenderingManager* presenceRenderer)
    {
        OMAF_ASSERT_NOT_NULL(presenceRenderer);

        if (presenceRenderer == OMAF_NULL)
        {
            return;
        }

        MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

        OMAF_DELETE(allocator, presenceRenderer);
    }

    AudioRendererAPI* createAudioRenderer(AudioRendererObserver* observer)
    {
        OMAF_ASSERT_NOT_NULL(observer);

        if (observer == OMAF_NULL)
        {
            return OMAF_NULL;
        }

        if (sState == SDKState::INITIALIZED)
        {
            MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();
            AudioRendererAPI* instance = OMAF_NEW(allocator, NullAudioRenderer)(allocator, *observer);

            return instance;
        }
        else
        {
            OMAF_LOG_E("Can't create AudioRenderer because of invalid state");

            return OMAF_NULL;
        }
    }

    void_t destroyAudioRenderer(AudioRendererAPI* audioRenderer)
    {
        OMAF_ASSERT_NOT_NULL(audioRenderer);

        if (audioRenderer == OMAF_NULL)
        {
            return;
        }

        MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

        OMAF_DELETE(allocator, audioRenderer);
    }

    VideoProvider* createVideoProvider()
    {
        if (sState == SDKState::INITIALIZED)
        {
            MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();
            VideoProvider* instance = OMAF_NEW(allocator, VideoProvider)();

            return instance;
        }
        else
        {
            OMAF_LOG_E("Can't create VideoProvider because of invalid state");

            return OMAF_NULL;
        }
    }

    void_t destroyVideoProvider(VideoProvider* localVideoProvider)
    {
        OMAF_ASSERT_NOT_NULL(localVideoProvider);

        if (localVideoProvider == OMAF_NULL)
        {
            return;
        }

        MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

        OMAF_DELETE(allocator, localVideoProvider);
    }

    AudioBackend* createAudioBackend()
    {
        if (sState == SDKState::INITIALIZED)
        {
            MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();
            return OMAF_NEW(allocator, NullBackend)();
        }
        else
        {
            OMAF_LOG_E("Can't create AudioBackend because of invalid state");

            return OMAF_NULL;
        }
    }

    void_t destroyAudioBackend(AudioBackend* router)
    {
        OMAF_ASSERT_NOT_NULL(router);

        if (router == OMAF_NULL)
        {
            return;
        }

        MemoryAllocator& allocator = *MemorySystem::DefaultHeapAllocator();

        OMAF_DELETE(allocator, router);
    }

    void_t logDeviceInfo()
    {
        OMAF_LOG_I("-------------------- Device Capabilities -------------------------------------");
        OMAF_LOG_I("Device model = %s", DeviceInfo::getDeviceModel().getData());
        OMAF_LOG_I("Device platform = %s", DeviceInfo::getDevicePlatformId().getData());
        OMAF_LOG_I("Device platform info = %s", DeviceInfo::getDevicePlatformInfo().getData());
        OMAF_LOG_I("Device supports 2 video tracks = %s", DeviceInfo::deviceSupports2VideoTracks() ? "true" : "false");
        OMAF_LOG_I("------------------------------------------------------------------------------");
    }

    void_t logSDKInfo()
    {
        OMAF_LOG_I("-------------------- SDK Info ------------------------------------------------");
        OMAF_LOG_I("SDK version = %s", BuildInfo::Version);
        OMAF_LOG_I("Build time = %s", BuildInfo::Time);
        OMAF_LOG_I("------------------------------------------------------------------------------");
    }

    void_t logGraphicsDeviceInfo()
    {
        const RenderBackend::Capabilities& capabilities = RenderBackend::getCapabilities();

        OMAF_LOG_I("-------------------- Render Backend Capabilities ------------------------------------------");
        OMAF_LOG_I("Renderer type = %s", RendererType::toString(capabilities.rendererType));
        OMAF_LOG_I("");
        OMAF_LOG_I("API version = %s", capabilities.apiVersionStr.getData());
        OMAF_LOG_I("Shader language version = %s", capabilities.apiShaderVersionStr.getData());
        OMAF_LOG_I("Vendor = %s", capabilities.vendorStr.getData());
        OMAF_LOG_I("Device = %s", capabilities.deviceStr.getData());
        OMAF_LOG_I("Memory info = %s", capabilities.memoryInfo.getData());
        OMAF_LOG_I("");
        OMAF_LOG_I("Num texture units = %d", capabilities.numTextureUnits);
        OMAF_LOG_I("Max 2D texture size = %d", capabilities.maxTexture2DSize);
        OMAF_LOG_I("Max 3D texture size = %d", capabilities.maxTexture3DSize);
        OMAF_LOG_I("Max cube texture size = %d", capabilities.maxTextureCubeSize);
        OMAF_LOG_I("Max render target texture attachment = %d", capabilities.maxRenderTargetAttachments);
        OMAF_LOG_I("");
        OMAF_LOG_I("Instancing support = %s", capabilities.instancedSupport ? "yes" : "no");
        OMAF_LOG_I("Compute support = %s", capabilities.computeSupport ? "yes" : "no");
        OMAF_LOG_I("");
        OMAF_LOG_I("Supported texture formats =");

        for (size_t f = 0; f < TextureFormat::COUNT; ++f)
        {
            TextureFormat::Enum textureFormat = (TextureFormat::Enum)f;

            if (capabilities.textureFormats[f])
            {
                if (TextureFormat::isCompressed(textureFormat))
                {
                    OMAF_LOG_I("\t\tx %s (compressed)", TextureFormat::toString(textureFormat));
                }
                else
                {
                    OMAF_LOG_I("\t\tx %s", TextureFormat::toString(textureFormat));
                }
            }
            else
            {
                if (TextureFormat::isCompressed(textureFormat))
                {
                    OMAF_LOG_I("\t\t  %s (compressed)", TextureFormat::toString(textureFormat));
                }
                else
                {
                    OMAF_LOG_I("\t\t  %s", TextureFormat::toString(textureFormat));
                }
            }
        }

        OMAF_LOG_I("------------------------------------------------------------------------------");
    }
OMAF_NS_END
