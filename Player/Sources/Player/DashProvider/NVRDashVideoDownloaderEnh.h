
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

#include "NVREssentials.h"
#include "DashProvider/NVRDashVideoDownloader.h"


OMAF_NS_BEGIN

    class DashVideoDownloaderEnh : public DashVideoDownloader
    {

    public:

        DashVideoDownloaderEnh();
        virtual ~DashVideoDownloaderEnh();


    public:
        // called by renderer thread
        // all parameters in degrees
        virtual const MP4VideoStreams& selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet);

        virtual void_t release();
        virtual void_t clearDownloadedContent();

        virtual void_t getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs);


        virtual Error::Enum startDownload(time_t aStartTime);
        virtual Error::Enum stopDownload();

        virtual void_t updateStreams(uint64_t currentPlayTimeUs);

        virtual Error::Enum readVideoFrames(int64_t currentTimeUs);

        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual Error::Enum seekToMs(uint64_t& aSeekMs, uint64_t aSeekResultMs);

        virtual uint32_t getCurrentBitrate();
        virtual bool_t isError() const;

        virtual bool_t isBuffering();
        virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;

        virtual Error::Enum completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo);

        virtual void_t processSegmentDownload();

    protected:

        virtual bool_t updateVideoStreams();
        virtual void_t checkVASVideoStreams(uint64_t currentPTS);

    protected:

    private:
        VASTilesLayer mVideoEnhTiles;
        DashAdaptationSet* mVideoBaseAdaptationSetStereo;
        bool_t mGlobalSubSegmentsSupported;

    };
OMAF_NS_END
