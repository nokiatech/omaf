
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

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"


OMAF_NS_BEGIN

class MediaContent
{
public:
    enum class Type : uint64_t
    {
        UNSPECIFIED = 0u,

        AUDIO_LOUDSPEAKER = 1u,

        VIDEO_BASE = (1u << 3),
        VIDEO_ENHANCEMENT = (1u << 4),  // a subpicture with full 360 baselayer

        // generic informative
        IS_MUXED = (1u << 8),
        HAS_AUDIO = (1u << 9),
        HAS_VIDEO = (1u << 10),
        HAS_META = (1u << 11),

        HEVC = (1u << 12),
        AVC = (1u << 13),

        VIDEO_EXTRACTOR = (1u << 14),
        VIDEO_TILE = (1u << 15),  // a tile in OMAF way

        VIDEO_OVERLAY = (1u << 16),  // contains overlay(s)

        METADATA_INVO = (1u << 20),  // Initial viewing orientation
        METADATA_DYOL = (1u << 21)   // Dynamic overlay metadata

    };

    void addType(Type type);
    void removeType(Type type);
    void clear();
    bool_t matches(Type type) const;
    bool_t isSpecified() const;

private:
    uint64_t mTypeBitmask;
};

OMAF_NS_END
