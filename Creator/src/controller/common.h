
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include <streamsegmenter/segmenterapi.hpp>

#include "async/asyncnode.h"
#include "segmenter/segmenter.h"
#include "segmenter/save.h"

namespace VDD
{
    struct SegmentDurations
    {
        StreamSegmenter::Segmenter::Duration segmentDuration;
        size_t subsegmentsPerSegment;
    };

    struct SegmenterAndSaverConfig
    {
        Segmenter::Config segmenterConfig;
        Save::Config segmentSaverConfig;
    };

    enum class PipelineOutput : int {
        VideoMono,
		VideoFramePacked,	// generic, not to be used with OMAF
        VideoTopBottom,
        VideoSideBySide,
        VideoLeft,
        VideoRight,
        Audio,
        VideoExtractor
    };

    /* Given an output a pipeline produces, give the AsyncNode* that the data flows out to the next
     * step. */
    using PipelineOutputNodeMap = std::map<PipelineOutput, std::pair<AsyncNode*, ViewMask>>;

    enum class PipelineMode : int {
        VideoMono,
		VideoFramePacked,	// generic, not to be used with OMAF
        VideoTopBottom,
        VideoSideBySide,
        VideoSeparate,
        Audio
    };

    using StreamAndTrack = std::pair<StreamId, TrackId>;
    using StreamAndTrackIds = std::list<StreamAndTrack>;


    /** Determine a filename component for the given pipeline output. Note: defined in controller.cpp */
    std::string filenameComponentForPipelineOutput(PipelineOutput aOutput);

    /** Determine a output name for the given pipeline output. Note: defined in controller.cpp */
    std::string outputNameForPipelineOutput(PipelineOutput aOutput);

    /** Determine a filename component for the given pipeline output. Note: defined in controller.cpp */
    std::string nameOfPipelineMode(PipelineMode aOutput);

    /** Reads a pipeline mode from a stream. Note: defined in controller.cpp */
    std::istream& operator>>(std::istream& aStream, PipelineMode& aMode);
}
