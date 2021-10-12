
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
#include <mutex>

#include <streamsegmenter/segmenterapi.hpp>
#include <streamsegmenter/mpdtree.hpp>

#include "common.h"

#include "async/combinenode.h"
#include "async/future.h"
#include "common/utils.h"
#include "config/config.h"
#include "omaf/tileconfig.h"
#include "segmenter/save.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/singlefilesave.h"

namespace VDD
{
    class AsyncNode;
    class ControllerOps;

    typedef std::string DashSegmenterName;

    enum class DashProfile {
        Live,
        OnDemand
    };

    struct DashSegmenterConfig
    {
        Optional<ViewId> viewId; // used for hookSegmentSaverSignal. optional because currently we're not using views (omafvi).
        uint32_t adaptationSetPriority;
        DashSegmenterName segmenterName;
        RepresentationId representationId;
        SegmentDurations durations;
        SegmenterAndSaverConfig segmenterAndSaverConfig;
        SegmenterInit::Config segmenterInitConfig;

        // used for determining which segmenter produced a particular segment to MPD writer
        Optional<size_t> segmenterSinkIndex;

        // Only set when writing individual segment files, not in on-demand profile when writing to a
        // single file
        Optional<Save::Config> segmentInitSaverConfig;

        // Only set when writing to one file in on demand profile
        Optional<SingleFileSave::Config> singleFileSaverConfig;
    };

    enum class StoreContentComponent
    {
        TrackId
    };

    struct RepresentationConfig
    {
        Optional<int> videoBitrate;
        DashSegmenterConfig output;
        std::list<std::string> associationType;
        std::list<RepresentationId> associationId;
        Optional<ViewId> allMediaAssociationViewpoint;
        Optional<StoreContentComponent> storeContentComponent;
    };

    struct Representation
    {
        // in case of encoder variants, this is the first of them. it is used for retrieving the
        // frame dimensions or audio configuration.
        AsyncNode* encodedFrameSource;

        RepresentationConfig representationConfig;
    };

    struct AdaptationConfig
    {
        StreamId adaptationSetId;
        Optional<StreamSegmenter::MPDTree::OmafProjectionFormat> projectionFormat;
        std::list<Representation> representations;
        Optional<std::list<StreamId>> overlayBackground;
        Optional<std::string> stereoId;

        // set after video initialization; after determining if there is viewpoint metadata
        Optional<StreamSegmenter::MPDTree::Omaf2Viewpoint> viewpoint;
    };

    // Used for MPD conversions
    Optional<StreamSegmenter::MPDTree::OmafProjectionFormat>
    mpdProjectionFormatOfOmafProjectionType(Optional<OmafProjectionType> aProjection);

#if defined(__GNUC__)
    // According to GCC, IdKind::TrackId shadows TrackId class; I disagree
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
    enum class IdKind
    {
        NotExtractor,
        Extractor,
        TrackId
    };
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

    class Dash
    {
    public:
        /** Requires (asserts) aDashConfig to be a valid JSON configuration */
        Dash(std::shared_ptr<Log> aLog,
            const ConfigValue& aDashConfig,
            ControllerOps& aControllerOps);
        virtual ~Dash();

        /** Hook the given node as an input to the signal that drives MPD writing for a particular
         * DASH segmenter. Typically one would hook the output of a node doing the writing of the
         * segments to this. */
        virtual void hookSegmentSaverSignal(const DashSegmenterConfig& aDashOutput, AsyncProcessor* aNode);

        /** @brief Retrieve the Dash configuration key for a particular name */
        virtual ConfigValue dashConfigFor(std::string aDashName) const;

        /** @brief Allocates a new adaptation id; starts from 1000. */
        StreamId newId(IdKind aKind = IdKind::NotExtractor);

        static void createHevcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec);
        static void createAvcCodecsString(const std::vector<uint8_t>& sps, std::string& aCodec);

        /** @brief Retrieve the base directory prefixing all outputs; may be empty */
        std::string getBaseDirectory() const;

        /** Set the base name component for the generated MPD file. Must be called before calling DashOmaf::setupMpd. */
        void setBaseName(std::string aBaseName);

        /** Retrieve the value set with setBaseName. Will fail to an assert if base name is not set. */
        std::string getBaseName() const;

        // Used for implementing some aspects that use the same configuration as DashOmaf..
        const ConfigValue& getConfig() const;

        /** @brief Retrieve the curren opreating mode set by the "mode" field */
        OutputMode getOutputMode() const;

        /** @brief Creates a new imda id for a pipeline output. Protected by a mutex, as this may be
            called from multiple threads. */
        StreamSegmenter::Segmenter::ImdaId newImdaId(PipelineOutput aOutput);

        /** @brief Retrieve current Dash profile */
        DashProfile getProfile() const;

        /** @breif Returns the (fixed) default period id; goes away when we're going to support multiple periods */
        std::string getDefaultPeriodId() const;

    protected: 
        ControllerOps& mOps;
        std::shared_ptr<Log> mLog;
        ConfigValue mDashConfig;
        // mMpdConfigMutex protects only the bits of mMpdConfig that are accessed from multiple
        // threads. In particular the adaptationSets that is written to from all segment nodes.
        std::mutex mMpdConfigMutex;
        Optional<StreamSegmenter::Segmenter::Duration> mOverrideTotalDuration;

        bool mCompactNumbering; // Use small numbers for numering adaptation sets etc

        /* Nodes for each used dash segmenter (indexed by presentation id) that combines all segment
        * saves to signal MPD that the segment has been written and it's ok to proceed in writing
        * the MPD. */
        struct SegmentSaverSignal {
            std::unique_ptr<CombineNode> combineSignals;
            size_t sinkCount = 0;
            StreamSegmenter::Segmenter::Duration segmentsTotalDuration = { 0, 1 };
        };

        // optional key as omafvi doesn't use views
        std::map<Optional<ViewId>, std::map<DashSegmenterName, SegmentSaverSignal>> mSegmentSavedSignals;

        std::mutex mImdaIdMutex;
        StreamSegmenter::Segmenter::ImdaId mImdaId = {10000};

        /* MPD output file name */
        std::string mMpdFilename;

        /* Protects writeMpd */
        std::mutex mMpdWritingMutex;

        std::string mBaseDirectory;

        Optional<std::string> mBaseName;

        StreamId mNextId;
        Optional<IdKind> mPrevIdKind;

        // Used for mapping MPD writer inputs to presentation ids for the purpose of updating bitrates
        std::map<Optional<ViewId>, std::map<size_t, std::string>> mSinkIndexToRepresentationId;

        DashProfile mDashProfile = DashProfile::Live;

        OutputMode mOutputMode = OutputMode::None;
    };

}
