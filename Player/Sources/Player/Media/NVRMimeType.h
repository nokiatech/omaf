
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
#include "Foundation/NVRFixedString.h"

OMAF_NS_BEGIN

    typedef FixedString128 MimeType;

    static const MimeType VIDEO_H264_MIME_TYPE = "video/avc";
    static const MimeType VIDEO_HEVC_MIME_TYPE = "video/hevc";

    static const MimeType AUDIO_AAC_MIME_TYPE = "audio/mp4a-latm";
    static const MimeType AUDIO_MP3_MIME_TYPE = "audio/mpeg";
    static const MimeType AUDIO_VORBIS_MIME_TYPE = "audio/vorbis";

    static const MimeType UNKNOWN_VIDEO_MIME_TYPE = "video/unknown";
    static const MimeType UNKNOWN_AUDIO_MIME_TYPE = "audio/unknown";
    static const MimeType UNKNOWN_MIME_TYPE = "";

OMAF_NS_END

