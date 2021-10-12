
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

#include "DashProvider/NVRDashVideoDownloaderExtractor.h"
#include "NVREssentials.h"


OMAF_NS_BEGIN

class DashVideoDownloaderExtractorMultiRes : public DashVideoDownloaderExtractor
{
public:
    DashVideoDownloaderExtractorMultiRes();
    virtual ~DashVideoDownloaderExtractorMultiRes();

public:
    virtual void_t clearDownloadedContent();

    virtual Error::Enum startDownload(time_t aStartTime, uint32_t aBufferingTimeMs);
    virtual Error::Enum startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs);
    virtual Error::Enum stopDownload();
    virtual void_t release();

    virtual bool_t isSeekable();
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;
    virtual bool_t isEndOfStream() const;
    virtual bool_t isBuffering();

    virtual const CoreProviderSources& getPastBackgroundVideoSourcesAt(uint64_t aPts);

    virtual Error::Enum completeInitialization(DashComponents& aDashComponents,
                                               SourceDirection::Enum aUriSourceDirection,
                                               sourceid_t aSourceIdBase);

    virtual void_t processSegmentDownload();

    virtual void_t
    setQualityLevels(uint8_t aFgQuality, uint8_t aMarginQuality, uint8_t aBgQuality, uint8_t aNrQualityLevels);

protected:  // from DashVideoDownloaderExtractor
    void_t checkVASVideoStreams(uint64_t currentPTS);
    void_t printViewportTiles(VASTileSelection& viewport) const;

private:
    void_t doSwitch(DashAdaptationSetExtractorMR* aNextVideoBaseAdaptationSet);
    DashAdaptationSetExtractorMR* hasSegmentAvailable(uint32_t aSegmentId, size_t aNrNextSetsToCheck);

private:
    FixedArray<DashAdaptationSetExtractorMR*, 32> mAlternateVideoBaseAdaptationSets;
    FixedArray<DashAdaptationSetExtractorMR*, 32> mNextVideoBaseAdaptationSets;
    struct PastAdaptationSet
    {
        uint64_t validFrom;
        CoreProviderSources sources;
    };
    FixedArray<PastAdaptationSet, 8> mPastVideoSources;
};
OMAF_NS_END
