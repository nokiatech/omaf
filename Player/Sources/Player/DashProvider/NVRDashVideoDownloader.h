
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
#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "VAS/NVRVASTilePicker.h"
#include "VAS/NVRVASTileContainer.h"
#include "Media/NVRMP4StreamManager.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "DashProvider/NVRDashBitrateController.h"


OMAF_NS_BEGIN
    namespace VASType
    {
        enum Enum
        {
            INVALID = -1,
            NONE,
            SUBPICTURES,
            EXTRACTOR_PRESELECTIONS_QUALITY,    // single resolution tiles, multiple qualities, linked with Preselections
            EXTRACTOR_DEPENDENCIES_QUALITY,     // extractor has the qualities & coverage, linked with dependencyId
            EXTRACTOR_PRESELECTIONS_RESOLUTION, // multi-resolution tiles, linked with Preselections
            EXTRACTOR_DEPENDENCIES_RESOLUTION,  // multi-resolution tiles, linked with dependencyId
            COUNT
        };

    }

    class DashVideoDownloader : public DashAdaptationSetObserver
    {

    public:

        DashVideoDownloader();
        virtual ~DashVideoDownloader();

        void_t enableABR();
        void_t disableABR();
        void_t setBandwidthOverhead(uint32_t aBandwidthOverhead);


        bool_t isMpdUpdateRequired() const;


        void_t getMediaInformation(MediaInformation& aMediaInformation);

    public:
        // called by renderer thread
        // all parameters in degrees
        virtual const MP4VideoStreams& selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet);

        virtual void_t release();
        virtual void_t clearDownloadedContent();

        virtual void_t getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs);


        virtual Error::Enum startDownload(time_t aStartTime, uint32_t aExpectedPingTimeMs);
        virtual Error::Enum stopDownload();

        virtual void_t updateStreams(uint64_t currentPlayTimeUs);

        virtual const MP4VideoStreams& getVideoStreams();
        virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);


        virtual Error::Enum readVideoFrames(int64_t currentTimeUs);

        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekMs, uint64_t aSeekResultMs);

        virtual uint32_t getCurrentBitrate();
        virtual bool_t isError() const;

        virtual uint64_t durationMs() const;
        virtual bool_t isEndOfStream() const;

        virtual bool_t isBuffering();
        virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;

        virtual Error::Enum completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo);

        virtual void_t processSegmentDownload();

    public: // from AdaptationSetObserver
        void_t onNewStreamsCreated();
        void_t onDownloadProblem(IssueType::Enum aIssueType);

    protected:

        virtual bool_t updateVideoStreams();

    protected:
        AdaptationSets mAdaptationSets;
        DashAdaptationSet* mVideoBaseAdaptationSet;

        // The two tile-lists are alternatives: first is used with base layer + enhancement layer scheme, the 2nd with OMAF HEVC tiles
        DashBitrateContoller mBitrateController;
        VASTilePicker* mTilePicker;

        MediaInformation mMediaInformation;
        bool_t mStreamUpdateNeeded;
        AtomicBoolean mVideoStreamsChanged;
        bool_t mTileSetupDone;
        bool_t mReselectSources;
        uint32_t mBaseLayerDecoderPixelsinSec;
        MP4VideoStreams mCurrentVideoStreams;
        MP4VideoStreams mRenderingVideoStreams;
        CoreProviderSourceTypes mVideoSourceTypes;
    private:

        DashStreamType::Enum mStreamType;

        AtomicBoolean mNewVideoStreamsCreated;

        bool_t mABREnabled;
        uint32_t mBandwidthOverhead;
    };
OMAF_NS_END
