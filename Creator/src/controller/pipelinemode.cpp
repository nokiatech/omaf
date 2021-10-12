
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
#include "pipelinemode.h"

namespace VDD
{
    PipelineMode::PipelineMode() : mValid(false)
    {
    }

    PipelineMode::PipelineMode(PipelineModeVideo aVideo)
        : mValid(true), mClass(PipelineClass::Video), mVideo(aVideo)
    {
    }

    PipelineMode::PipelineMode(PipelineModeAudio aAudio)
        : mValid(true), mClass(PipelineClass::Audio), mAudio(aAudio)
    {
    }

    PipelineMode::PipelineMode(PipelineModeTimedMetadata aTimedMetadata)
        : mValid(true), mClass(PipelineClass::TimedMetadata), mTimedMetadata(aTimedMetadata)
    {
    }

    PipelineMode::PipelineMode(PipelineModeIndexSegment aIndexSegment)
        : mValid(true), mClass(PipelineClass::IndexSegment), mIndexSegment(aIndexSegment)
    {
    }
}  // namespace VDD