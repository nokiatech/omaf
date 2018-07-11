
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
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRArray.h"
#include "Foundation/NVRSpinlock.h"

#include "Media/NVRMP4Parser.h"
#include "Media/NVRMP4VideoStream.h"
#include "Media/NVRMP4AudioStream.h"

#include "DashProvider/NVRDashMediaSegment.h"
#include "DashProvider/NVRDashUtils.h"
#include "DashProvider/NVRDashSegmentStream.h"

OMAF_NS_BEGIN

    class DashRepresentation;

    class DashRepresentationObserver
    {
    public:
        virtual void_t onSegmentDownloaded(DashRepresentation* representation) = 0;
        virtual void_t onNewStreamsCreated() = 0;
        virtual void_t onCacheWarning() = 0;
    };

    class DashRepresentation
            : public DashSegmentStreamObserver
    {
    public:
        
        DashRepresentation();
        Error::Enum initialize(
            DashComponents dashComponents,
            uint32_t bitrate,
            MediaContent content,
            uint32_t aInitializationSegmentId,
            DashRepresentationObserver* observer);
        virtual ~DashRepresentation();

        bool_t mpdUpdateRequired();
        Error::Enum updateMPD(DashComponents components);

        virtual Error::Enum startDownload(time_t startDownloadTime);
        virtual Error::Enum startDownloadABR(uint32_t overrideSegmentId);
        virtual Error::Enum startDownloadWithOverride(uint64_t overridePTS, uint32_t overrideSegmentId);
        virtual Error::Enum stopDownload();
        virtual Error::Enum stopDownloadAsync(bool_t aReset);

        bool_t supportsSubSegments() const;
        Error::Enum hasSubSegmentsFor(uint64_t targetPtsUs, uint32_t overrideSegmentId);
        Error::Enum startSubSegmentDownload(uint64_t targetPtsUs, uint32_t overrideSegmentId);

        virtual void_t clearDownloadedContent();

        void processSegmentDownload();
        void_t processSegmentIndexDownload(uint32_t segmentId);

        bool_t isDone();//when all packets and segments are used up.. and not downloading more.. 

        uint32_t getWidth() const;
        uint32_t getHeight() const;
        float64_t getFramerate() const;
        uint32_t getBitrate() const;

        void_t assignQualityLevel(uint8_t aQualityIndex);
        uint8_t getQualityLevel() const;

        const SegmentContent& getSegmentContent() const;

        void setAssociatedToRepresentation(DashRepresentation* representation);
        MP4VRParser* getParserInstance();

    public:
        virtual MP4VideoStreams& getCurrentVideoStreams();
        virtual MP4AudioStreams& getCurrentAudioStreams();

        virtual Error::Enum readNextVideoFrame(bool_t& segmentChanged, int64_t currentTimeUs);
        virtual Error::Enum readNextAudioFrame(bool_t& segmentChanged);

        virtual void_t createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel);
        virtual void_t createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel, const VASTileViewport& viewport);
        virtual void_t updateVideoSourceTo(SourcePosition::Enum aPosition);

        virtual const CoreProviderSources& getVideoSources();
        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual uint64_t durationMs();
        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs);

        virtual Error::Enum seekStreamsTo(uint64_t seekToPts);

        virtual bool_t isDownloading() { return mDownloading; }
        virtual bool_t isBuffering();
        virtual bool_t isEndOfStream();
        virtual bool_t isError();
        virtual void_t releaseOldSegments();

        virtual float32_t getDownloadSpeedFactor();

        virtual uint64_t getReadPositionUs(uint32_t& segmentIndex);
        virtual uint32_t getLastSegmentId();
        virtual uint32_t getSegmentId();
        virtual bool_t isSegmentDurationFixed(uint64_t& segmentDurationMs);

        virtual uint32_t cachedSegments() const;

        virtual const char_t* getId() const;
        virtual const char_t* isAssociatedTo() const;

        void_t setCacheFillMode(bool_t autoFill);

        virtual void_t setNotSupported();
        virtual bool_t getIsSupported();

        bool_t getIsInitialized() const;

        virtual DashSegment* peekSegment() const;
        virtual size_t getNrSegments() const;
        virtual DashSegment* getSegment();
        virtual bool_t readyForSegment(uint32_t aId);
        virtual Error::Enum parseConcatenatedMediaSegment(DashSegment *aSegment);
        virtual void_t cleanUpOldSegments(uint32_t aNextSegmentId);
        virtual bool_t hasSegment(uint32_t aSegmentId, size_t& aSegmentSize);
    public:
        // DashSegmentStreamObserver
        Error::Enum onMediaSegmentDownloaded(DashSegment *segment);
        void_t onSegmentIndexDownloaded(DashSegment *segment);
        void_t onCacheWarning();

    protected:
        virtual Error::Enum handleInputSegment(DashSegment* aSegment);
        virtual void_t seekWhenSegmentAvailable();

    protected:
        DashRepresentationObserver *mObserver;
        MP4VideoStreams mVideoStreams;
        MP4AudioStreams mAudioStreams;
        streamid_t mVideoStreamId;  // we assume 1 representation carries max 1 video stream
        VideoStreamMode::Enum mStreamMode;    // need to be saved until we have stream opened
        mutable Spinlock mLock;
        bool_t mInitializeIndependently; // separate vs common init
        BasicSourceInfo mBasicSourceInfo;
        uint64_t mSeekToUsWhenDLComplete;

    private:
        dash::mpd::ISegmentTemplate* getSegmentTemplate(DashComponents dashComponents);

        void_t loadVideoMetadata();

    private:

        MemoryAllocator& mMemoryAllocator;

        bool_t mOwnsParserInstance;
        MP4VRParser* mParser;
        DashRepresentation* mAssociatedToRepresentation;

        bool_t mInitialized;

        DashSegmentStream *mSegmentStream;

        DashStreamType::Enum mStreamType;

        uint32_t mLastSegmentId;

        bool_t mDownloading;

        bool_t mMetadataCreated;
        SegmentContent mSegmentContent;
        uint32_t mWidth;
        uint32_t mHeight;
        float64_t mFps;
        bool_t mRestarted;
        bool_t mAutoFillCache;
        bool_t mSubSegmentSupported;
        bool_t mDownloadSubSegment;

        bool_t mIsSupported;    // the representation parameters fall within the supported range in this device
        uint32_t mBitrate;

        uint8_t mQualityLevel;
    };
OMAF_NS_END
