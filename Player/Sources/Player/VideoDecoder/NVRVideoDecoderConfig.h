
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

#include "Media/NVRMimeType.h"
#include "Provider/NVRCoreProviderSources.h"

OMAF_NS_BEGIN

struct VideoInfo
{
    uint32_t width;
    uint32_t height;

    streamid_t streamId;

    VideoInfo()
        : width(0)
        , height(0)
        , streamId(OMAF_UINT8_MAX)
    {
    }
    VideoInfo(const VideoInfo& other)
        : width(other.width)
        , height(other.height)
        , streamId(other.streamId)
    {
    }
};

struct DecoderConfig : public VideoInfo
{
    static const size_t DATA_SIZE = 256;

    MimeType mimeType;

    size_t configInfoSize;
    uint8_t configInfoData[DATA_SIZE];

    size_t spsSize;
    uint8_t spsData[DATA_SIZE];

    size_t ppsSize;
    uint8_t ppsData[DATA_SIZE];

    size_t vpsSize;
    uint8_t vpsData[DATA_SIZE];

    DecoderConfig()
        : VideoInfo()
        , mimeType(UNKNOWN_MIME_TYPE)
        , configInfoSize(0)
        , spsSize(0)
        , ppsSize(0)
        , vpsSize(0)
    {
        MemoryZero(configInfoData, sizeof(configInfoData));
        MemoryZero(spsData, sizeof(spsData));
        MemoryZero(ppsData, sizeof(ppsData));
        MemoryZero(vpsData, sizeof(vpsData));
    }

    DecoderConfig(const DecoderConfig& other)
        : VideoInfo(other)
        , mimeType(other.mimeType)
        , configInfoSize(other.configInfoSize)
        , spsSize(other.spsSize)
        , ppsSize(other.ppsSize)
        , vpsSize(other.vpsSize)
    {
        MemoryCopy(configInfoData, other.configInfoData, configInfoSize);
        MemoryCopy(vpsData, other.vpsData, vpsSize);
        MemoryCopy(spsData, other.spsData, spsSize);
        MemoryCopy(ppsData, other.ppsData, ppsSize);
    }

    DecoderConfig& operator=(const DecoderConfig& other)
    {
        mimeType = other.mimeType;

        width = other.width;
        height = other.height;

        streamId = other.streamId;

        configInfoSize = other.configInfoSize;
        MemoryCopy(configInfoData, other.configInfoData, configInfoSize);

        vpsSize = other.vpsSize;
        MemoryCopy(vpsData, other.vpsData, vpsSize);

        spsSize = other.spsSize;
        MemoryCopy(spsData, other.spsData, spsSize);

        ppsSize = other.ppsSize;
        MemoryCopy(ppsData, other.ppsData, ppsSize);

        return *this;
    }
};

OMAF_NS_END
