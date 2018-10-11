
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

#include <string>
#include <functional>

#include "dash.h"
#include "mp4vrwriter.h"

#include "common/optional.h"
#include "config/config.h"
#include "videoinput.h"


namespace VDD
{
    namespace PipelineModeDefs
    {

        const std::map<std::string, PipelineMode> kPipelineModeNameToType{
            { "mono",        PipelineMode::VideoMono },
			{ "framepacked", PipelineMode::VideoFramePacked },
            { "topbottom",   PipelineMode::VideoTopBottom },
            { "sidebyside",  PipelineMode::VideoSideBySide },
            { "separate",    PipelineMode::VideoSeparate },
            { "audio",       PipelineMode::Audio }
        };

        struct PipelineOutputTypeInfo
        {
            std::string outputName;
            std::string filenameComponent;
        };

        // Note: this now merges all videos to "video", hence eliminating possibility to use two-channel stereo. This is done for simplicity, as OMAF doesn't really consider multi-track videos
        const std::map<PipelineOutput, PipelineOutputTypeInfo> kOutputTypeInfo{
            { PipelineOutput::VideoMono,{ "video", "video" } },
			{ PipelineOutput::VideoFramePacked,{ "video", "video" } },
            { PipelineOutput::VideoTopBottom,{ "video", "video" } },
            { PipelineOutput::VideoSideBySide,{ "video", "video" } },
            { PipelineOutput::VideoLeft,{ "video", "video" } },
            { PipelineOutput::VideoRight,{ "video", "video" } },
            { PipelineOutput::Audio,{ "audio", "audio" } },
            { PipelineOutput::VideoExtractor,{ "extractor", "extractor" } }
        };

    }

    class Log;

    class ControllerConfigure;

    class ControllerBase
    {
    public:
        struct Config
        {
            std::shared_ptr<VDD::Config> config;

            /* Logging facility to use. If not set, defaults to ConsoleLog. */
            std::shared_ptr<Log> log;
        };

        ControllerBase();
        ~ControllerBase();

    protected:
        AsyncNode* mInputLeft = nullptr;
        ViewMask mInputLeftMask;
        AsyncNode* mInputRight = nullptr;
        ViewMask mInputRightMask;

        bool mIsStereo;
        VideoInputMode mInputVideoMode;
        FrameDuration mVideoFrameDuration;
        FrameDuration mVideoTimeScale;
        VideoGOP mGopInfo;

        /** Builds the pipeline from a raw frame source (ie. import or tiling or audio) to saving
            segments. If there is no video encoder config, the video encoder is skipped and
            pipeline is built for raw data (ie AAC).

            @param aName Name of the output
            @param aDashOutput The Dash output configuration for this stream
            @param aPipelineOutput What kind of output is this? Mono, Left, Audio, etc
            @param aSources Which source to capture the initial frames from (raw import, aac import, or tiler)
            Uses aSegmenterOutput to choose the correct source from here.
            @param aMP4VRSink If this pipeline (the part before segmenter) is to be written to
            an MP4VR file, provide a writer sink

            @return Return the end of the pipeline, suitable for using with configureMPD
        */
        virtual AsyncNode* buildPipeline(std::string aName,
            Optional<DashSegmenterConfig> aDashOutput,
            PipelineOutput aPipelineOutput,
            PipelineOutputNodeMap aSources,
            AsyncProcessor* aMP4VRSink) = 0;

        /** @brief Builds the dash-pipeline from given aac input. Used by
        * makeAudio its last stage */
        virtual void makeAudioDashPipeline(const ConfigValue& aDashConfig,
            std::string aAudioName,
            AsyncNode* aAacInput,
            FrameDuration aTimeScale) {};


        friend class ControllerConfigure;
    };

}
