
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

#include "NVRNamespace.h"
#include "Foundation/NVRPathName.h"

#include "NVRPlayer.h"
#include "OMAFPlayer.h"

#include "API/OMAFDataTypeConversions.h"


namespace OMAF
{
    struct AudioState
    {
        bool_t initialized;

        bool_t useDirectRouting;
        bool_t allowExclusiveMode;

        AudioObserver* observer;
        int8_t outChannels;
        AudioPlaybackMode::Enum playbackMode;
        AudioOutputRange::Enum outputRange;
    };

    class OmafPlayerPrivate
    : public IOMAFPlayer
    , public IRenderer
    , public IPlaybackControls
    , public IAudio
    , public Private::AudioRendererObserver
    {
    public:

        OmafPlayerPrivate();
        virtual ~OmafPlayerPrivate();

        // IOMAFPlayer
        virtual Version getVersionNumber();

        virtual IAudio* getAudio();
        virtual IPlaybackControls* getPlaybackControls();
        virtual IRenderer* getRenderer();

        virtual bool_t isFeatureSupported(Feature::Enum feature);

        virtual void_t suspend();

        virtual void_t resume();

    public: // IAudio

        virtual Result::Enum initializeAudioWithDirectRouting(bool_t allowExclusiveMode);

        virtual Result::Enum initializeAudioWithDirectRouting(bool_t allowExclusiveMode, const wchar_t* audioDevice);

        virtual Result::Enum resetAudio();

        virtual Result::Enum setGain(float gain);
        virtual Result::Enum setAudioLatency(int64_t latencyUs);

        virtual AudioReturnValue::Enum renderSamples(size_t bufferSize, float* samples, size_t& samplesRendered);
        virtual AudioReturnValue::Enum renderSamples(size_t bufferSize, int16_t* samples, size_t& samplesRendered);

        virtual void firstSamplesConsumed();

        virtual void setHeadTransform(const HeadTransform& headTransform);

    public: // IRenderer

        virtual uint32_t createRenderTarget(const RenderTextureDesc* colorAttachment, const RenderTextureDesc* depthStencilAttachment);
        virtual void destroyRenderTarget(uint32_t handle);

        virtual void renderSurfaces(const HeadTransform& aHeadTransform, const RenderSurface** aRenderSurfaces, uint8_t aNumRenderSurfaces, const bool aDisplayWaterMark = false);

        virtual void renderSurfaces(const HeadTransform& aHeadTransform, const RenderSurface** aRenderSurfaces, uint8_t aNumRenderSurfaces, const RenderingParameters& aRenderingParameters = RenderingParameters());

    public: // IPlaybackControls

        virtual void setVideoPlaybackObserver(OMAF::VideoPlaybackObserver* observer);
        virtual OMAF::VideoPlaybackState::Enum getVideoPlaybackState();
        virtual OMAF::Result::Enum loadVideo(const char *uri, uint64_t initialPositionMS);
        virtual OMAF::Result::Enum play();
        virtual OMAF::Result::Enum pause();
        virtual OMAF::Result::Enum stop();
        virtual OMAF::Result::Enum next();
        virtual bool isSeekable();
        virtual OMAF::Result::Enum seekTo(uint64_t milliSeconds, SeekAccuracy::Enum accuracy);
        virtual uint64_t elapsedTime();
        virtual uint64_t duration();

        virtual MediaInformation getMediaInformation();

        virtual void_t setAudioVolume(float aVolume);

        virtual void_t setAudioVolumeAuxiliary(float aVolume);


        virtual Result::Enum loadAuxiliaryVideo(const char* aUri);

        virtual OMAF::VideoPlaybackState::Enum getAuxiliaryVideoPlaybackState();

        virtual Result::Enum playAuxiliary();

        virtual Result::Enum pauseAuxiliary();

        virtual Result::Enum stopAuxiliary();

        virtual Result::Enum seekToAuxiliary(uint64_t aMilliSeconds);

        virtual uint64_t elapsedTimeAuxiliary();
        virtual uint64_t durationAuxiliary();

    public: // AudioRenderer observer

        virtual void_t onRendererReady();
        virtual void_t onRendererPlaying();
        virtual void_t onFlush();
        virtual int64_t onGetPlayedTimeUs();
        virtual void_t onErrorOccurred(Private::Error::Enum error);
        virtual void_t onRendererPaused();

    private:
        void_t initialize();
        void_t deinitialize();

    private:

        Private::MemoryAllocator &mAllocator;
        Private::VideoProvider* mVideoProvider;
        Private::RenderingManager* mRenderingManager;
        Private::AudioRendererAPI* mAudioRenderer;
        Private::AudioBackend *mAudioBackend;

        OMAF::AudioState mAudioState;

        bool_t mSuspended;
        Private::PathName mLastLoadedVideo;
        uint64_t mElapsedTimeAtSuspend;
    };
}
