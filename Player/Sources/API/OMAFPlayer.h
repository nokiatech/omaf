
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
#pragma once

#include "API/OMAFIArray.h"
#include "API/OMAFPlayerPlatformParameters.h"
#include "OMAFPlayerDataTypes.h"

#if defined(__ANDROID__)

#if defined(OMAF_BUILD_SHARED_LIBRARY)

#define OMAF_EXPORT __attribute__((visibility("default")))

#else

#define OMAF_EXPORT  // Static library only

#endif

#elif defined(_WIN32)

#if defined(OMAF_BUILD_SHARED_LIBRARY)

#define OMAF_EXPORT __declspec(dllexport)

#elif defined(OMAF_BUILD_STATIC_LIBRARY)

#define OMAF_EXPORT  // Static library only

#else

#define OMAF_EXPORT __declspec(dllimport)

#endif

#elif ((defined __MACH__) && (defined __APPLE__))

#if defined(OMAF_BUILD_SHARED_LIBRARY)

#define OMAF_EXPORT __attribute__((visibility("default")))

#else

#define OMAF_EXPORT  // Static library only

#endif

#else

#define OMAF_EXPORT  // Static library only

#endif

namespace OMAF
{
    /**
     * Observer class for video playback errors
     * NOTE: This observer interface is now deprecated and will not be called
     */
    class VideoPlaybackObserver
    {
    public:
        virtual ~VideoPlaybackObserver()
        {
        }

        /**
         * Callback for errors which occur in asynchronous operations
         * @param error Error code
         */
        virtual void onErrorOccurred(Result::Enum aError) = 0;
    };

    /**
     * Observer class for the audio rendering. Anyone who wants to implement their own audio
     * playback should implement this observer. Not required when using direct routing for the audio.
     */
    class AudioObserver
    {
    public:
        virtual ~AudioObserver()
        {
        }

        /**
         * New audio samples have become available. They should be fetched with IAudio::renderSamples
         */
        virtual void onNewSamplesAvailable() = 0;

        /**
         * Request to report the played time in microseconds. Is used to finetune AV-sync
         */
        virtual int64_t onGetPlayedTimeUs() = 0;

        /**
         * Request to flush audio buffers, e.g. when seeking. After this call, IAudio::firstSamplesConsumed need to be
         * called and times returned by onGetPlayedTimeUs should restart from 0
         */
        virtual void onFlush() = 0;

        /**
         * Informs that audio renderer is ready and will start producing output samples
         * @param sampleRate
         */
        virtual void onRendererReady(int32_t aSampleRate) = 0;

        /**
         * Informs the audio renderer to pause playback. playback should resume when onNewSamplesAvailable is called
         * again.
         */
        virtual void onRendererPaused() = 0;
    };

    /**
     * Interface for rendering video.
     */
    class IRenderer
    {
    public:
        /**
         * Default destructor. Calling delete directly can/will cause a crash.
         * Renderer is destroyed by calling destroy on the main instance
         */
        virtual ~IRenderer()
        {
        }

        /**
         * Creates a render target for RenderSurface
         * @param colorAttachment The native color texture attachment
         * @param depthStencilAttachment The native depth stencil texture attachment
         * @return The render target handle for RenderSurface
         */
        virtual uint32_t createRenderTarget(const RenderTextureDesc* aColorAttachment,
                                            const RenderTextureDesc* aDepthStencilAttachment = NULL) = 0;

        /**
         * Destroys the render target
         * @param handle The handle of the render target
         */
        virtual void destroyRenderTarget(uint32_t aHandle) = 0;

        /**
         * Renders in to the given render surface.
         * @param headTransform The head transform for rendering
         * @param renderSurfaces The render surface for rendering
         * @param numRenderSurfaces The number of render surface for rendering
         * @param renderingParameters Struct containing different parameters for the rendering
         */
        virtual void renderSurfaces(const HeadTransform& aHeadTransform,
                                    const RenderSurface** aRenderSurfaces,
                                    uint8_t aNumRenderSurfaces,
                                    const RenderingParameters& aRenderingParameters = RenderingParameters()) = 0;

        /**
         * Renders in to the given render surface.
         * NOTE: This method is deprecated, please use the other renderSurfaces method
         * @param headTransform The head transform for rendering
         * @param renderSurfaces The render surface for rendering
         * @param numRenderSurfaces The number of render surface for rendering
         * @param displayWaterMark Flag to enable the watermark rendering (off by default). Evaluation version of the
         * SDK will always render the watermark.
         */
        virtual void renderSurfaces(const HeadTransform& aHeadTransform,
                                    const RenderSurface** aRenderSurfaces,
                                    uint8_t aNumRenderSurfaces,
                                    const bool aDisplayWaterMark) = 0;
    };

    /**
     * Interface for audio playback
     */
    class IAudio
    {
    public:
        /**
         * Default destructor. Calling delete directly can/will cause a crash.
         * Audio is destroyed by calling destroy on the main instance
         */
        virtual ~IAudio()
        {
        }

        /**
         * Initializes the audio playback using direct routing
         * When used, the audio samples are directly played back without the ability to edit them.
         * The output will be binauralized, headtracked stereo.
         * @param allowExclusiveMode Tells if the audio is allowed to run in exclusive mode. This can improve latency
         * but can block others from accessing the audio hardware. Currently supported only on Windows
         * @return Result
         */
        virtual Result::Enum initializeAudioWithDirectRouting(bool_t aAllowExclusiveMode = false) = 0;

        /**
         * Initializes the audio playback using direct routing
         * When used, the audio samples are directly played back without the ability to edit them.
         * The output will be binauralized, headtracked stereo.
         * @param allowExclusiveMode Tells if the audio is allowed to run in exclusive mode. This can improve latency
         * but can block others from accessing the audio hardware. Currently supported only on Windows
         * @param audioDevice The UID of the audio device to be used. This is used only on Windows
         * @return Result
         */
        virtual Result::Enum initializeAudioWithDirectRouting(bool_t aAllowExclusiveMode,
                                                              const wchar_t* audioDevice) = 0;

        /**
         * Resets the audio to a state where a new initialization is required
         * @return Result
         */
        virtual Result::Enum resetAudio() = 0;

        /**
         * Sets the gain for the binauralizer
         * @param gain The value for the gain, values 0.0-1.0 can be used to reduce audio volume, and values > 1.0
         * increase the volume
         * @return Result
         */
        virtual Result::Enum setGain(float aGain) = 0;
    };

    /**
     * Interface for controlling the video playback
     */
    class IPlaybackControls
    {
    public:
        /**
         * Default destructor. Calling delete directly can/will cause a crash.
         * PlaybackControls are destroyed by calling destroy on the main instance
         */
        virtual ~IPlaybackControls()
        {
        }

        /**
         * Sets the observer for the Playback control interface
         * @param observer A pointer to an object which implements the observer interface
         */
        virtual void setVideoPlaybackObserver(VideoPlaybackObserver* aObserver) = 0;

        /**
         * Returns the current state of the video playback
         */
        virtual VideoPlaybackState::Enum getVideoPlaybackState() = 0;

        /**
         * Returns the current state of the video playback
         */
        virtual OverlayState getOverlayState(uint32_t ovlyId) const = 0;

        /**
         * Loads a video
         * The operation is asynchronous so an error code can be returned through the observer
         * @param uri The location of the file, should have either "asset://", "storage://" or "file://" prefix
         * @param initialPositionMS Initial position of the stream in milliseconds. Will be ignored if larger than the
         * duration of the stream
         * @return Result of the call
         */
        virtual Result::Enum loadVideo(const char* aUri, uint64_t aInitialPositionMS = 0) = 0;

        /**
         * Return a struct containing information about the currently loaded stream. Will contain valid information
         * after file loading is completed
         * @return A struct containing the media information
         */
        virtual MediaInformation getMediaInformation() = 0;

        /**
         * Starts the playback after loading a video or after pausing
         * If called immediately after calling load, the playback will start once the
         * asynchronous loading of the video is complete
         * Asynchronous operation so error maybe returned through the observer
         * @return Result of the call
         */
        virtual Result::Enum play() = 0;

        /**
         * Pauses the playback.
         * @return Result
         */
        virtual Result::Enum pause() = 0;

        /**
         * Stops the playback and releases the resources
         * After calling stop, a video must be (re)loaded using loadVideo before playback can continue
         * @return Result
         */
        virtual Result::Enum stop() = 0;

        /**
         * Requests for changing sources to the next item in the video archive for multi-item archives.
         * @return Result
         */
        virtual Result::Enum next() = 0;

        /**
         * Requests for changing sources to the overlay group.
         * @return Result
         */
        virtual Result::Enum nextOverlayGroup() = 0;

        /**
         * Requests for changing sources to the previous overlay group.
         * @return Result
         */
        virtual Result::Enum prevOverlayGroup() = 0;

        /**
         * Information about possible user interactions.
         */
        virtual IArray<UserAction>& getUserActions() = 0;

        // experimental API for triggering user actions
        virtual Result::Enum runUserAction(const OverlayControl& cmd) = 0;

        /**
         * Tells if the source is seekable. The source needs to be loaded fully before the value is valid
         * @return If the source can be seeked
         */
        virtual bool isSeekable() = 0;

        /**
         * Seeks the video to the given time instant.
         * Seeking is asynchronous operation, and is not completed when this function returns.
         * Possible errors in seeking (e.g. seeking past the end of the clip) are reported via observer callback.
         * @param milliSeconds The seek target time in milliseconds from the beginning of the video
         * @return Result
         */
        virtual Result::Enum seekTo(uint64_t aMilliSeconds,
                                    SeekAccuracy::Enum accuracy = SeekAccuracy::FRAME_ACCURATE) = 0;

        /**
         * Returns the current video playback position in milliseconds.
         * @return Elapsed time in milliseconds
         */
        virtual uint64_t elapsedTime() = 0;

        /**
         * Returns the duration of the clip in milliseconds
         * If the total duration of the video is not known, the return value will be 0
         * @return The duration of the video
         */
        virtual uint64_t duration() = 0;

        /**
         * Sets the audio volume for the stream
         * @param aVolume Volume, ranging from 0.0f to 1.0f
         */
        virtual void_t setAudioVolume(float aVolume) = 0;
    };

    /**
     * Main interface for the SDK.
     */
    class IOMAFPlayer
    {
    public:
        /**
         * Default destructor. The instance must be destroyed using the destroy method.
         * Calling delete directly can/will cause a crash
         */
        virtual ~IOMAFPlayer()
        {
        }

        /**
         * Creates the main instance of the Player
         * @param graphicsAPI The graphics API to be used by this instance of the Player
         * @param platformParameters Pointer for passing platform specific information to the Player
         * @return Pointer to the created instance, returns NULL if the creation failed
         */
        OMAF_EXPORT static IOMAFPlayer* create(GraphicsAPI::Enum aGraphicsAPI, PlatformParameters* aPlatformParameters);

        /**
         * Destroys the given instance. The instance must be deleted with the destroy method.
         * Calling delete directly on the instance can/will cause a crash
         * @param aOmafPlayer Pointer to the instance to be destroyed
         */
        OMAF_EXPORT static void destroy(IOMAFPlayer* aOmafPlayer);

        /**
         * Suspends the SDK. Should be called when the app using the SDK is moved to the background
         */
        virtual void_t suspend() = 0;

        /**
         * Wakes up the SDK after suspending.
         */
        virtual void_t resume() = 0;

        /**
         * Returns the version number information of the created instance
         * @return Version struct containing the version information
         */
        virtual Version getVersionNumber() = 0;

        /**
         * Returns if the given feature is supported by the used device
         * @return If the feature is supported
         */
        virtual bool_t isFeatureSupported(Feature::Enum aFeature) = 0;

        /**
         * Returns a pointer to the audio interface
         * @return Pointer to the audio interface
         */
        virtual IAudio* getAudio() = 0;

        /**
         * Returns a pointer to the playback controls interface
         * @return Pointer to the Playback controls
         */
        virtual IPlaybackControls* getPlaybackControls() = 0;

        /**
         * Returns a pointer to the renderer interface
         * @return Pointer to the renderer interface
         */
        virtual IRenderer* getRenderer() = 0;

        /**
         * Returns a initial offset that is used to fix viewing orientation.
         */
        virtual const Quaternion viewingOrientationOffset() const = 0;
    };
}  // namespace OMAF
