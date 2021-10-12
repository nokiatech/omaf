
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

#include "DashProvider/NVRDashUtils.h"
#include "Foundation/NVRFixedString.h"
#include "Media/NVRMediaType.h"
#include "Media/NVRMimeType.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "VAS/NVRVASViewport.h"

OMAF_NS_BEGIN

namespace MPDAttribute
{
    /*
     * Represents the MPEG-DASH Streaming common attributes and elements.
     */
    extern const FixedString64 VIDEO_CONTENT;
    extern const FixedString64 AUDIO_CONTENT;
    extern const FixedString64 METADATA_CONTENT;

    extern const FixedString64 ROLE_URI;
    extern const FixedString64 ROLE_METADATA;
    extern const FixedString64 ROLE_STEREO_URI;
    extern const char_t* ROLE_LEFT;
    extern const char_t* ROLE_RIGHT;
    extern const char_t* FRAME_PACKING_SIDE_BY_SIDE;
    extern const char_t* FRAME_PACKING_TOP_BOTTOM;
    extern const char_t* FRAME_PACKING_TEMPORAL;

    extern const MimeType VIDEO_MIME_TYPE;
    extern const MimeType AUDIO_MIME_TYPE;
    extern const MimeType META_MIME_TYPE;

    extern const FixedString64 SUPPLEMENTAL_PROP;
    extern const FixedString64 ESSENTIAL_PROP;

    extern const char_t* kAssociationIdKey;
    extern const char_t* kAssociationTypeKey;
    extern const char_t* kAssociationTypeValueCdsc;
    extern const char_t* kAssociationTypeValueAudio;
}  // namespace MPDAttribute

namespace StereoRole
{
    enum Enum
    {
        INVALID = -1,
        MONO,
        LEFT,
        RIGHT,
        FRAME_PACKED,
        UNKNOWN,
        COUNT
    };
}

namespace DashAttributes
{
    struct Coverage
    {
        float64_t azimuthCenter;
        float64_t elevationCenter;
        float64_t tiltCenter;
        float64_t azimuthRange;
        float64_t elevationRange;
        bool_t shapeEquirect;
    };

    StereoRole::Enum getStereoRole(DashComponents& aDashComponents);
    SourceDirection::Enum getFramePacking(DashComponents& aDashComponents);
};  // namespace DashAttributes

OMAF_NS_END
