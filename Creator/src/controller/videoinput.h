
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
#pragma once

#include <list>

#include "common.h"

#include "processor/data.h"
#include "common/exceptions.h"
#include "config/config.h"


namespace VDD
{
    enum class VideoInputMode
    {
        Mono,    // no packing, one video track
        LeftRight,
        RightLeft,
        TopBottom,
        BottomTop,
        Separate // two separate tracks. VideoFilenameConfiguration::videoIndices indicates the order.
    };

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
        int tilesX;
        int tilesY;
        int ctuSize;

        VideoFileProperties() = default;
    };

    class VideoInput
    {
    public:
        VideoInput();
        virtual ~VideoInput();

        /** @brief Retrieve properties of the given video file */
        virtual VideoFileProperties getVideoFileProperties(std::string aFilename);

        /** @brief Opens mp4 file for input */
        virtual bool setupMP4VideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure, const ConfigValue& aConfig, VideoProcessingMode aProcessingMode);

    protected:
        struct GenericVRVideo
        {
            ConfigValue config;
            ConfigValue filename;
            std::set<TrackId> videoTracks;
            std::set<VideoInputMode> allowedInputModes;
        };

        virtual bool setupGenericVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            GenericVRVideo aGenericVRVideo,
            MP4Loader& aMP4Loader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        virtual bool setupOMAFVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            VideoInput::GenericVRVideo aGenericVRVideo,
            MP4Loader& aMP4Loader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        /** @brief Setup the input pipeline according to the Google VR Video metadata */
        virtual void setupGoogleVRVideoInput(ControllerOps& aOps, ControllerConfigure& aConfigure,
            GoogleVRVideoMetadata aGoogleVRVideoMetadata,
            MP4Loader& aMP4Loader,
            std::string aInputLabel,
            VideoProcessingMode aProcessingMode);

        virtual VideoInputMode validateMp4VideoInputConfig(GenericVRVideo aGenericVRVideo, MP4Loader& aMP4Loader, std::list<std::unique_ptr<MP4LoaderSource>>& aSources);

        virtual bool buildVideoPassThrough(ControllerOps& aOps, std::string aInputLabel, VideoInputMode aMode,
            std::list<std::unique_ptr<MP4LoaderSource>>& aSources,
            AsyncNode*& aInputLeft, ViewMask& aInputLeftMask,
            AsyncNode*& aInputRight, ViewMask& aInputRightMask);

    };
}
