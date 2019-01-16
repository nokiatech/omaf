
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "NVREssentials.h"
#include "NVRDashVideoDownloader.h"


OMAF_NS_BEGIN

    class DashVideoDownloaderExtractor : public DashVideoDownloader
    {

    public:

        DashVideoDownloaderExtractor();
        virtual ~DashVideoDownloaderExtractor();

    public:
        // called by renderer thread
        // all parameters in degrees
        virtual const MP4VideoStreams& selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet);
        virtual void_t release();

        virtual void_t updateStreams(uint64_t currentPlayTimeUs);

        virtual Error::Enum completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo);

    protected:

        virtual void_t checkVASVideoStreams(uint64_t currentPTS);

    protected:
        VASTilesLayer mVideoPartialTiles;

    private:
    };
OMAF_NS_END
