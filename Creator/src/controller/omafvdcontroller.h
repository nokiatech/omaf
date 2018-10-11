
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
#include <map>
#include <mutex>

#include "audio.h"
#include "common.h"
#include "dashomaf.h"
#include "mp4vrwriter.h"

#include "controllerbase.h"

#include "async/asyncnode.h"
#include "async/graph.h"
#include "async/combinenode.h"
#include "common/utils.h"
#include "common/optional.h"
#include "concurrency/threadedpoolprocessor.h"
#include "config/config.h"
#include "omaf/tileproducer.h"


namespace VDD
{
    class Log;

    class ControllerConfigure;

    class ControllerOps;

    class ConfigValueWithFallback;

    class OmafVDController : public ControllerBase
    {
    public:
        OmafVDController(Config aConfig);
        OmafVDController(const OmafVDController&) = delete;
        OmafVDController& operator=(const OmafVDController&) = delete;

        ~OmafVDController();
        void run();

        /** @brief moves errors to the client. Can only be called when 'run' is not being run. */
        GraphErrors moveErrors();

    protected:
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
        AsyncNode* buildPipeline(std::string aName,
            Optional<DashSegmenterConfig> aDashOutput,
            PipelineOutput aPipelineOutput,
            PipelineOutputNodeMap aSources,
            AsyncProcessor* aMP4VRSink);

    private:

        struct InputVideo
        {
            std::string filename;
            std::string label;
            std::uint8_t quality;
            std::uint32_t width;
            std::uint32_t height;
            int ctuSize;

            // In DASH, tile configs specify adaptation sets, and inputs specify Representations. Hence we need to iterate them independently and can't store the configs to input
            TileProducer::Config mTileProducerConfig;
            AsyncSourceWrapper* mTileProducer = nullptr;
        };

        enum VDMode
        {
            Invalid = -1,
            MultiQ,
            MultiRes5K,
            MultiRes6K,
            COUNT
        };

        void readCommonInputVideoParams(const VDD::ConfigValue& aValue, Projection& aProjection, uint32_t& aFrameCount);
        InputVideo& readInputVideoConfig(const VDD::ConfigValue& aVideoInputConfiguration, uint32_t aFrameCount, std::pair<int, int>& aTiles);
        void associateTileProducer(InputVideo& aVideo, ExtractorMode aExtractor, Projection& aProjection);
        void rewriteIdsForTiles(InputVideo& aVideo);

        /** @brief Given an pipeline mode and base layer name, find an existing MP4VR producer for
        * it, or create one if it is missing and MP4 generation is enabled. Arranges left and
        * right outputs of VideoSeparate to end up in the same MP4VRWriter.
        *
        * @return nullptr if mp4 output is not enabled, otherwise a pointer to an MP4VRWriter */
        MP4VRWriter* createMP4Output(PipelineMode aMode, std::string aName);

        /** @brief Creates the video passthru pipeline.
        */
        void makeVideo();

        void setupMultiQSubpictureAdSets(const ConfigValue& aDashConfig, std::list<StreamId>& aAdSetIds, SegmentDurations& aDashDurations, PipelineOutput aOutput, std::pair<int, int> aTilesXY);

        void setupMultiResSubpictureAdSets(const ConfigValue& aDashConfig, std::list<StreamId>& aAdSetIds, SegmentDurations& aDashDurations, PipelineOutput aOutput);

        /** @brief Reads keys in configuration under the "debug" and does some operations based on
        * them.
        *
        * Exporting output of a node: "debug": { "raw_export": { "5" : "foo" } }
        * or from command line: --debug.raw_export.3=debug
        **/
        void setupDebug();

        /** @brief Writes the dot file describing the graph, if enabled by { "debug" { "dot*:
        * "foo.dot" } } (maps to mDotFile) */
        void writeDot();

    private:
        std::shared_ptr<Log> mLog;
        std::shared_ptr<VDD::Config> mConfig;

        bool mParallel;
        std::unique_ptr<GraphBase> mGraph;
        ThreadedWorkPool mWorkPool;
        Optional<std::string> mDotFile;

        std::mutex mFramesMutex;
        std::map<std::string, int> mFrames; // one frame counter per input file label

        std::vector<InputVideo> mInputVideos;

        AsyncProcessorWrapper* mTileProxyDash = nullptr;
        AsyncProcessorWrapper* mTileProxyMP4 = nullptr;

        GraphErrors mErrors;

        /* Graph ops lifting the shared functionality. unique_ptr so we don't need to include the
         * header. */
        std::unique_ptr<ControllerOps> mOps;

        DashOmaf mDash;

        /* Incremented to provide unique identifiers for each left/right pair. Actually
        * incremented a bit too eagerly, but its only downside is the gaps in the identifiers. */
        size_t mMpdViewCount = 0;

        /* Lower level configuration operations (ie. access to mInputLeft). */
        std::unique_ptr<ControllerConfigure> mConfigure;

        Optional<MP4VRWriter::Config> mMP4VRConfig;
        MP4VROutputs mMP4VROutputs;

        StreamId mNextStreamId = 1;

        VDMode mVDMode;

        VideoInputMode mVideoInputMode;

        std::string mDashBaseName;

        friend class ControllerConfigure;
    };
}
