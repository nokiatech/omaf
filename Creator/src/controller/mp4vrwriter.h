
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

#include <functional>
#include <map>

#include "common/exceptions.h"
#include "segmenter/metadata.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/singlefilesave.h"

namespace VDD
{
    class AsyncProcessor;
    class ConfigValue;
    class ControllerOps;

    enum class PipelineMode : int;
    enum class PipelineOutput : int;

    struct VRVideoConfig
    {
        StreamAndTrackIds ids;
        OperatingMode mode;
        bool packedSubPictures;
        Optional<StreamAndTrack> extractor;
    };

    enum class PipelineMode : int;
    class MP4VRWriter;
    using MP4VROutputs = std::map<std::pair<PipelineMode, std::string>, std::unique_ptr<MP4VRWriter>>;

    class MP4VRWriter
    {
    public:
        struct Config
        {
            StreamSegmenter::Segmenter::Duration segmentDuration;
            bool fragmented;
            std::shared_ptr<Log> log;
        };

        static Config loadConfig(const ConfigValue& aConfig);

        MP4VRWriter(ControllerOps& aOps, const Config& aConfig, std::string aName, PipelineMode aMode);

        MP4VRWriter(MP4VRWriter&&) = delete;
        MP4VRWriter(const MP4VRWriter&) = delete;

        /** @brief Create a new track to embed to this MP4VR file */
        AsyncProcessor* createSink(Optional<VRVideoConfig> aVRVideoConfig,
                                   PipelineOutput aPipelineOutput,
                                   FrameDuration aFrameDuration,
                                   FrameDuration aRandomAccessPeriod);

        /** @brief No more sinks are going to be created, you have the required configuration,
         * finalize the pipeline */
        void finalizePipeline();

    private:
        std::reference_wrapper<ControllerOps> mOps;
        std::shared_ptr<Log> mLog;

        /* Used for constructing new track ids */
        size_t mTrackCount = 0;

        Segmenter::Config mSegmenterConfig;
        SegmenterInit::Config mSegmenterInitConfig;
        SingleFileSave::Config mSingleFileSaveConfig;

        std::map<TrackId, AsyncProcessor*> mTrackSinks;

        TrackId createTrackId();

        void createVRMetadataGenerator(TrackId aTrackId, VRVideoConfig aVRVideoConfig, FrameDuration aTimeScale, FrameDuration aRandomAccessDuration);

        bool mFragmented;
    };
}
