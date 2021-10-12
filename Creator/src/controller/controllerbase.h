
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

#include <string>
#include <functional>

#include "dashomaf.h"
#include "mp4vrwriter.h"

#include "common/optional.h"
#include "config/config.h"
#include "videoinput.h"
#include "concurrency/threadedpoolprocessor.h"

namespace VDD
{
    class CombineNode;

    namespace PipelineModeDefs
    {
        struct PipelineOutputTypeKey
        {
            PipelineClass pipelineClass;
            bool isExtractor;
        };

        inline bool operator<(const PipelineOutputTypeKey& a, const PipelineOutputTypeKey& b)
        {
            return (a.pipelineClass < b.pipelineClass)
                       ? true
                       : (a.pipelineClass > b.pipelineClass) ? false
                                                             : a.isExtractor < b.isExtractor;
        }

        struct PipelineOutputTypeInfo
        {
            std::string outputName;
            std::string filenameComponent;
        };
    }  // namespace PipelineModeDefs

    class Log;

    class View;

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

        ControllerBase(const Config& aConfig);
        virtual ~ControllerBase();

        StreamId newAdaptationSetId();
        StreamId newExtractorAdaptationSetIt(); // Gives a nice number like 1000, 2000, etc
        TrackId newTrackId();

        /** Retrieve Dash durations; useful for determining ie. GOP length. */
        SegmentDurations getDashDurations() const;

        const TileIndexTrackGroupIdMap& getTileIndexTrackGroupMap(PipelineOutput aOutput) const;
        TileIndexTrackGroupIdMap& getTileIndexTrackGroupMap(PipelineOutput aOutput);

        DashOmaf* getDash()
        {
            return mDash.get();
        }

        const DashOmaf* getDash() const
        {
            return mDash.get();
        }

        SegmentDurations makeSegmentDurations(const VideoGOP& aGopInfo, Optional<FrameDuration> aCompleteDuration) const;

        std::list<EntityIdReference> resolveRefIdLabels(
            std::list<ParsedValue<RefIdLabel>> aRefIdLabels);

    protected:
        std::shared_ptr<VDD::Config> mConfig;
        std::shared_ptr<Log> mLog;

        bool mParallel;
        std::unique_ptr<GraphBase> mGraph;
        ThreadedWorkPool mWorkPool;
        Optional<std::string> mDotFile;

        /* Graph ops lifting the shared functionality. unique_ptr so we don't need to include the
         * header. */
        std::unique_ptr<ControllerOps> mOps;

        StreamId mFakeAdaptationSetIdForMP4 { 1 };

        EntityGroupReadContext mEntityGroupReadContext;

        std::unique_ptr<DashOmaf> mDash; // not set = dash not enabled
        Optional<SegmentDurations> mDashDurations;

        /* Dump the processing graph every mDumpDotInterval seconds */
        Optional<double> mDumpDotInterval;

        std::map<PipelineOutput, TileIndexTrackGroupIdMap> mTileIndexMap;

        /** Builds the pipeline from a raw frame source (ie. import or tiling or audio) to saving
            segments. If there is no video encoder config, the video encoder is skipped and
            pipeline is built for raw data (ie AAC).

            @param aView Pointer to the View, if any
            @param aDashOutput The Dash output configuration for this stream
            @param aPipelineOutput What kind of output is this? Mono, Left, Audio, etc
            @param aSources Which source to capture the initial frames from (raw import, aac import, or tiler)
            Uses aSegmenterOutput to choose the correct source from here.
            @param aMP4VRSink If this pipeline (the part before segmenter) is to be written to
            an MP4VR file, provide a writer sink

            @return Return the end of the pipeline, suitable for using with configureMPD
        */
        virtual PipelineInfo buildPipeline(View* aView, Optional<DashSegmenterConfig> aDashOutput,
                                           PipelineOutput aPipelineOutput,
                                           PipelineOutputNodeMap aSources,
                                           AsyncProcessor* aMP4VRSink,
                                           Optional<PipelineBuildInfo> aPipelineBuildInfo) = 0;

        /** @brief Builds the dash-pipeline from given aac input. Used by
         * makeAudio its last stage */
        virtual Optional<AudioDashInfo> makeAudioDashPipeline(View* aView,
                                                              const ConfigValue& aDashConfig,
                                                              std::string aAudioName,
                                                              AsyncNode* aAacInput,
                                                              FrameDuration aTimeScale,
                                                              Optional<PipelineBuildInfo> aPipelineBuildInfo);

        friend class ControllerConfigureForwarder;
    };

}
