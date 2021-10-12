
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

#include <map>
#include <functional>

#include <streamsegmenter/segmenterapi.hpp>

#include "pipelineoutput.h"
#include "pipelinemode.h"
#include "async/asyncnode.h"
#include "segmenter/segmenter.h"
#include "segmenter/save.h"

namespace VDD
{
    using ViewpointGroupId = StreamSegmenter::ExplicitIdBase<std::uint8_t, class ViewpointGroupIdTag>;
    using ViewId = StreamSegmenter::ExplicitIdBase<std::uint32_t, class ViewTag>;
    using ViewLabel = StreamSegmenter::ExplicitIdBase<std::string, class ViewLabelTag>;

    template <typename Sample>
    struct SampleTimeDuration
    {
        FrameTime time;          // first we read these from config
        FrameDuration duration;  // but then we derive these
        Sample sample;

        /** @brief fills in locally available information to the sample */
        CodedFrameMeta getCodedFrameMeta(Index aIndex, CodedFormat format) const;
    };

    /** @brief Find an optimal time scale given the fractional times and the optional GOP
        duration */
    template <typename Sample>
    FrameDuration timescaleForSamples(const std::vector<Sample>& aSamples,
                                      Optional<FrameTime::value> aGopDenominator);

    struct SegmentDurations
    {
        StreamSegmenter::Segmenter::Duration segmentDuration;
        size_t subsegmentsPerSegment;
        Optional<FrameDuration> completeDuration;
    };

    struct SegmenterAndSaverConfig
    {
        Segmenter::Config segmenterConfig;
        // Only set when writing individual segment files in live profile, not in on-demand profile
        // when writing to a single file
        Optional<Save::Config> segmentSaverConfig;
    };

    using RepresentationId = std::string;

    // Struct passed to buildPipeline and then eventually pased back to View::combineMoof
    struct PipelineBuildInfo
    {
        Optional<StreamId> streamId;
        PipelineOutput output;
        bool isExtractor = false;
        AsyncProcessor* segmentSink; // used for OMAFv2 to store both extractor and moof segments in one palce
        bool connectMoof = true; // set to false to disable storage of secondary moof boxes in OMAFv2 (e.g. other qualities)

        PipelineBuildInfo withStreamId(StreamId aStreamId) const
        {
            auto ret = *this;
            ret.streamId = aStreamId;
            return ret;
        }
    };

    using TileIndex = StreamSegmenter::IdBaseWithAdditions<size_t, class TileIndexTag>;
    using StreamAndTrack = std::pair<StreamId, TrackId>;
    using StreamAndTrackGroup = std::pair<StreamId, TrackGroupId>;
    using StreamAndTrackIds = std::list<StreamAndTrack>;
    using StreamAndTrackGroupIds = std::list<StreamAndTrackGroup>;
    using TrackIdProvider = std::function<TrackId()>;
    using TrackGroupIdProvider = std::function<TrackGroupId(TileIndex)>;
    using StreamIdProvider = std::function<StreamId()>;

    using AllocateAdaptationSetId = std::function<StreamId(void)>;

    /** Determine a filename component for the given pipeline output. Note: defined in controller.cpp */
    std::string filenameComponentForPipelineOutput(PipelineOutput aOutput);

    /** Determine a output name for the given pipeline output. Note: defined in controller.cpp */
    std::string outputNameForPipelineOutput(PipelineOutput aOutput);

    /** Determine a filename component for the given pipeline output. Note: defined in controller.cpp */
    std::string nameOfPipelineMode(PipelineMode aOutput);

    /** Reads a pipeline mode from a stream. Note: defined in controller.cpp */
    std::istream& operator>>(std::istream& aStream, PipelineMode& aMode);

    /** Easyishly dump values from a function call to its passing as argument, for debugging */
    template <typename Dump, typename Value> Value trace(Dump f, Value&& x);

    template <template <class, class> typename OutputContainer, typename Mapper, typename Container>
    auto mapContainer(const Container& aContainer, const Mapper& aMapper)
        -> OutputContainer<decltype(aMapper(*aContainer.begin())),
                           std::allocator<decltype(aMapper(*aContainer.begin()))>>
    {
        OutputContainer<decltype(aMapper(*aContainer.begin())),
                        std::allocator<decltype(aMapper(*aContainer.begin()))>>
            output;
        for (auto& x : aContainer)
        {
            output.push_back(aMapper(x));
        }
        return output;
    }
}  // namespace VDD

#include "common.icpp"

