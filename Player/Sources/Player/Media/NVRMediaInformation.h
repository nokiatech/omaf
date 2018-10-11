
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
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN

namespace MediaFileType
{
    enum Enum
    {
        INVALID = - 1,

        MP4,
        MP4_OMAF,
        DASH,
        DASH_OMAF,
        HEIF,
        HEIF_OMAF,
        COUNT
    };
}

namespace StreamType
{
    enum Enum
    {
        INVALID = -1,

        LOCAL_FILE,
        LIVE_STREAM,
        VIDEO_ON_DEMAND,

        COUNT
    };
}

namespace VideoType
{
    enum Enum
    {
        INVALID = -1,

        FIXED_RESOLUTION,
        VARIABLE_RESOLUTION,
        VIEW_ADAPTIVE,

        COUNT,
    };
}

namespace VideoCodec
{
    enum Enum
    {
        INVALID = -1,

        AVC,
        HEVC,

        COUNT
    };
}

namespace AudioFormat
{
    enum Enum
    {
        INVALID = -1,

        LOUDSPEAKER,

        COUNT
    };
}

struct MediaInformation
{
    MediaInformation()
    {
        width = 0;
        height = 0;
        duration = 0;
        fileType = MediaFileType::INVALID;
        numberOfFrames = 0;
        frameRate = 0;
        isStereoscopic = false;
        baseLayerCodec = VideoCodec::INVALID;
        enchancementLayerCodec = VideoCodec::INVALID;
        audioFormat = AudioFormat::INVALID;
    }

    uint32_t width;
    uint32_t height;
    uint64_t duration;
    uint64_t numberOfFrames;

    float64_t frameRate;
    bool_t isStereoscopic;

    VideoCodec::Enum baseLayerCodec;
    VideoCodec::Enum enchancementLayerCodec;
    AudioFormat::Enum audioFormat;
    MediaFileType::Enum fileType;
    VideoType::Enum videoType;
    StreamType::Enum streamType;

};

OMAF_NS_END
