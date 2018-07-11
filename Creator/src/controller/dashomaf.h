
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

    class DashOmaf : public Dash
    {
    public:
        DashOmaf(std::shared_ptr<Log> aLog,
             const ConfigValue& aDashConfig,
             ControllerOps& aControllerOps);
        virtual ~DashOmaf();

        /** @brief Implements the common part of MPD configuration. The stream-specific parts are
         * implemented by makeDashSegmenterConfig called from the three functions below.
         */
        virtual void setupMpd();

        /** @brief Write MPD; collect progress information from aViews[0]
        * Protects for concurrency with mMpdWritingMutex
        */
        virtual void writeMpd(DashSegmenterName aPresentationId, const Views& aViews);

        /** called from buildPipeline to inform MPD writing about the stream
         *
         * @param aRawFrameSource the data immediately after encoding or importing encoded
         * data. This is used for retrieving stream parameters such as width/height in a way that
         * takes into account the effects of ie. scaler and tiler to the image size.
         * @param aAdaptationConfig is the configuration for the adaptation set
         * @param aVideoFrameDuration video frame duration; required for video, not used for other media
         */
        virtual void configureMPD(PipelineOutput aSegmenterOutput, size_t aMPDViewIndex,
                          const AdaptationConfig& aAdaptationConfig,
                          Optional<FrameDuration> aVideoFrameDuration,
                          Optional<std::list<StreamId>> aSupportingAdSets);

        /** Given a JSON configuration and various other parameters, build the configuration for
        * segmenters and segmentinits. This is then fed to Controller::buildPipeline */
        static DashSegmenterConfig makeDashSegmenterConfig(const ConfigValue& aConfig, PipelineOutput aOutput, const std::string& aName,
            const SegmentDurations& aDashDurations,
            bool aDisableMediaOutput,
            FrameDuration aTimeScale,
            TrackId& aTrackId,
            Optional<StreamId> aStreamId);

        static void addAdditionalVideoTracksToInitConfig(TrackId aExtractorTrackId, SegmenterInit::Config& aInitConfig, const TileFilter::OmafTileSets& aTileConfig, FrameDuration aTimeScale);

    private:
        StreamSegmenter::MPDTree::OmafMPDRoot mMpd;
    };

}
