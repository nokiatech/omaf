
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

#include <list>
#include "reader/mp4vrfiledatatypes.h"
#include "mp4vromafreaders.h"

#include "common.h"

#include "processor/data.h"
#include "common/exceptions.h"
#include "config/config.h"
#include "medialoader/mp4loader.h"
#include "mediainput.h"

namespace VDD
{
    enum class VideoInputMode : int
    {
        Mono,    // no packing, one video track
        LeftRight,
        RightLeft,
        TopBottom,
        BottomTop,
        Separate, // two separate tracks. VideoFilenameConfiguration::videoIndices indicates the order.
        TemporalInterleaving,
        NonVR    // basic 2d video
    };

    VideoInputMode videoInputOfProperty(MP4VR::StereoScopic3DProperty aStereoscopic);
    VideoInputMode videoInputOfProperty(MP4VR::PodvStereoVideoConfiguration aStereoscopic);

    enum class VideoProcessingMode
    {
        Transcode,
        Passthrough
    };

    class UnsupportedVideoInput : public Exception
    {
    public:
        UnsupportedVideoInput(std::string aMessage);

        std::string message() const;

    private:
        std::string mMessage;
    };

    class IndeterminateVideoPacking : public Exception
    {
    public:
        IndeterminateVideoPacking();
    };

    std::ostream& operator<<(std::ostream& aStream, VideoInputMode aMode);

    struct VideoFilenameConfiguration
    {
        VideoInputMode videoInputMode;
        std::list<TrackId> videoTracks;
    };

    class ControllerOps;
    class ControllerConfigure;

    class MP4Loader;
    class MP4LoaderSource;
    struct GoogleVRVideoMetadata;

    struct VideoGOP
    {
        FrameDuration duration;
        bool fixed;
    };

    struct VideoFileProperties
    {
        bool stereo;

        VideoInputMode mode;

        // of one picture, ie. half the size of the actual track in case of frame packing
        uint32_t width;
        uint32_t height;

        FrameRate fps;

        FrameDuration timeScale;

        VideoGOP gopDuration;

        bool tiles;
        size_t tilesX;
        size_t tilesY;
        int ctuSize;

        VideoFileProperties() = default;
    };

    extern const std::function<VDD::VideoInputMode(const VDD::ConfigValue&)> readVideoInputMode;

    /** Converts VideoInputMode */
    PipelineOutput pipelineOutputOfVideoInputMode(VideoInputMode aInputMode);

    class VideoInput
    {
    public:
        VideoInput();
        virtual ~VideoInput();

        /** @brief Retrieve properties of the given MP4 source */
        virtual VideoFileProperties getVideoFileProperties(const ParsedValue<MediaInputConfig>& aMP4LoaderConfig,
                                                           Optional<FrameDuration> aOverrideFrameDuration);

        /** @brief Opens mp4 file for input */
        virtual bool setupMP4VideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
                                        const ConfigValue& aConfig,
                                        VideoProcessingMode aProcessingMode,
                                        Optional<ParsedValue<RefIdLabel>>& aRefIdLabel);

    protected:
        struct GenericVRVideo
        {
            ConfigValue config;
            ParsedValue<MediaInputConfig> mediaInputConfig;
            std::set<TrackId> videoTracks;
            std::set<VideoInputMode> allowedInputModes;
        };

        virtual bool setupGenericVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            GenericVRVideo aGenericVRVideo,
            MediaLoader& aMediaLoader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        virtual bool setupOMAFVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            VideoInput::GenericVRVideo aGenericVRVideo,
            MediaLoader& aMediaLoader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        /** @brief Setup the input pipeline according to the Google VR Video metadata */
        virtual void setupGoogleVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            GoogleVRVideoMetadata aGoogleVRVideoMetadata,
            MediaLoader& aMediaLoader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        virtual VideoInputMode validateMp4VideoInputConfig(GenericVRVideo aGenericVRVideo, MediaLoader& aMediaLoader, std::list<std::unique_ptr<MediaSource>>& aSources);

        virtual bool buildVideoPassThrough(ControllerOps& aOps, std::string aInputLabel, VideoInputMode aMode,
            std::list<std::unique_ptr<MediaSource>>& aSources,
            AsyncNode*& aInputLeft, StreamFilter& aInputLeftMask,
            AsyncNode*& aInputRight, StreamFilter& aInputRightMask);

    };

}  // namespace VDD
