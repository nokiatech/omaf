
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

#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "NVRErrorCodes.h"

#include "DashProvider/NVRDashRepresentation.h"
#include "DashProvider/NVRDashUtils.h"
#include "DashProvider/NVRDashIssueType.h"
#include "DashProvider/NVRMPDOmafAttributes.h"
#include "VAS/NVRVASViewport.h"


OMAF_NS_BEGIN
    class DashAdaptationSetObserver
    {
    public:
        virtual void_t onNewStreamsCreated() = 0;
        virtual void_t onDownloadProblem(IssueType::Enum aIssueType) = 0;
    };

    typedef FixedArray<uint32_t, 32> Bitrates;

    class DashAdaptationSet;
#define MAX_ADAPTATION_SET_COUNT 128
    typedef FixedArray<DashAdaptationSet*, MAX_ADAPTATION_SET_COUNT> AdaptationSets;


    namespace DashType
    {
        enum Enum
        {
            INVALID = -1,
            BASELINE_DASH,
            DASH_WITH_VIDEO_METADATA,
            OMAF,
            COUNT
        };
    }

    namespace AdaptationSetType
    {
        enum Enum
        {
            INVALID = -1,
            BASELINE_DASH,
            SUBPICTURE,
            TILE,
            EXTRACTOR,
            COUNT
        };
    }

    class DashAdaptationSet
    : public DashRepresentationObserver
    {
    public:
        DashAdaptationSet(DashAdaptationSetObserver& observer);

        virtual ~DashAdaptationSet();

        static bool_t externalAdSetIdExists();
        static uint32_t getNextAdSetId();

        virtual AdaptationSetType::Enum getType() const;

        virtual Error::Enum initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);

        virtual bool_t mpdUpdateRequired();
        virtual Error::Enum updateMPD(DashComponents dashComponents);

        virtual Error::Enum startDownload(time_t startTime);
        virtual Error::Enum startDownload(time_t startTime, uint32_t aExpectedPingTimeMs);
        virtual Error::Enum startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId, uint32_t aNextToBeProcessedSegmentId, uint32_t aExpectedPingTimeMs);
        virtual Error::Enum startDownloadFromTimestamp(uint64_t overridePTSUs, uint32_t overrideSegmentId, VideoStreamMode::Enum aMode);
        virtual Error::Enum stopDownload();
        virtual Error::Enum stopDownloadAsync(bool_t aAbort, bool_t aReset);

        virtual uint32_t getCurrentWidth() const;
        virtual uint32_t getCurrentHeight() const;
        virtual uint32_t getCurrentBandwidth() const;
        virtual float64_t getCurrentFramerate() const;
        virtual float64_t getMinResXFps() const;

        virtual bool_t supportsSubSegments() const;
        virtual Error::Enum hasSubSegments(uint64_t targetPtsUs, uint32_t overrideSegmentId);
        virtual Error::Enum startSubSegmentDownload(uint64_t targetPtsUs, uint32_t overrideSegmentId);

        virtual void_t clearDownloadedContent();
        virtual bool_t processSegmentDownload();
        virtual void_t processSegmentIndexDownload(uint32_t segmentId);

        virtual MediaContent& getAdaptationSetContent();
        virtual const RepresentationId& getCurrentRepresentationId();
        virtual DashRepresentation* getCurrentRepresentation();
        virtual DashRepresentation* getNextRepresentation();

        // metadata related methods
        virtual bool_t isAssociatedToRepresentation(RepresentationId& aAssociatedTo);
        virtual void setMetadataAdaptationSet(DashAdaptationSet* metadata);
        virtual DashAdaptationSet* getMetadataAdaptationSet();

        virtual const MP4VideoStreams& getCurrentVideoStreams();
        virtual const MP4AudioStreams& getCurrentAudioStreams();
        virtual const MP4MetadataStreams& getCurrentMetadataStreams();
        virtual Error::Enum readNextVideoFrame(int64_t currentTimeUs);
        virtual Error::Enum readNextAudioFrame();
        virtual Error::Enum readMetadataFrame(int64_t currentTimeUs);

        virtual bool_t createVideoSources(sourceid_t& firstSourceId);
        virtual Error::Enum createMetadataParserLink(DashAdaptationSet* aDependsOn);

        virtual const CoreProviderSources& getVideoSources();
        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);

        virtual uint64_t durationMs();

        virtual bool_t isBuffering();
        virtual bool_t isEndOfStream();
        virtual bool_t isError();

        virtual const char_t* getRepresentationId();
        virtual const char_t* isAssociatedTo();

        virtual uint64_t getReadPositionUs(uint32_t& segmentIndex);
        virtual uint32_t getLastSegmentId(bool_t aIncludeSegmentInDownloading = false) const;

        virtual bool_t isBaselayer() const;
        virtual StereoRole::Enum getVideoChannel() const;
        virtual const VASTileViewport* getCoveredViewport() const;
        virtual bool_t forceVideoTo(StereoRole::Enum aToRole);

        virtual bool_t hasMPDVideoMetadata() const;
        virtual VideoStreamMode::Enum getVideoStreamMode() const;

        virtual bool_t getAlignmentId(uint64_t& segmentDurationMs, uint32_t& aAlignmentId);

        virtual uint32_t getNrOfRepresentations() const;
        virtual Error::Enum getRepresentationParameters(uint32_t aIndex, uint32_t& aWidth, uint32_t& aHeight, float64_t& aFps) const;
        virtual Error::Enum setRepresentationNotSupported(uint32_t aIndex);

        virtual void_t selectBitrate(uint32_t bitrate);
        virtual bool_t isABRSwitchOngoing();

        virtual float32_t getDownloadSpeedFactor();

        virtual const Bitrates& getBitrates();
        virtual bool_t isActive() const;

        virtual uint32_t getId() const;
        virtual bool_t getIsInitialized() const;

        virtual uint32_t peekNextSegmentId() const;
        virtual uint8_t getNrQualityLevels() const;
        virtual DashRepresentation* getRepresentationForBitrate(uint32_t bitrate);

        virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;
        // DashRepresentationObserver
    public:
        void_t onSegmentDownloaded(DashRepresentation* representation, float32_t aSpeedFactor);
        void_t onNewStreamsCreated();
        void_t onCacheWarning();

    protected:
        virtual Error::Enum doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId);
        virtual void_t doSwitchRepresentation();
        virtual bool_t parseVideoProperties(DashComponents& aNextComponents);
        virtual void_t parseVideoViewport(DashComponents& aNextComponents);
        virtual bool_t parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityIndex, bool_t& aGlobal, DashRepresentation* aLatestRepresentation);
        virtual void_t parseVideoChannel(DashComponents& aNextComponents);
        virtual DashRepresentation* createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth);

    protected:
        MemoryAllocator& mMemoryAllocator;
        MediaContent mContent;
        time_t mDownloadStartTime;
        typedef FixedArray<DashRepresentation*, 512> Representations;
        Representations mRepresentations;   // ordered by bitrate; owns the members
        DashRepresentation* mCurrentRepresentation;
        DashRepresentation* mNextRepresentation;
        VASTileViewport* mCoveredViewport;
        uint32_t mAdaptationSetId;
        SourceType::Enum mSourceType;
        Bitrates mBitrates;
        DashType::Enum mDashType;
        StereoRole::Enum mVideoChannel;

    private:
        void_t analyzeContent(DashComponents aDashComponents);
        bool_t checkMime(const std::string& aMimeType);
        bool_t checkCodecs(const std::string& aCodec);

    private:
        DashAdaptationSetObserver& mObserver;
        uint64_t mDuration;
        uint32_t mMaxBandwidth;
        uint32_t mMinBandwidth;
        DashAdaptationSet* mMetadataAdaptationSet;

        float32_t mPreviousSpeedFactor;
        bool_t mIsSeekable;
        uint32_t mVideoChannelIndex;

        Spinlock mRepresentationLock;

        bool_t mHasSegmentAlignmentId;
        uint32_t mSegmentAlignmentId;


    };
OMAF_NS_END
