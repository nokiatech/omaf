
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
#include "API/OMAFPlayerPrivate.h"

#include "buildinfo.hpp"

#include "Foundation/NVRArray.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRLogger.h"

#include "Graphics/NVRRenderBackend.h"


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
        Private::RendererType::Enum rendererType = Private::RendererType::INVALID;

        switch (graphicsAPI)
        {
        case OMAF::GraphicsAPI::OPENGL:
            rendererType = Private::RendererType::OPENGL;
            break;
        case OMAF::GraphicsAPI::OPENGL_ES:
            rendererType = Private::RendererType::OPENGL_ES;
            break;
        case OMAF::GraphicsAPI::VULKAN:
            rendererType = Private::RendererType::VULKAN;
            break;
        case OMAF::GraphicsAPI::METAL:
            rendererType = Private::RendererType::METAL;
            break;
        case OMAF::GraphicsAPI::D3D11:
            rendererType = Private::RendererType::D3D11;
            break;
        case OMAF::GraphicsAPI::D3D12:
            rendererType = Private::RendererType::D3D12;
            break;
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
        , mUserActions(mAllocator)
    {
        uint16_t major = BuildInfo::VersionMajor;
        uint16_t minor = BuildInfo::VersionMinor;
        uint16_t revision = BuildInfo::VersionRevision;
        static char_t version[20];
        sprintf(version, "%d.%d.%d", major, minor, revision);

        Private::MemoryZero(&mAudioState, OMAF_SIZE_OF(mAudioState));
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

    const Quaternion OmafPlayerPrivate::viewingOrientationOffset() const
    {
        return mVideoProvider->viewingOrientationOffset();
    }

    Version OmafPlayerPrivate::getVersionNumber()
    {
        Version version;
        version.major = BuildInfo::VersionMajor;
        version.minor = BuildInfo::VersionMinor;
        version.revision = BuildInfo::VersionRevision;

        // Build a magic hash from the version string so it's not optimized out.
        uint8_t crc = 0;
        size_t i;
        for (i = 0; i < sizeof(BuildInfo::Version); ++i)
        {
            crc ^= BuildInfo::Version[i];
        }
        // Also add the timestamp
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

    Result::Enum OmafPlayerPrivate::initializeAudioWithDirectRouting(bool_t allowExclusiveMode,
                                                                     const wchar_t* audioDevice)
    {
        if (mSuspended)
        {
            return OMAF::Result::INVALID_STATE;
        }

        if (mAudioBackend)
        {
            // already initialized... (resume initializes the audiodevice already.. )
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

    AudioReturnValue::Enum OmafPlayerPrivate::renderSamples(size_t bufferSize,
                                                            int16_t* samples,
                                                            size_t& samplesRendered)
    {
        return AudioReturnValue::INVALID;
    }

    void_t OmafPlayerPrivate::firstSamplesConsumed()
    {
        return;
    }

    void_t OmafPlayerPrivate::setHeadTransform(const HeadTransform& headTransform)
    {
        return;
    }

    uint32_t OmafPlayerPrivate::createRenderTarget(const RenderTextureDesc* colorAttachment,
                                                   const RenderTextureDesc* depthStencilAttachment)
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

    void OmafPlayerPrivate::renderSurfaces(const HeadTransform& aHeadTransform,
                                           const RenderSurface** aRenderSurfaces,
                                           uint8_t aNumRenderSurfaces,
                                           const bool aDisplayWaterMark)
    {
        RenderingParameters params;
        params.displayWaterMark = aDisplayWaterMark;
        renderSurfaces(aHeadTransform, aRenderSurfaces, aNumRenderSurfaces, params);
    }

    void OmafPlayerPrivate::renderSurfaces(const HeadTransform& aHeadTransform,
                                           const RenderSurface** aRenderSurfaces,
                                           uint8_t aNumRenderSurfaces,
                                           const RenderingParameters& aRenderingParameters)
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

    OMAF::OverlayState OmafPlayerPrivate::getOverlayState(uint32_t ovlyId) const
    {
        return mVideoProvider->overlayState(ovlyId);
    }

    Result::Enum OmafPlayerPrivate::loadVideo(const char* uri, uint64_t initialPositionMS)
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
            mUserActions.clear();
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

    Result::Enum OmafPlayerPrivate::nextOverlayGroup()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        mVideoProvider->nextSourceGroup();

        return Result::OK;
    }

    Result::Enum OmafPlayerPrivate::prevOverlayGroup()
    {
        if (mSuspended)
        {
            return Result::INVALID_STATE;
        }

        mVideoProvider->prevSourceGroup();

        return Result::OK;
    }

    bool_t OmafPlayerPrivate::isSeekable()
    {
        if (mSuspended)
        {
            return false;
        }

        return mVideoProvider->isSeekable();
    }

    static void fillActionRegion(UserAction& act, uint32_t aOverlayId, Private::OverlaySource* regionOvly)
    {
        act.overlayId = aOverlayId;

        if (regionOvly->overlayParameters.sphereRelative2DOverlay.doesExist)
        {
            act.actionRegion.type = ActionRegionType::SPHERE_RELATIVE_2D;
            auto& ovlyReg = regionOvly->overlayParameters.sphereRelative2DOverlay;
            act.actionRegion.centerLatitude = (float) ovlyReg.sphereRegion.centreElevation / 65536.0;
            act.actionRegion.centerLongitude = (float) ovlyReg.sphereRegion.centreAzimuth / 65536.0;
            act.actionRegion.spanLatitude = (float) ovlyReg.sphereRegion.elevationRange / 65536.0;
            act.actionRegion.spanLongitude = (float) ovlyReg.sphereRegion.azimuthRange / 65536.0;

            act.actionRegion.yaw = ovlyReg.overlayRotation.yaw;
            act.actionRegion.pitch = ovlyReg.overlayRotation.pitch;
            act.actionRegion.roll = ovlyReg.overlayRotation.roll;
        }

        if (regionOvly->overlayParameters.sphereRelativeOmniOverlay.doesExist)
        {
            act.actionRegion.type = ActionRegionType::SPHERE_RELATIVE_OMNI;
            auto& ovlyReg = regionOvly->overlayParameters.sphereRelativeOmniOverlay;
            act.actionRegion.centerLatitude = (float) ovlyReg.sphereRegion.centreElevation / 65536.0;
            act.actionRegion.centerLongitude = (float) ovlyReg.sphereRegion.centreAzimuth / 65536.0;
            act.actionRegion.spanLatitude = (float) ovlyReg.sphereRegion.elevationRange / 65536.0;
            act.actionRegion.spanLongitude = (float) ovlyReg.sphereRegion.azimuthRange / 65536.0;

            act.actionRegion.yaw = 0;
            act.actionRegion.pitch = 0;
            act.actionRegion.roll = ovlyReg.sphereRegion.elevationRange / 65536.0;
        }
    }

    IArray<UserAction>& OmafPlayerPrivate::getUserActions()
    {
        mVideoProvider->enter();
        const Private::CoreProviderSources& sources = mVideoProvider->getAllSources();
        if (sources.isEmpty())
        {
            // not ready yet
            // OMAF_LOG_D("Not able to get any sources yet");
        }

        // always complete recreate
        mUserActions.clear();

        for (Private::CoreProviderSource* source : sources)
        {
            if (source->type == Private::SourceType::OVERLAY)
            {
                Private::OverlaySource* ovlySource = (Private::OverlaySource*) (source);

                // depth modify action area for sphere relative overlays
                if (ovlySource->overlayParameters.sphereRelative2DOverlay.doesExist ||
                    ovlySource->overlayParameters.sphereRelativeOmniOverlay.doesExist)
                {
                    UserAction act{};
                    fillActionRegion(act, ovlySource->overlayParameters.overlayId, ovlySource);
                    act.canUpdateDistance = true;
                    mUserActions.add(act);
                }

				if (ovlySource->overlayParameters.associatedSphereRegion.doesExist)
                {
                    // create action region for associasted sphere region
                    UserAction act{};

                    act.overlayId = ovlySource->overlayParameters.overlayId;

                    act.actionRegion.type = ActionRegionType::SPHERE_RELATIVE_2D;

                    auto& ovlyReg = ovlySource->overlayParameters.associatedSphereRegion;
                    act.actionRegion.centerLatitude = (float) ovlyReg.sphereRegion.centreElevation / 65536.0;
                    act.actionRegion.centerLongitude = (float) ovlyReg.sphereRegion.centreAzimuth / 65536.0;
                    act.actionRegion.spanLatitude = (float) ovlyReg.sphereRegion.elevationRange / 65536.0;
                    act.actionRegion.spanLongitude = (float) ovlyReg.sphereRegion.azimuthRange / 65536.0;

                    act.actionRegion.yaw = 0;
                    act.actionRegion.pitch = 0;
                    act.actionRegion.roll = 0;

                    act.canActivate = true;
                    mUserActions.add(act);
                }
            }
        }
        // then check the viewpoints
        Private::ViewpointSwitchControls vpControls;
        mVideoProvider->getViewpointUserControls(vpControls);
        for (Private::ViewpointSwitchControl& control : vpControls)
        {
            // create action region
            UserAction act{};
            // switch takes place

            act.destinationViewpointId = control.destinationViewpointId;
            // find overlay based on the overlay ID
            bool_t found = false;
            for (Private::CoreProviderSource* source : sources)
            {
                if (source->type == Private::SourceType::OVERLAY)
                {
                    Private::OverlaySource* ovlySource = (Private::OverlaySource*) (source);

                    if (control.viewpointSwitchRegion.refOverlayId == ovlySource->overlayParameters.overlayId)
                    {
                        fillActionRegion(act, ovlySource->overlayParameters.overlayId, ovlySource);
                        found = true;
                        break;
                    }
                }
            }

            if (found)
            {
                act.canSwitchViewpoint = true;
                mUserActions.add(act);
            }
            else
            {
                OMAF_LOG_V("Overlay for viewpoint switching not found!");
            }
        }

        mVideoProvider->leave();

        return mUserActions;
    }

    /**
     * Maybe this thing could be extended to support other type of actions too at some point,
     * but right now the whole UserAction description stuff is designed only for controlling overlays,
     * including switching viewpoints via overlay.
     */
    Result::Enum OmafPlayerPrivate::runUserAction(const OverlayControl& cmd)
    {
        return convertResult(mVideoProvider->controlOverlay(cmd));
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

}  // namespace OMAF
