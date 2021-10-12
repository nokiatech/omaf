
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

#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/mpdtree.hpp>


#include "common.h"

#include "dash.h"

#include "common/utils.h"
#include "async/combinenode.h"
#include "async/future.h"
#include "config/config.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/save.h"
#include "omaf/tileproducer.h"

namespace VDD
{
    RepresentationId representationIdForExtractorId(StreamId aExtractorId);

    struct DashSegmenterMetaConfig
    {
        SegmentDurations dashDurations;
        bool disableMediaOutput = false;
        FrameDuration timeScale;
        TrackId trackId;
        Optional<StreamId> streamId;
        size_t abrIndex = 0u;
        Optional<ISOBMFF::OverlayStruct> overlays;
        Optional<ViewId> viewId; // we may not have view id with omafvi
        Optional<ViewLabel> viewLabel;
        Optional<Future<ISOBMFF::Optional<StreamSegmenter::Segmenter::MetaSpec>>> fileMeta;
        std::map<std::string, std::set<TrackId>> trackReferences;
        Optional<FragmentSequenceGenerator> fragmentSequenceGenerator; // used for generating sequence numbers for OMAFv2
    };

    struct DashMPDConfig
    {
        PipelineOutput segmenterOutput;
        size_t mpdViewIndex = 0u;
        AdaptationConfig adaptationConfig;
        Optional<std::list<StreamId>> supportingAdSets;
        Optional<ISOBMFF::OverlayStruct> overlays;
        std::list<StreamId> baseAdaptationSetIds;
    };

    // Used for tagging Data to have a particular mimetype for the purpose of writing Dash data
    struct MimeTypeCodecs
    {
        std::string mimeType;
        Optional<std::string> codecs;
    };

    using MimeTypeCodecsTag = ValueTag<MimeTypeCodecs>;

    class DashOmaf : public Dash
    {
    public:
        DashOmaf(std::shared_ptr<Log> aLog,
             const ConfigValue& aDashConfig,
             ControllerOps& aControllerOps);
        virtual ~DashOmaf();

        /** @brief Implements the common part of MPD configuration. The stream-specific parts are
         * implemented by makeDashSegmenterConfig called from the three functions below.
         *
         * Youy must call setBaseName() before invoking this function
         */
        virtual void setupMpd(Optional<Projection> aProjection = Optional<Projection>());

        /** @brief Write MPD; no collection is done, just use current information
        */
        void writeMpd();

        /** @brief Find the Dash MPD config by given adaptation set id
         *
         * The value returned by this function can be mutated _before_ starting the pipeline.
         *
         * This is used to fill in later the viewpoint information
         */
        std::shared_ptr<DashMPDConfig> getDashMPDConfigByAdaptationsetId(StreamId aAdaptationSetId);

        /** called from buildPipeline to inform MPD writing about the stream
         *
         * @param aRawFrameSource the data immediately after encoding or importing encoded
         * data. This is used for retrieving stream parameters such as width/height in a way that
         * takes into account the effects of ie. scaler and tiler to the image size.
         * @param aAdaptationConfig is the configuration for the adaptation set
         */
        virtual void configureMPD(const DashMPDConfig& aDashMPDConfig);

        /** Given a JSON configuration and various other parameters, build the configuration for
        * segmenters and segmentinits. This is then fed to Controller::buildPipeline */
        DashSegmenterConfig makeDashSegmenterConfig(const ConfigValue& aConfig,
                                                    PipelineOutput aOutput,
                                                    const std::string& aNamePrefix,
                                                    DashSegmenterMetaConfig aMetaConfig);

        static void addAdditionalVideoTracksToExtractorInitConfig(
            TrackId aExtractorTrackId, SegmenterInit::Config& aInitConfig,
            StreamFilter& aStreamFilter, const OmafTileSets& aTileConfig, FrameDuration aTimeScale,
            const std::list<StreamId>& aAdSetIds, const PipelineOutput& aOutput);

        // also adds the inverse link from tiles to base adaptation set. tiles must exist first.
        void addPartialAdaptationSetsToMultiResExtractor(const TileDirectionConfig& aDirection, std::list<StreamId>& aAdSetIds, StreamId aBaseAdaptationId);

        // For each adaptation set id referenced by aAdSetIds, add the given base track to the list of base adaptation sets
        void addBasePreselectionToAdaptationSets(const std::list<StreamId>& aAdSetIds,
                                                 StreamId aBaseAdaptationId);

        /** Adds a new entity group */
        void addEntityGroup(StreamSegmenter::MPDTree::EntityGroup aGroup);

        void updateViewpoint(uint32_t aAdaptationSetId,
                             const StreamSegmenter::MPDTree::Omaf2Viewpoint aViewpoint);

        /** @brief Should this output be written as imda */
        bool useImda(PipelineOutput aOutput) const;
    private:
        StreamSegmenter::MPDTree::OmafMPDRoot mMpd;
        std::map<StreamId, std::shared_ptr<DashMPDConfig>> mDashMPDConfigs;

        // mEntityGroups are stored separately, as they are created before mMpd.period is set up
        std::list<StreamSegmenter::MPDTree::EntityGroup> mEntityGroups;

        // viewpoint info is stored separately, as they may be created before mMpd.period is set up
        std::map<std::uint32_t, StreamSegmenter::MPDTree::Omaf2Viewpoint> mViewpoints;

        // Find from which segmenters these streams originate from, check if they come with
        // segmenter bitrate information, and if so, update bitrate
        void updateBitrates(Optional<ViewId> aViewId, const Streams& aStreams);

        /** @brief Collect progress information from aStreams.front()
        * Protects for concurrency with mMpdWritingMutex
        */
        void collectDurations(Optional<ViewId> viewId, DashSegmenterName aDashSegmenterName, const Streams& aStreams);

        // Fill in missing data for overlays. Has a useless parameter aAllMeta so this model fits
        // all the other MPD configuration functions as well in future refactorings.
        void configureMPDOverlays(StreamSegmenter::MPDTree::OmafAdaptationSet& aAdaptationSet,
                                  std::shared_ptr<DashMPDConfig> aDashMPDConfig,
                                  const std::vector<std::vector<Meta>>& aAllMeta);

        void configureMPDWithMeta(std::shared_ptr<DashMPDConfig> aDashMPDConfig,
                                  const std::vector<std::vector<Meta>>& aAllMeta);

        // Doesn't do locking, doesn't write to mViewpoints
        void updateViewpointInternal(uint32_t aAdaptationSetId,
                                     const StreamSegmenter::MPDTree::Omaf2Viewpoint aViewpoint);

        RepresentationId disambiguateRepresentationId(RepresentationId aRepresentationId);
        std::string disambiguateOutputTemplate(std::string aTemplate);

        std::map<RepresentationId, int> mRepresentationIdCount; // used for generating unique representation ids when needed
        std::map<std::string, int> mOutputTemplateCount; // used for generating unique file names when needed
    };
}
