
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
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"


OMAF_NS_BEGIN

    class DashVideoDownloaderExtractorMultiRes : public DashVideoDownloaderExtractor
    {

    public:

        DashVideoDownloaderExtractorMultiRes();
        virtual ~DashVideoDownloaderExtractorMultiRes();

    public:

        virtual void_t clearDownloadedContent();

        virtual Error::Enum startDownload(time_t aStartTime, uint32_t aExpectedPingTimeMs);
        virtual Error::Enum stopDownload();
        virtual void_t release();

        virtual bool_t isSeekable();
        virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;
        virtual bool_t isEndOfStream() const;

        virtual Error::Enum completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo);

        virtual void_t processSegmentDownload();

    protected:

        void_t checkVASVideoStreams(uint64_t currentPTS);

    protected:
    private:
        FixedArray<DashAdaptationSetExtractorMR*, 32> mAlternateVideoBaseAdaptationSets;
        DashAdaptationSetExtractorMR* mNextVideoBaseAdaptationSet;
        uint32_t mExpectedPingTimeMs;
    };
OMAF_NS_END
