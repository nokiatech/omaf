
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

#include <cassert>
#include <map>

#include "pipelineclass.h"
#include "processor/streamfilter.h"
#include "streamsegmenter/track.hpp"

namespace VDD
{
    enum class PipelineOutputVideo
    {
        Mono,
        FramePacked,  // generic, not to be used with OMAF
        TopBottom,
        SideBySide,
        Left,
        Right,
        TemporalInterleaving,
        NonVR
    };

    enum class PipelineOutputAudio
    {
        Audio
    };

    enum class PipelineOutputTimedMetadata
    {
        TimedMetadata
    };

    enum class PipelineOutputIndexSegment
    {
        IndexSegment
    };

    class PipelineOutput {
    public:
        enum IsExtractorType {
            IsExtractor
        };

        PipelineOutput();

        PipelineOutput(PipelineOutputVideo aVideo, IsExtractorType);
        PipelineOutput(PipelineOutputVideo aVideo, bool aOverlay = false);
        PipelineOutput(PipelineOutputAudio aAudio, bool aOverlay = false);
        PipelineOutput(PipelineOutputTimedMetadata aTimedMetadata);
        PipelineOutput(PipelineOutputIndexSegment aIndexSegment);

        inline bool isExtractor() const
        {
            return mIsExtractor;
        }

        inline PipelineClass getClass() const
        {
            assert(mValid);
            return mClass;
        }

        inline PipelineOutputVideo getVideo() const
        {
            assert(mValid && mClass == PipelineClass::Video);
            return mVideo;
        }

        PipelineOutput withExtractorSet() const;

        PipelineOutput withVideoOverlaySet() const;

        bool isVideoOverlay() const;

        inline PipelineOutputAudio getAudio() const
        {
            assert(mValid && mClass == PipelineClass::Audio);
            return mAudio;
        }

        inline PipelineOutputTimedMetadata getTimedMetadata() const
        {
            assert(mValid && mClass == PipelineClass::TimedMetadata);
            return mTimedMetadata;
        }

        StreamSegmenter::MediaType getMediaType() const;

    private:
        bool mValid;
        PipelineClass mClass;
        union {
            PipelineOutputVideo mVideo;
            PipelineOutputAudio mAudio;
            PipelineOutputTimedMetadata mTimedMetadata;
            PipelineOutputIndexSegment mIndexSegment;
        };
        bool mOverlay; // applicable for video and audio
        bool mIsExtractor;

        friend bool operator<(const PipelineOutput& a, const PipelineOutput& b);
    };

    inline bool operator<(const PipelineOutput& a, const PipelineOutput& b)
    {
        assert(a.mValid && b.mValid);
        if (a.mIsExtractor < b.mIsExtractor)
        {
            return true;
        }
        if (a.mIsExtractor > b.mIsExtractor)
        {
            return false;
        }
        if (a.mClass < b.mClass)
        {
            return true;
        }
        if (a.mClass > b.mClass)
        {
            return false;
        }
        else // implies mClass and mIsExtractor match
        {
            switch (a.mClass)
            {
            case PipelineClass::Video:
                return (a.mVideo < b.mVideo) || (a.mVideo == b.mVideo && a.mOverlay < b.mOverlay);
            case PipelineClass::Audio:
                return a.mAudio < b.mAudio;
            case PipelineClass::TimedMetadata:
                return a.mTimedMetadata < b.mTimedMetadata;
            case PipelineClass::IndexSegment:
                return a.mIndexSegment < b.mIndexSegment;
            }
            return false;  // this code is never reached..
        }
    }

    inline bool operator>(const PipelineOutput& a, const PipelineOutput& b)
    {
        return b < a;
    }

    inline bool operator==(const PipelineOutput& a, const PipelineOutput& b)
    {
        return !(a < b) && !(a > b);
    }

    inline bool operator<=(const PipelineOutput& a, const PipelineOutput& b)
    {
        return a < b || a == b;
    }

    inline bool operator!=(const PipelineOutput& a, const PipelineOutput& b)
    {
        return !(a == b);
    }

    inline bool operator>=(const PipelineOutput& a, const PipelineOutput& b)
    {
        return b < a || a == b;
    }

    class AsyncNode;

    /* Given an output a pipeline produces, give the AsyncNode* that the data flows out to the next
     * step. */
    using PipelineOutputNodeMap = std::map<PipelineOutput, std::pair<AsyncNode*, StreamFilter>>;
}