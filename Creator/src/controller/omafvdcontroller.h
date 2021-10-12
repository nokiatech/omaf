
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
#include <map>
#include <mutex>
#include <tuple>
#include <ctime>

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
#include "config/config.h"
#include "omaf/tileproducer.h"
#include "omaf/tileproxy.h"
#include "omaf/tileconfigurations.h"

namespace VDD
{
    class Log;

    class ControllerConfigureForwarderForView;

    class ControllerOps;

    class ConfigValueWithFallback;

    class MediaStepLock;

    class View;

    struct InputVideo
    {
        Optional<VideoRole> role;
        ParsedValue<MediaInputConfig> config;
        std::string label;
        std::uint8_t quality;
        std::uint32_t width;
        std::uint32_t height;
        int ctuSize;
        Optional<size_t> frameLimit;
        Optional<FrameDuration> duration; // if known; usually is?

        // abr index == 0u -> should be the highest quality (biggest frames) stream of its role
        // other indices don't matter
        size_t abrIndex = 0u;

        // which is the primary input video for this output? index to the mInputVideos
        size_t primaryInputVideoIndex = 0u;

        // In DASH, tile configs specify adaptation sets, and inputs specify Representations. Hence
        // we need to iterate them independently and can't store the configs to input
        TileProducer::Config mTileProducerConfig;
        ParsedValue<MediaInputConfig> mInputConfig;
        AsyncNode* mMediaSource = nullptr;
        AsyncProcessor* mTileProducer = nullptr;

        /** @brief Retrieves the set of stream ids produced by this video source by looking inside
         * mTileProducer */
        std::set<StreamId> getStreamIds() const;
    };

    using InputVideos = std::vector<InputVideo>;

    FrameDuration getLongestVideoDuration(const InputVideos&);

    struct ConfigureOutputForVideo
    {
        VDMode vdMode;
        FrameDuration videoTimeScale;
        VideoGOP mGopInfo;
        InputVideos mInputVideos;
        std::function<size_t(void)> newMpdViewId;
        std::shared_ptr<TileProxyConnector> mTileProxyDash;
    };

    struct ConfigureOutputValue
    {
        StreamId adaptationSetId;
        RepresentationId representationId;
        TrackId trackId; // aka entity_id for some purposes
    };

    struct ConfigureOutputInfo
    {
        std::list<ConfigureOutputValue> dashIds;
    };

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

        MP4VROutputs& getMP4VROutputs();

        /** @brief Convert InputVideo parameters to a tile */
        static TileConfigurations::TiledInputVideo tiledVideoInputOfInputVideo(
            const InputVideo& aInputVideo);

        TrackGroupId newTrackGroupId();

        // note: adds the configuration to mInputVideos
        static std::tuple<std::string, Optional<VideoRole>> readLabelRole(const VDD::ConfigValue& aVideoInputConfigurationValue);

        /** @brief Validates (by throwing ConfigValueInvalid on failure) by comparing dimensions etc */
        static void validateABRVideoConfig(const InputVideo& aReferenceVideo, const InputVideo& aCurrentVideo, const ConfigValue& aVideoConfig);

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
        PipelineInfo buildPipeline(View* aView,
                                   Optional<DashSegmenterConfig> aDashOutput,
                                   PipelineOutput aPipelineOutput, PipelineOutputNodeMap aSources,
                                   AsyncProcessor* aMP4VRSink,
                                   Optional<PipelineBuildInfo> aPipelineBuildInfo) override;

        /** @brief configureOutputForVideo
         *
         * @param aExtractorStreamAndTrackForMP4VR is used only for MP4 output and only in the case there are is no "tile" option for the output
         */
        ConfigureOutputInfo configureOutputForVideo(
            const ConfigureOutputForVideo& aConfig, View& aView, PipelineOutput aOutput,
            PipelineMode aPipelineMode,
            std::function<std::unique_ptr<TileProxy>(void)> aTileProxyFactory,
            std::pair<size_t, size_t> aTilesXY, TileProxy::Config aTileProxyConfig,
            Optional<StreamAndTrack> aExtractorStreamAndTrackForMP4VR,
            Optional<PipelineBuildInfo> aPipelineBuildInfo);

        /** Access the entity group read context */
        const EntityGroupReadContext& getEntityGroupReadContext() const;

        /** Merge new entity mappings to global mappings */
        void addToLabelEntityIdMapping(const LabelEntityIdMapping& aMapping);

        /** Allow postponing operations until a given time */
        void postponeOperation(PostponeTo aToPhase, std::function<void(void)> aOperation);

    private:
        std::list<View> mViews;

        /** @brief Given an pipeline mode and base layer name, find an existing MP4VR producer for
        * it, or create one if it is missing and MP4 generation is enabled. Arranges left and
        * right outputs of VideoSeparate to end up in the same MP4VRWriter.
        *
        * @return nullptr if mp4 output is not enabled, otherwise a pointer to an MP4VRWriter */
        MP4VRWriter* createMP4Output(PipelineMode aMode, std::string aName);

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

        /** @brief Process entity groups */
        void makeEntityGroups();

        void makeTimedMetadata();

    private:
        GraphErrors mErrors;

        bool mParseOnly; // only parse the config, don't run the graph

        Optional<MP4VRWriter::Config> mMP4VRConfig;
        MP4VROutputs mMP4VROutputs;
        std::list<std::function<void(void)>> mPostponedOperations;
        Optional<uint32_t> mInitialViewpointId;

        // placeholders for adding intermediate configuration boxes just before segmenter
        std::map<View*, std::list<AsyncProcessor*>> mMediaPipelineEndPlaceholders;

        // Must be set before starting the pipeline
        Promise<ISOBMFF::Optional<StreamSegmenter::Segmenter::MetaSpec>> mFileMeta;

        friend class ControllerConfigureForwarderForView;
    };

    class ControllerConfigureForwarderForView : public ControllerConfigure
    {
    public:
        ControllerConfigureForwarderForView(ControllerConfigureForwarderForView& aSelf);
        ControllerConfigureForwarderForView(OmafVDController& aController);
        ControllerConfigureForwarderForView(OmafVDController& aController, View& aView);

        ~ControllerConfigureForwarderForView() override;

        bool isView() const override;

        /** @brief Generate a new adaptation set id */
        StreamId newAdaptationSetId() override;

        /** @brief Generate a new track id */
        TrackId newTrackId() override;

        /** @brief Set the input sources and streams */
        void setInput(AsyncNode* aInputLeft, AsyncNode* aInputRight, VideoInputMode aInputVideoMode,
                      FrameDuration aFrameDuration, FrameDuration aTimeScale,
                      VideoGOP aGopInfo) override;

        /** @brief Is the Controller in mpd-only-mode? */
        bool isMpdOnly() const override;

        /** @brief Retrieve frame duration */
        FrameDuration getFrameDuration() const override;

        /** @see OmafVDController::getDashDurations */
        SegmentDurations getDashDurations() const override;

        /** @brief Builds the audio dash pipeline */
        Optional<AudioDashInfo> makeAudioDashPipeline(const ConfigValue& aDashConfig,
                                                      std::string aAudioName, AsyncNode* aAacInput,
                                                      FrameDuration aTimeScale,
                                                      Optional<PipelineBuildInfo> aPipelineBuildInfo) override;

        /** @see OmafVDController::buildPipeline */
        PipelineInfo buildPipeline(Optional<DashSegmenterConfig> aDashOutput,
                                   PipelineOutput aPipelineOutput, PipelineOutputNodeMap aSources,
                                   AsyncProcessor* aMP4VRSink,
                                   Optional<PipelineBuildInfo> aPipelineBuildInfo) override;

        ViewId getViewId() const override;

        Optional<ViewLabel> getViewLabel() const override;

        std::list<AsyncProcessor*> getMediaPlaceholders() override;

        Optional<Future<Optional<StreamSegmenter::Segmenter::MetaSpec>>> getFileMeta() const override;

        void postponeOperation(PostponeTo aToPhase, std::function<void(void)> aOperation) override;

        std::list<EntityIdReference> resolveRefIdLabels(std::list<ParsedValue<RefIdLabel>> aRefIdLabels) override;

        ControllerConfigureForwarderForView* clone() override;

    private:
        OmafVDController& mController;
        View* mView; // might be null
    };
}  // namespace VDD
