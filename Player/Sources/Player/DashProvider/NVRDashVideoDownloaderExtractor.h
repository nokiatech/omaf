
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

#include "NVRDashVideoDownloader.h"
#include "NVREssentials.h"


OMAF_NS_BEGIN

class DashVideoDownloaderExtractor : public DashVideoDownloader
{
public:
    DashVideoDownloaderExtractor();
    virtual ~DashVideoDownloaderExtractor();

public:
    // called by renderer thread
    // all parameters in degrees
    virtual const MP4VideoStreams& selectSources(float64_t longitude,
                                                 float64_t latitude,
                                                 float64_t roll,
                                                 float64_t width,
                                                 float64_t height,
                                                 bool_t& aViewportSet);
    virtual void_t release();

    virtual Error::Enum startDownload(time_t aStartTime, uint32_t aBufferingTimeMs);
    virtual Error::Enum startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs);
    virtual void_t updateStreams(uint64_t currentPlayTimeUs);
    virtual void_t processSegments();

    virtual Error::Enum completeInitialization(DashComponents& aDashComponents,
                                               SourceDirection::Enum aUriSourceDirection,
                                               sourceid_t aSourceIdBase);

protected:
    virtual void_t printViewportTiles(VASTileSelection& viewport) const;
    struct ViewportRate
    {
        float32_t top;
        float32_t left;
        uint32_t bitrate;
    };

    void_t estimateBitrates(uint8_t aNrQualityLevels);
    void_t estimateBitrateForVP(float64_t top,
                                float64_t bottom,
                                float64_t left,
                                float64_t right,
                                uint8_t aNrQualityLevels,
                                FixedArray<ViewportRate, 200>& aBitrates);

    virtual void_t checkVASVideoStreams(uint64_t currentPTS);
    void_t initializeTiles();

protected:
    VASTilesLayer mVideoPartialTiles;

private:
};
OMAF_NS_END
