
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

namespace VDD
{
    enum class PipelineModeVideo
    {
        Mono,
        FramePacked,  // generic, not to be used with OMAF
        TopBottom,
        SideBySide,
        Separate,
        Overlay  // OMAF video overlay
    };

    enum class PipelineModeAudio
    {
        Audio
    };

    enum class PipelineModeTimedMetadata
    {
        TimedMetadata
    };

    enum class PipelineModeIndexSegment
    {
        IndexSegment
    };

    class PipelineMode
    {
    public:
        PipelineMode();

        PipelineMode(PipelineModeVideo aOutput);
        PipelineMode(PipelineModeAudio aOutput);
        PipelineMode(PipelineModeTimedMetadata aOutput);
        PipelineMode(PipelineModeIndexSegment aOutput);

        inline PipelineClass getClass() const
        {
            assert(mValid);
            return mClass;
        }

        inline PipelineModeVideo getVideo() const
        {
            assert(mValid && mClass == PipelineClass::Video);
            return mVideo;
        }

        inline PipelineModeAudio getAudio() const
        {
            assert(mValid && mClass == PipelineClass::Audio);
            return mAudio;
        }

        inline PipelineModeTimedMetadata getTimedMetadata() const
        {
            assert(mValid && mClass == PipelineClass::TimedMetadata);
            return mTimedMetadata;
        }

    private:
        bool mValid;
        PipelineClass mClass;
        union {
            PipelineModeVideo mVideo;
            PipelineModeAudio mAudio;
            PipelineModeTimedMetadata mTimedMetadata;
            PipelineModeIndexSegment mIndexSegment;
        };

        friend bool operator<(const PipelineMode& a, const PipelineMode& b);
    };

    inline bool operator<(const PipelineMode& a, const PipelineMode& b)
    {
        assert(a.mValid && b.mValid);
        if (a.mClass < b.mClass)
        {
            return true;
        }
        else if (a.mClass == b.mClass)
        {
            switch (a.mClass)
            {
            case PipelineClass::Video:
                return a.mVideo < b.mVideo;
            case PipelineClass::Audio:
                return a.mAudio < b.mAudio;
            case PipelineClass::TimedMetadata:
                return a.mTimedMetadata < b.mTimedMetadata;
            case PipelineClass::IndexSegment:
                return a.mIndexSegment < b.mIndexSegment;
            }
        }
        else
        {
            return false;
        }
        return false;
    }

    inline bool operator>(const PipelineMode& a, const PipelineMode& b)
    {
        return b < a;
    }

    inline bool operator==(const PipelineMode& a, const PipelineMode& b)
    {
        return !(a < b) && !(a > b);
    }

    inline bool operator<=(const PipelineMode& a, const PipelineMode& b)
    {
        return a < b || a == b;
    }

    inline bool operator!=(const PipelineMode& a, const PipelineMode& b)
    {
        return !(a == b);
    }

    inline bool operator>=(const PipelineMode& a, const PipelineMode& b)
    {
        return b < a || a == b;
    }

}