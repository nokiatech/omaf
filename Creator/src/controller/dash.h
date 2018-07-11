
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

#include <streamsegmenter/segmenterapi.hpp>

#include "common.h"

#include "common/utils.h"
#include "async/combinenode.h"
#include "async/future.h"
#include "config/config.h"
#include "segmenter/segmenter.h"
#include "segmenter/segmenterinit.h"
#include "segmenter/save.h"

namespace VDD
{
    class AsyncNode;
    class ControllerOps;

    typedef std::string DashSegmenterName;

    enum class PipelineOutput : int;

    struct DashSegmenterConfig
    {
        uint32_t adaptationSetPriority;
        DashSegmenterName segmenterName;
        std::string presentationId;
        SegmentDurations durations;
        SegmenterAndSaverConfig segmenterAndSaverConfig;
        SegmenterInit::Config segmenterInitConfig;
        Save::Config segmentInitSaverConfig;
    };

    struct RepresentationConfig
    {
        Optional<int> videoBitrate;
        DashSegmenterConfig output;
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

        std::list<Representation> representations;
    };

    class Dash
    {
    public:
        Dash(std::shared_ptr<Log> aLog,
            const ConfigValue& aDashConfig,
            ControllerOps& aControllerOps);
        virtual ~Dash();

        /** Hook the given node as an input to the signal that drives MPD writing for a particular
         * DASH segmenter. Typically one would hook the output of a node doing the writing of the
         * segments to this. */
        virtual void hookSegmentSaverSignal(const DashSegmenterConfig& aDashOutput, AsyncProcessor* aNode);

        /** @brief Is Dash enabled at all? Ie. does the configuration "dash" exist. */
        virtual bool isEnabled() const;
        /** @brief Retrieve the Dash configuration key for a particular name */
        virtual ConfigValue dashConfigFor(std::string aDashName) const;


        static void createHevcCodecsString(const std::vector<uint8_t>& aSps, std::string& aCodec);
        static void createAvcCodecsString(const std::vector<uint8_t>& sps, std::string& aCodec);

    protected: 
        ControllerOps& mOps;
        std::shared_ptr<Log> mLog;
        ConfigValue mDashConfig;
        // mMpdConfigMutex protects only the bits of mMpdConfig that are accessed from multiple
        // threads. In particular the adaptationSets that is written to from all segment nodes.
        std::mutex mMpdConfigMutex;
        Optional<StreamSegmenter::Segmenter::Duration> mOverrideTotalDuration;

        /* Nodes for each used dash segmenter (indexed by presentation id) that combines all segment
        * saves to signal MPD that the segment has been written and it's ok to proceed in writing
        * the MPD. */
        struct SegmentSaverSignal {
            std::unique_ptr<CombineNode> combineSignals;
            size_t sinkCount = 0;
            StreamSegmenter::Segmenter::Duration segmentsTotalDuration = { 0, 1 };
        };
        std::map<DashSegmenterName, SegmentSaverSignal> mSegmentSavedSignals;

        /* MPD output file name */
        std::string mMpdFilename;

        /* Protects writeMpd */
        std::mutex mMpdWritingMutex;
    };

}
