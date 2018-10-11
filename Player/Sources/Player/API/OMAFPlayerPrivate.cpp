
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
#include "API/OMAFPlayerPrivate.h"

#include "buildinfo.hpp"

#include "Graphics/NVRRenderBackend.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRDeviceInfo.h"

namespace OMAF
{
    static const uint16_t kAudioOutputChannelCount = 2;

    OMAF_LOG_ZONE(OmafPlayerPrivate);

    bool_t validateLicenseID(const char_t* licenseID)
    {
        if (licenseID == NULL)
        {
            return false;
        }

        size_t length = OMAF_NS::StringByteLength(licenseID);

        if (length == 0)
        {
            return false;
        }

        if (length > 256)
        {
            return false;
        }

        return true;
    }

    IOMAFPlayer* IOMAFPlayer::create(GraphicsAPI::Enum graphicsAPI, PlatformParameters* platformParameters)
    {
        if (!Private::DeviceInfo::isCurrentDeviceSupported())
        {
            OMAF_LOG_E("Unsupported device or OS");
            return NULL;
        }
        // TODO: We should organize data types so that we can remove all convertions
        Private::RendererType::Enum rendererType = Private::RendererType::INVALID;

        switch (graphicsAPI)
        {
            case OMAF::GraphicsAPI::OPENGL:      rendererType = Private::RendererType::OPENGL; break;
            case OMAF::GraphicsAPI::OPENGL_ES:   rendererType = Private::RendererType::OPENGL_ES; break;
            case OMAF::GraphicsAPI::VULKAN:      rendererType = Private::RendererType::VULKAN; break;
            case OMAF::GraphicsAPI::METAL:       rendererType = Private::RendererType::METAL; break;
            case OMAF::GraphicsAPI::D3D11:       rendererType = Private::RendererType::D3D11; break;
            case OMAF::GraphicsAPI::D3D12:       rendererType = Private::RendererType::D3D12; break;
            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }

        // Initialize SDK
        Private::Error::Enum initializeResult = Private::initialize(rendererType, platformParameters);

        if (initializeResult != Private::Error::OK)
        {
            OMAF_LOG_E("SDK initialize failed: %d", initializeResult);
            return OMAF_NULL;
        }

        return OMAF_NEW(*Private::MemorySystem::DefaultHeapAllocator(), OmafPlayerPrivate)();
    }

    void IOMAFPlayer::destroy(IOMAFPlayer* aOmafPlayer)
    {
        OMAF_DELETE(*Private::MemorySystem::DefaultHeapAllocator(), aOmafPlayer);

        Private::deinitialize();
    }

    OmafPlayerPrivate::OmafPlayerPrivate()
    : mAllocator(*Private::MemorySystem::DefaultHeapAllocator())
    , mVideoProvider(OMAF_NULL)
    , mRenderingManager(OMAF_NULL)
    , mAudioRenderer(OMAF_NULL)
    , mAudioBackend(OMAF_NULL)
    , mSuspended(false)
    , mElapsedTimeAtSuspend(OMAF_UINT64_MAX)
    {

        uint16_t major = BuildInfo::VersionMajor;
        uint16_t minor = BuildInfo::VersionMinor;
        uint16_t revision = BuildInfo::VersionRevision;
        static char_t version[20];
        sprintf(version, "%d.%d.%d", major, minor, revision);

        Private::MemoryZero(&mAudioState,OMAF_SIZE_OF(mAudioState));
        initialize();
    }

    OmafPlayerPrivate::~OmafPlayerPrivate()
    {
        deinitialize();
    }

    void_t OmafPlayerPrivate::initialize()
    {
        OMAF_ASSERT(mVideoProvider == OMAF_NULL, "Calling initialize incorrectly");
        mVideoProvider = Private::createVideoProvider();
        mRenderingManager = Private::createRenderingManager();
        mAudioRenderer = createAudioRenderer(this);
        mVideoProvider->setAudioInputBuffer(mAudioRenderer->getAudioInputBuffer());
    }

    void_t OmafPlayerPrivate::deinitialize()
    {
        if (mVideoProvider == OMAF_NULL)
        {
            // Already deinitialized so do nothing
            return;
        }
        if (getVideoPlaybackState() != VideoPlaybackState::IDLE)
        {
            stop();
        }

        if (mAudioBackend != OMAF_NULL)
        {
            destroyAudioBackend(mAudioBackend);
        }
        mAudioBackend = OMAF_NULL;

        destroyVideoProvider(mVideoProvider);
        mVideoProvider = OMAF_NULL;

        destroyAudioRenderer(mAudioRenderer);
        mAudioRenderer = OMAF_NULL;

        destroyRenderingManager(mRenderingManager);
        mRenderingManager = OMAF_NULL;

    }

    void_t OmafPlayerPrivate::resume()
    {
        if (mSuspended)
        {
            mSuspended = false;

            initialize();

            if (mAudioState.initialized)
            {
                if (mAudioState.useDirectRouting)
                {
                    initializeAudioWithDirectRouting(mAudioState.allowExclusiveMode);
                }
            }

            if (mElapsedTimeAtSuspend != OMAF_UINT64_MAX)
            {
                loadVideo(mLastLoadedVideo, mElapsedTimeAtSuspend);
            }
        }
    }

    void_t OmafPlayerPrivate::suspend()
    {
        if (!mSuspended)
        {
            VideoPlaybackState::Enum state = getVideoPlaybackState();

            if (state == VideoPlaybackState::PLAYING || state == VideoPlaybackState::BUFFERING)
            {
                pause();
            }
            
            if (state != VideoPlaybackState::IDLE)
            {
                mElapsedTimeAtSuspend = elapsedTime();
                stop();
            }
            else
            {
                mElapsedTimeAtSuspend = OMAF_UINT64_MAX;
            }
            deinitialize();
            mSuspended = true;
        }
    }

    bool_t OmafPlayerPrivate::isFeatureSupported(Feature::Enum feature)
    {
        if (mSuspended)
        {
            return false;
        }

        switch (feature)
        {
        case Feature::HEVC:
            return Private::DeviceInfo::deviceSupportsHEVC();
        case Feature::UHD_PER_EYE:
            return Private::DeviceInfo::deviceSupports2VideoTracks();
        default:
            return false;
        }
    }

    IAudio* OmafPlayerPrivate::getAudio()
    {
        return this;
    }

    IPlaybackControls* OmafPlayerPrivate::getPlaybackControls()
    {
        return this;
    }

    IRenderer* OmafPlayerPrivate::getRenderer()
    {
        return this;
    }

    Version OmafPlayerPrivate::getVersionNumber()
    {
        //TODO: These should come somehow from build automation
        Version version;
        version.major = BuildInfo::VersionMajor;
        version.minor = BuildInfo::VersionMinor;
        version.revision = BuildInfo::VersionRevision;

        //Build a magic hash from the version string so it's not optimized out.
        uint8_t crc = 0;
        size_t i;
        for (i = 0; i < sizeof(BuildInfo::Version); ++i)
        {
            crc ^= BuildInfo::Version[i];
        }
        //Also add the timestamp
        for (i = 0; i < sizeof(BuildInfo::Time); ++i)
        {
            crc ^= BuildInfo::Time[i];
        }
        return version;
    }

    void_t OmafPlayerPrivate::onErrorOccurred(Private::Error::Enum error)
    {
    }

    void_t OmafPlayerPrivate::onRendererReady()
    {
        if (mAudioState.useDirectRouting && mAudioBackend != OMAF_NULL)
        {
            mAudioBackend->onRendererReady();
        }
    }

    void_t OmafPlayerPrivate::onRendererPlaying()
    {
        if (mAudioState.useDirectRouting && mAudioBackend != OMAF_NULL)
        {
            mAudioBackend->onRendererPlaying();
        }
    }

    void_t OmafPlayerPrivate::onFlush()
    {
        if (mAudioState.useDirectRouting && mAudioBackend != OMAF_NULL)
        {
            mAudioBackend->onFlush();
        }
    }

    int64_t OmafPlayerPrivate::onGetPlayedTimeUs()
    {
        if (mAudioState.useDirectRouting && mAudioBackend != OMAF_NULL)
        {
            return mAudioBackend->onGetPlayedTimeUs();
        }
        else
        {
            OMAF_LOG_E("No audio observer");

            return 0;
        }
    }

    void_t OmafPlayerPrivate::onRendererPaused()
    {
        if (mAudioState.useDirectRouting && mAudioBackend != OMAF_NULL)
        {
            mAudioBackend->onRendererPaused();
        }
    }

    Result::Enum OmafPlayerPrivate::initializeAudioWithDirectRouting(bool_t allowExclusiveMode)
    {
        return initializeAudioWithDirectRouting(allowExclusiveMode, NULL);
    }

    Result::Enum OmafPlayerPrivate::initializeAudioWithDirectRouting(bool_t allowExclusiveMode, const wchar_t* audioDevice)
    {
        if (mSuspended)
        {
            return OMAF::Result::INVALID_STATE;
        }

        if (mAudioBackend)
        {
            //already initialized... (resume initializes the audiodevice already.. )
            return OMAF::Result::INVALID_STATE;
        }

        mAudioBackend = Private::createAudioBackend();
        if (mAudioBackend == OMAF_NULL)
        {
            return Result::NOT_SUPPORTED;
        }
        Private::Error::Enum result = mAudioRenderer->init();

        if (result == Private::Error::OK)
        {
            mAudioBackend->init(mAudioRenderer, allowExclusiveMode, audioDevice);
            mAudioState.useDirectRouting = true;
            mAudioState.initialized = true;
        }
        
        return convertResult(result);
    }

    Result::Enum OmafPlayerPrivate::resetAudio()
    {
        if (mSuspended)
        {
            return OMAF::Result::INVALID_STATE;
        }

        if (mAudioBackend != OMAF_NULL)
        {
            destroyAudioBackend(mAudioBackend);
            mAudioBackend = OMAF_NULL;
            mAudioState.useDirectRouting = false;
        }

        mAudioState.observer = OMAF_NULL;
        mAudioState.initialized = false;

        return convertResult(mAudioRenderer->reset());
    }

    Result::Enum OmafPlayerPrivate::setGain(float gain)
    {
        return Result::DEPRECATED;
    }

    Result::Enum OmafPlayerPrivate::setAudioLatency(int64_t latencyUs)
    {
        return OMAF::Result::NOT_SUPPORTED;
    }

    AudioReturnValue::Enum OmafPlayerPrivate::renderSamples(size_t bufferSize, float* samples, size_t& samplesRendered)
    {
        return AudioReturnValue::INVALID;
    }

    AudioReturnValue::Enum OmafPlayerPrivate::renderSamples(size_t bufferSize, int16_t* samples, size_t& samplesRendered)
    {
        return AudioReturnValue::INVALID;
    }

    void_t OmafPlayerPrivate::firstSamplesConsumed()
    {
        return;
    }

    void_t OmafPlayerPrivate::setHeadTransform(const HeadTransform &headTransform)
    {
        return;
    }

    uint32_t OmafPlayerPrivate::createRenderTarget(const RenderTextureDesc* colorAttachment, const RenderTextureDesc* depthStencilAttachment)
    {
        if (mSuspended)
        {
            return 0;
        }
        return mRenderingManager->createRenderTarget(colorAttachment, depthStencilAttachment);
    }

    void OmafPlayerPrivate::destroyRenderTarget(uint32_t handle)
    {
        if (mSuspended)
        {
            return;
        }
        mRenderingManager->destroyRenderTarget(handle);
    }

    void OmafPlayerPrivate::renderSurfaces(const HeadTransform& aHeadTransform, const RenderSurface** aRenderSurfaces, uint8_t aNumRenderSurfaces, const bool aDisplayWaterMark)
    {
        RenderingParameters params;
        params.displayWaterMark = aDisplayWaterMark;
        renderSurfaces(aHeadTransform, aRenderSurfaces, aNumRenderSurfaces, params);
    }

    void OmafPlayerPrivate::renderSurfaces(const HeadTransform& aHeadTransform, const RenderSurface** aRenderSurfaces, uint8_t aNumRenderSurfaces, const RenderingParameters& aRenderingParameters)
    {
        if (mSuspended)
        {
            return;
        }

        Private::RenderBackend::activate();
        
        mRenderingManager->prepareRender(aHeadTransform, aRenderingParameters);
        
        for (uint8_t s = 0; s < aNumRenderSurfaces; ++s)
        {
            const RenderSurface& surface = *aRenderSurfaces[s];
            
            mRenderingManager->render(aHeadTransform, surface, aRenderingParameters);
        }
        
        mRenderingManager->finishRender();
        
        Private::RenderBackend::deactivate();
    }

    void_t OmafPlayerPrivate::setVideoPlaybackObserver(VideoPlaybackObserver* observer)
    {
    }

    VideoPlaybackState::Enum OmafPlayerPrivate::getVideoPlaybackState()
    {
        if (mSuspended)
        {
            return VideoPlaybackState::INVALID;
        }
        return convertVideoPlaybackState(mVideoProvider->getState());
    }

    Result::Enum OmafPlayerPrivate::loadVideo(const char *uri, uint64_t initialPositionMS)
    {
        if (!mAudioState.initialized || mVideoProvider == OMAF_NULL)
        {
            return OMAF::Result::INVALID_STATE;
        }
        else
        {
            Private::PathName uriPath = Private::PathName(uri);
            Private::Error::Enum result = mVideoProvider->loadSource(uriPath, initialPositionMS);

            if (result == Private::Error::OK)
            {
                mLastLoadedVideo = uriPath;
                mRenderingManager->initialize(mVideoProvider);
            }

            return convertResult(result);
        }
    }

    Result::Enum OmafPlayerPrivate::play()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        Private::Error::Enum result = mVideoProvider->start();
        return convertResult(result);
    }

    Result::Enum OmafPlayerPrivate::pause()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        Private::Error::Enum result = mVideoProvider->pause();
        return convertResult(result);
    }

    Result::Enum OmafPlayerPrivate::stop()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        OMAF_NS::VideoProviderState::Enum state = mVideoProvider->getState();
        mRenderingManager->deinitialize();
        return convertResult(mVideoProvider->stop());
    }


    Result::Enum OmafPlayerPrivate::next()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }
        return convertResult(mVideoProvider->next());
    }

    Result::Enum OmafPlayerPrivate::loadAuxiliaryVideo(const char *uri)
    {
        if (!mAudioState.initialized || mVideoProvider == OMAF_NULL)
        {
            return OMAF::Result::INVALID_STATE;
        }
        else
        {
            Private::PathName uriPath = Private::PathName(uri);
            Private::Error::Enum result = mVideoProvider->loadAuxiliaryStream(uriPath);
            return convertResult(result);
        }
    }

    Result::Enum OmafPlayerPrivate::playAuxiliary()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        Private::Error::Enum result = mVideoProvider->playAuxiliary();
        return convertResult(result);
    }

    Result::Enum OmafPlayerPrivate::pauseAuxiliary()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        Private::Error::Enum result = mVideoProvider->pauseAuxiliary();
        return convertResult(result);
    }

    Result::Enum OmafPlayerPrivate::stopAuxiliary()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }
        return convertResult(mVideoProvider->stopAuxiliary());
    }

    Result::Enum  OmafPlayerPrivate::seekToAuxiliary(uint64_t aMilliSeconds)
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }
        return convertResult(mVideoProvider->seekToMsAuxiliary(aMilliSeconds));
    }

    VideoPlaybackState::Enum OmafPlayerPrivate::getAuxiliaryVideoPlaybackState()
    {
        if (mSuspended)
        {
            return VideoPlaybackState::INVALID;
        }
        return convertVideoPlaybackState(mVideoProvider->getAuxiliaryState());
    }

    uint64_t OmafPlayerPrivate::elapsedTimeAuxiliary()
    {
        if (mSuspended)
        {
            return 0;
        }
        return mVideoProvider->elapsedTimeMsAuxiliary();
    }

    uint64_t OmafPlayerPrivate::durationAuxiliary()
    {
        if (mSuspended)
        {
            return 0;
        }
        return mVideoProvider->durationMsAuxiliary();
    }

    bool_t OmafPlayerPrivate::isSeekable()
    {
        if (mSuspended)
        {
            return false;
        }

        return mVideoProvider->isSeekable();
    }

    Result::Enum OmafPlayerPrivate::seekTo(uint64_t milliSeconds, OMAF::SeekAccuracy::Enum accuracy)
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        Result::Enum result = convertResult(mVideoProvider->seekToMs(milliSeconds, convertSeekAccuracy(accuracy)));
        return result;
    }

    uint64_t OmafPlayerPrivate::elapsedTime()
    {
        if (mSuspended)
        {
            return 0;
        }
        return mVideoProvider->elapsedTimeMs();
    }

    uint64_t OmafPlayerPrivate::duration()
    {
        if (mSuspended)
        {
            return 0;
        }
        return mVideoProvider->durationMs();
    }

    MediaInformation OmafPlayerPrivate::getMediaInformation()
    {
        if (mSuspended)
        {
            return MediaInformation();
        }
        return convertMediaInformation(mVideoProvider->getMediaInformation());
    }

    void_t OmafPlayerPrivate::setAudioVolume(float aVolume)
    {
        if (mAudioRenderer != OMAF_NULL)
        {
            mAudioRenderer->setAudioVolume(aVolume);
        }
    }

    void_t OmafPlayerPrivate::setAudioVolumeAuxiliary(float aVolume)
    {
        if (mAudioRenderer != OMAF_NULL)
        {
            mAudioRenderer->setAudioVolumeAuxiliary(aVolume);
        }
    }

}
