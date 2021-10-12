
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

#include "controllerops.h"
#include "omafvdcontroller.h"
#include "common/lazy.h"

namespace VDD
{
    const auto readViewId = mapConfigValue<std::uint32_t, ViewId>(readUint32, [](const std::uint32_t& id) { return ViewId(id); } );

    const auto readViewLabel = mapConfigValue<std::string, ViewLabel>(
        readRegexValidatedString(std::regex(R"(^[a-zA-Z0-9]+$)"), "Expected id matching [a-zA-Z0-9]"),
        [](const std::string& id) { return ViewLabel(id); });

    class View
    {
    public:
        static constexpr StreamId cExtractorMoofStreamId = {0};
        static constexpr StreamId cTilesMoofStreamId = {1};
        static constexpr StreamId cCombinedSidxStreamId = {2};

        View(OmafVDController& aController, GraphBase& aGraph, const ControllerOps& aOps,
             const VDD::ConfigValue& aConfig, std::shared_ptr<Log> aLog, ViewId aViewpointId,
             ViewpointGroupId aViewpointGroupId);
        ~View();

        // Once we make a Viewpoint, we don't want to copy or move it, ever
        View(View&&) = delete;
        View(const View&) = delete;
        void operator=(const View&) = delete;
        void operator=(View&&) = delete;

        // Retrieve the id of this view; can only be unset if there is exactly one view
        Optional<ViewLabel> getLabel() const;

        // Retrieve the viewpoint group id
        ViewId getId() const;

        // Retrieve the viewpoint id
        ViewpointGroupId getViewpointGroupId() const;

        // Retrieve the JSON configuration
        const ConfigValue& getConfig() const;

        /** @brief Retrieve the contents of mInputVideos but so that ones with abrIndex == 0u are
         * listed before others.
         */
        std::vector<std::reference_wrapper<InputVideo>> getInputVideosPrimaryFirst();

        void associateTileProducer(InputVideo& aVideo, ExtractorMode aExtractor);

        // aSequenceStep collects the length of the shortest video. Then you need to call all
        // the returned functions with that value.
        void setupMultiQSubpictureAdSets(const ConfigValue& aDashConfig,
                                         std::list<StreamId>& aStreamSetIds,
                                         std::list<StreamId>& aDashAdSetIds,
                                         SegmentDurations& aDashDurations, PipelineOutput aOutput,
                                         std::pair<size_t, size_t> aTilesXY,
                                         StreamSegmenter::Segmenter::SequenceId& aSequenceBase,
                                         StreamSegmenter::Segmenter::SequenceId& aSequenceStep);

        // aSequenceStep collectes the length of the shortest video. Then you need to call all
        // the returned functions with that value.
        void setupMultiResSubpictureAdSets(const ConfigValue& aDashConfig,
                                           std::list<StreamId>& aStreamSetIds,
                                           std::list<StreamId>& aDashAdSetIds,
                                           SegmentDurations& aDashDurations, PipelineOutput aOutput,
                                           StreamSegmenter::Segmenter::SequenceId& aSequenceBase,
                                           StreamSegmenter::Segmenter::SequenceId& aSequenceStep);

        /** Produce an end-report. Currently says the number of frames processed per second from
            one input source. */
        void report() const;

        const std::set<TrackId>& getVideoTrackIds() const;
        const std::set<TrackId> getMediaTrackIds() const;
        void addVideoTrackId(TrackId aTrackId);
        void addAudioTrackId(TrackId aTrackId);

        const std::list<RepresentationId>& getVideoRepresentationIds() const;
        const std::list<RepresentationId> getMediaRepresentationIds() const;

        void addVideoRepresentationId(RepresentationId aRepresentationId);
        void addAudioRepresentationId(RepresentationId aRepresentationId);
        void resetVideoRepresentationIds(const std::list<RepresentationId>& aRepresentationIds);

        void addMainVideoAdaptationId(StreamId aAdaptationId);
        void addVideoAdaptationId(StreamId aAdaptationId);
        void addAudioAdaptationId(StreamId aAdaptationId);

        const std::list<StreamId>& getMainVideoAdaptationIds() const;
        const std::list<StreamId>& getVideoAdaptationIds() const;
        const std::list<StreamId> getMediaAdaptationIds() const;

        Projection getProjection() const;

        /** Pipe this pipeline to be combined */
        void combineMoof(const PipelineInfo& aPipelineInfo,
                         const PipelineBuildInfo& aPipelineBuildInfo);

        /** Handle the final combining of Moofs */
        void handleMoofCombine(PipelineOutput aOutput);

        /** Retrieve the Index Segment stream id */
        StreamId getIndexStreamId() const;

        /** Adds a new OMAFv2 extractor; creates structures for synchronizing tiles and extractors
         * together, to construct the base segment
         *
         * @returns a source to be connected to the saver */
        AsyncSource* addExtractor(AsyncProcessor* extractor);

    private:
        /** @brief Creates the video passthru pipeline.
         */
        void makeVideo();

        /** @brief Create the overlays
         */
        void makeOverlays();

        /** @brief Create the viewpoints
         */
        void makeViewpointMeta(const ConfigValue& aConfigValue);

        /** @brief
         *
         * @param aTrackIdBase is used for to arrange different resolutions to use the same
         * track numbering due to extractors
         */
        InputVideos::iterator readInputVideoConfig(const VDD::ConfigValue& aVideoInputConfiguration,
                                                   Optional<size_t> aFrameLimit,
                                                   std::pair<size_t, size_t>& aTiles,
                                                   std::string aLabel, Optional<VideoRole> aRole,
                                                   TrackIdProvider aTrackIdProvider,
                                                   TrackGroupIdProvider aTrackGroupIdProvider,
                                                   StreamIdProvider aStreamIdProvider);

        void readCommonInputVideoParams(const VDD::ConfigValue& aValue,
                                        Optional<size_t>& aFrameCount,
                                        Optional<FrameDuration>& aOverrideFrameDuration);

        /** @brief For primary streams (abrIndex == 0u) return the appropriate (only) TileProxy;
         * for other streams return the actual stream */
        AsyncNode* getInputVideoOutputSource(const InputVideo& aInputVideo);

        /** @brief 5K video roles can only be determined automatically they have all been read;
         * assign roles here
         *
         * @param aInputVideoConfigs used for proper error signaling */
        void assign5KVideoRoles(const ConfigValue& aInputVideoConfigs);

        /** @brief For the given video roles find the appropriate video inputs and convert their
         * configuration to a format appropriate for TileConfigurations::create* functions
         *
         * @param aCrop used to call TileConfigurations::crop* functions to crop tile
         * configuration for each relevant input
         */
        std::map<VideoRole, TileConfigurations::TiledInputVideo> pickTileConfigurations(
            const std::set<VideoRole>& aRequiredRoles,
            const std::map<VideoRole, OmafTileSets (*)(OmafTileSets& aTileConfig,
                                                       VideoInputMode aInputVideoMode)>& aCrop);

        /** @brief Find all input videos for a role */
        std::vector<std::reference_wrapper<InputVideo>> findVideosForRole(
            const VideoRole& aRole);

        /** @brief Given a role, fints its primary video input or throw a ConfigValueReadError if no
         * such input found */
        InputVideo& findPrimaryVideoForRole(const VideoRole& aRole);

        /** @brief If we have eliminated tiles by cropping, we may renumber tracks to avoid gaps */
        void rewriteIdsForTiles(InputVideo& aVideo, const OmafTileSets& aRemovedTiles);

        /** @brief Copy the track ids from parent tiles for ABR streams */
        void copyTrackIdsForABR();

        std::unique_ptr<ControllerConfigure> getConfigure();

        OmafVDController& mController;
        GraphBase& mGraph;
        ControllerOps mOps;

        ViewId mId;
        Optional<ViewLabel> mLabel;
        ViewpointGroupId mViewpointGroupId;

        ConfigValue mConfig;

        std::shared_ptr<Log> mLog;

        std::unique_ptr<MediaStepLock>
            mVideoStepLock;  // limit all vidoe input files to the frame number of frames

        VDMode mVDMode;

        VideoInputMode mVideoInputMode = VideoInputMode::Mono;

        AsyncNode* mInputLeft = nullptr;
        AsyncNode* mInputRight = nullptr;

        VideoInputMode mInputVideoMode;
        FrameDuration mVideoFrameDuration;
        FrameDuration mVideoTimeScale;
        VideoGOP mGopInfo;

        clock_t mBeginTime;
        clock_t mEndTime;
        std::mutex mFramesMutex;
        std::map<size_t, int> mFrames;  // one frame counter per mInputVideos index

        InputVideos mInputVideos;
        Optional<FrameDuration> mOverrideFrameDuration;

        Optional<ParsedValue<RefIdLabel>> mVideoRefIdLabel;

        std::shared_ptr<TileProxyConnector>
            mTileProxyDash;

        // Used for combining Moofs and Extractor tracks into the same segmenter
        struct Extractor {
            AsyncProcessor* extractor;

            // combine two inputs:
            // - extractor segmenter sidx output
            // - extractor imda output
            // which will be used for generating sidx in on-demand case
            std::shared_ptr<CombineNode> forSidxBuilder;
            AsyncProcessor* extractorForSidxSink; // not always available
            AsyncProcessor* tilesMoofForSidxSink; // not always available
            AsyncSource* forSidxBuilderSource;

            // combines two segmenter outputs (extractor moof and imda),
            // the tiles moof and the output from SidxAdjuster
            std::shared_ptr<CombineNode> outputCombiner;
            AsyncProcessor* extractorMoofSink;
            AsyncProcessor* tilesMoofSink;
            AsyncProcessor* combinedSidxSink; // not always available
            AsyncSource* combinedSource;
        };

        std::list<Extractor> mExtractors;

        struct MoofOutput
        {
            CombineNode node;
            size_t count;

            // used to writer extractors before media in moof
            std::vector<StreamId> extractor;
            std::vector<StreamId> media;

            // used for combining segments written for extractor and moofs
            AsyncProcessor* segmentSink;
        };

        // Used for combinine the moof boxes from all for the segment index
        std::map<PipelineOutput, MoofOutput> mMoofOutput;

        AsyncProcessor* getMoofOutput(const PipelineBuildInfo& aPipelineBuildInfo);

        // Finds a frame source for providing synchronization for some other resource (like
        // generated metadata)
        AsyncNode* getSyncFrameSource();

        /* Incremented to provide unique identifiers for each left/right pair. Actually
         * incremented a bit too eagerly, but its only downside is the gaps in the identifiers.
         */
        size_t mMpdViewCount = 0;
        // note: this really works only for single MP4VR output scenario, in the other scenario it
        // works by accident, because all video track ids are the same
        std::set<TrackId> mVideoTrackIds;
        std::set<TrackId> mAudioTrackIds;

        Projection mProjection;

        // Collect a list of video presentation ids, so we can create the invo metadata and
        // viewpoint track referencing them
        std::list<RepresentationId> mVideoRepresentationIds;
        std::list<RepresentationId> mAudioRepresentationIds;

        // Collect a list of video or extractor adaptation ids (==stream id) for viewpoints
        std::list<StreamId> mVideoAdaptationIds;

        // Collect a list of video or extractor adaptation ids (==stream id) for overlay
        std::list<StreamId> mMainVideoAdaptationIds;

        // Collect a list of video, extractor, or audio adaptation ids (==stream id) for handling viewpoints
        std::list<StreamId> mAudioAdaptationIds;
        

        Lazy<StreamId> mIndexAdaptationSetId; // used for OMAFv2

        friend class ControllerConfigureForwarderForView;
    };
}  // namespace VDD