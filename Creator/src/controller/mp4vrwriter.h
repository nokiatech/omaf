
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

#include <functional>
#include <map>

#include "pipelinemode.h"
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

    class PipelineOutput;

    struct VRVideoConfig
    {
        StreamAndTrackIds ids;
        OutputMode mode;
        bool packedSubPictures;
        Optional<StreamAndTrack> extractor;
        Optional<ISOBMFF::OverlayStruct> overlays;
    };

    class MP4VRWriter;
    using MP4VROutputs =
        std::map<std::pair<PipelineMode, std::string>, std::unique_ptr<MP4VRWriter>>;

    class MP4VRWriter
    {
    public:
        struct Config
        {
            StreamSegmenter::Segmenter::Duration segmentDuration;
            bool fragmented;
            std::shared_ptr<Log> log;
            Optional<Future<Optional<StreamSegmenter::Segmenter::MetaSpec>>> fileMeta;
        };

        struct Sink
        {
            AsyncProcessor* sink;
            std::set<TrackId> trackIds;
        };

        static Config loadConfig(const ConfigValue& aConfig);

        MP4VRWriter(ControllerOps& aOps, const Config& aConfig, std::string aName,
                    PipelineMode aMode);

        MP4VRWriter(MP4VRWriter&&) = delete;
        MP4VRWriter(const MP4VRWriter&) = delete;

        /** @brief Create a new track to embed to this MP4VR file */
        Sink createSink(Optional<VRVideoConfig> aVRVideoConfig, const PipelineOutput& aPipelineOutput,
                        FrameDuration aFrameDuration,
                        const std::map<std::string, std::set<TrackId>>& aTrackReferences =
                            std::map<std::string, std::set<TrackId>>());

        /** @brief No more sinks are going to be created, you have the required configuration,
         * finalize the pipeline */
        void finalizePipeline();

    private:
        std::reference_wrapper<ControllerOps> mOps;
        std::shared_ptr<Log> mLog;

        /* Used for constructing new track ids; bumped to 200u to "handle" the case of assigning
           track ids outside this module */
        size_t mTrackIdCount = 200u;

        /* Used for constructing accompanying stream ids so we can add them to mSegmenterConfig.
           All in all this stuff should be removed. */
        size_t mStreamIdCount = 1000u;

        Segmenter::Config mSegmenterConfig;
        SegmenterInit::Config mSegmenterInitConfig;
        SingleFileSave::Config mSingleFileSaveConfig;

        std::map<TrackId, AsyncProcessor*> mTrackSinks;
        std::list<std::pair<AsyncProcessor*, StreamFilter>>
            mVRTrackSinks;  // actually NoOps that are eventually forwarded to the actual segmenter

        TrackId createTrackId();
        StreamId createStreamId();

        void createVRMetadataGenerator(TrackId aTrackId, VRVideoConfig aVRVideoConfig,
                                       FrameDuration aTimeScale,
                                       FrameDuration aRandomAccessDuration);

        bool mFragmented;

        bool mFinalized = false;
    };
}  // namespace VDD
