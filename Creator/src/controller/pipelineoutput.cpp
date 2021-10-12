
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
#include "pipelineoutput.h"

namespace VDD
{
    PipelineOutput::PipelineOutput()
        : mValid(false)
    {
    }

    PipelineOutput::PipelineOutput(PipelineOutputVideo aVideo, IsExtractorType)
        : mValid(true), mClass(PipelineClass::Video), mVideo(aVideo), mOverlay(false), mIsExtractor(true)
    {
    }

    PipelineOutput::PipelineOutput(PipelineOutputVideo aVideo, bool aOverlay)
        : mValid(true), mClass(PipelineClass::Video), mVideo(aVideo), mOverlay(aOverlay), mIsExtractor(false)
    {
    }

    PipelineOutput::PipelineOutput(PipelineOutputAudio aAudio, bool aOverlay)
        : mValid(true), mClass(PipelineClass::Audio), mAudio(aAudio), mOverlay(aOverlay), mIsExtractor(false)
    {
    }

    PipelineOutput::PipelineOutput(PipelineOutputTimedMetadata aTimedMetadata)
        : mValid(true), mClass(PipelineClass::TimedMetadata), mTimedMetadata(aTimedMetadata), mIsExtractor(false)
    {
    }

    PipelineOutput::PipelineOutput(PipelineOutputIndexSegment aIndexSegment)
        : mValid(true), mClass(PipelineClass::IndexSegment), mIndexSegment(aIndexSegment), mIsExtractor(false)
    {
    }

    bool PipelineOutput::isVideoOverlay() const
    {
        assert(mClass == PipelineClass::Video);
        return mOverlay;
    }

    PipelineOutput
    PipelineOutput::withExtractorSet() const
    {
        assert(!mOverlay && getClass() == PipelineClass::Video);
        return { getVideo(), IsExtractor };
    }

    PipelineOutput
    PipelineOutput::withVideoOverlaySet() const
    {
        assert(!mIsExtractor);
        switch (getClass())
        {
        case PipelineClass::Video:
            return { getVideo(), true };
        case PipelineClass::Audio:
            return { getAudio(), true };
        case PipelineClass::TimedMetadata:
        case PipelineClass::IndexSegment:
            assert(false);
            return *this;
        default:
            assert(false);
            return *this;
        }
    }

    StreamSegmenter::MediaType PipelineOutput::getMediaType() const
    {
        switch (getClass())
        {
        case PipelineClass::Video:
            return StreamSegmenter::MediaType::Video;
        case PipelineClass::Audio:
            return StreamSegmenter::MediaType::Audio;
        case PipelineClass::TimedMetadata:
            return StreamSegmenter::MediaType::Data;
        case PipelineClass::IndexSegment:
            return StreamSegmenter::MediaType::Data;
        default:
            assert(false);
            return StreamSegmenter::MediaType();
        }
    }
}