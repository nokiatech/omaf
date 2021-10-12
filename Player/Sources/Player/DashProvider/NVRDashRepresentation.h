
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

#include "Foundation/NVRArray.h"
#include "Foundation/NVRSpinlock.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "Media/NVRMP4AudioStream.h"
#include "Media/NVRMP4Parser.h"
#include "Media/NVRMP4VideoStream.h"

#include "DashProvider/NVRDashMediaSegment.h"
#include "DashProvider/NVRDashSegmentStream.h"
#include "DashProvider/NVRDashUtils.h"

#include "DashProvider/NVRDashMediaStream.h"

OMAF_NS_BEGIN

class DashRepresentation;

class DashRepresentationObserver
{
public:
    virtual void_t onSegmentDownloaded(DashRepresentation* representation, float32_t aSpeedFactor) = 0;
    virtual void_t onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs) = 0;
    virtual void_t onNewStreamsCreated(MediaContent& aType) = 0;
    virtual void_t onCacheWarning() = 0;
};

class DashRepresentation : public DashSegmentStreamObserver, public MP4StreamCreator
{
public:
    DashRepresentation();
    Error::Enum initialize(DashComponents dashComponents,
                           uint32_t bitrate,
                           MediaContent content,
                           uint32_t aInitializationSegmentId,
                           DashRepresentationObserver* observer,
                           DashAdaptationSet* adaptationSet);
    virtual ~DashRepresentation();

    bool_t mpdUpdateRequired();
    Error::Enum updateMPD(DashComponents components);

    virtual Error::Enum startDownload(time_t startDownloadTime);
    virtual Error::Enum startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                 uint32_t aNextToBeProcessedSegmentId);
    virtual Error::Enum startDownloadFromTimestamp(uint32_t& overridePTS);
    virtual Error::Enum stopDownload();
    virtual Error::Enum stopDownloadAsync(bool_t aAbort, bool_t aReset);
    virtual void_t switchedToAnother();

    bool_t supportsSubSegments() const;
    Error::Enum hasSubSegmentsFor(uint64_t targetPtsUs, uint32_t overrideSegmentId);

    virtual void_t clearDownloadedContent(bool_t aResetInitialization = true);

    void processSegmentDownload();
    void_t processSegmentIndexDownload(uint32_t segmentId);

    virtual bool_t isDone();  // when all packets and segments are used up.. and not downloading more..

    uint32_t getWidth() const;
    uint32_t getHeight() const;
    float64_t getFramerate() const;
    uint32_t getBitrate() const;

    void_t assignQualityLevel(uint8_t aQualityIndex);
    uint8_t getQualityLevel() const;

    virtual bool_t setLatencyReq(LatencyRequirement::Enum aType);

    const SegmentContent& getSegmentContent() const;

    void setAssociatedToRepresentation(DashRepresentation* representation);
    MP4VRParser* getParserInstance();
    virtual void_t setLoopingOn();

public:
    virtual MP4VideoStreams& getCurrentVideoStreams();
    virtual MP4AudioStreams& getCurrentAudioStreams();
    virtual MP4MetadataStreams& getCurrentMetadataStreams();

    virtual Error::Enum readNextVideoFrame(bool_t& segmentChanged, int64_t currentTimeUs);
    virtual Error::Enum readNextAudioFrame(bool_t& segmentChanged);
    virtual Error::Enum readMetadataFrame(bool_t& segmentChanged, int64_t currentTimeUs);
    /**
     * Inform parser that new metadata packet was read and now would be good time to apply it to the internal state.
     */
    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream,
                                       const MP4VideoStream& refStream,
                                       const MP4VRMediaPacket& metadataFrame);

    virtual void_t createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel);
    virtual void_t createVideoSource(sourceid_t& sourceId,
                                     SourceType::Enum sourceType,
                                     StereoRole::Enum channel,
                                     const VASTileViewport& viewport);
    virtual void_t updateVideoSourceTo(SourcePosition::Enum aPosition);
    virtual const CoreProviderSources& getVideoSources();
    virtual const CoreProviderSourceTypes& getVideoSourceTypes();
    virtual void_t clearVideoSources();

    virtual uint64_t durationMs();
    virtual bool_t isSeekable();
    virtual Error::Enum seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs, uint32_t& aSeekSegmentIndex);

    virtual Error::Enum seekStreamsTo(uint64_t seekToPts);

    virtual bool_t isDownloading()
    {
        return mDownloading;
    }
    virtual bool_t isBuffering();
    virtual bool_t isEndOfStream();
    virtual bool_t isError();
    virtual void_t releaseOldSegments();
    virtual void_t releaseSegmentsUntil(uint32_t aSegmentId);

    virtual uint64_t getReadPositionUs(uint32_t& segmentIndex);
    virtual uint32_t getLastSegmentId(bool_t aIncludeSegmentInDownloading = false);
    virtual uint32_t getFirstSegmentId();
    virtual uint32_t getCurrentSegmentId();
    virtual bool_t isSegmentDurationFixed(uint64_t& segmentDurationMs);

    virtual uint32_t cachedSegments() const;

    virtual const char_t* getId() const;
    virtual const char_t* isAssociatedTo(const char_t* aAssociationType) const;

    void_t setCacheFillMode(bool_t autoFill);
    void_t setBufferingTime(uint32_t aBufferingTimeMs);

    virtual void_t setNotSupported();
    virtual bool_t getIsSupported();

    bool_t getIsInitialized() const;

    virtual DashSegment* peekSegment() const;
    virtual size_t getNrSegments(uint32_t aNextNeededSegment) const;
    virtual DashSegment* getSegment();
    virtual bool_t readyForSegment(uint32_t aId);
    virtual Error::Enum parseConcatenatedMediaSegment(DashSegment* aSegment);
    virtual void_t cleanUpOldSegments(uint32_t aNextSegmentId);
    virtual bool_t hasSegment(const uint32_t aSegmentId, uint32_t& aOldestSegmentId, size_t& aSegmentSize);

    virtual void_t setVideoStreamId(streamid_t aVideoStreamId);
    virtual streamid_t getVideoStreamId() const;

public:
    // DashSegmentStreamObserver
    Error::Enum onMediaSegmentDownloaded(DashSegment* segment, float32_t aSpeedFactor);
    void_t onSegmentIndexDownloaded(DashSegment* segment);
    void_t onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs);
    void_t onCacheWarning();

public:  // from MP4StreamCreator
    virtual MP4MediaStream* createStream(MediaFormat* aFormat) const;
    virtual MP4VideoStream* createVideoStream(MediaFormat* aFormat) const;
    virtual MP4AudioStream* createAudioStream(MediaFormat* aFormat) const;
    virtual void_t destroyStream(MP4MediaStream* aStream) const;


protected:
    virtual Error::Enum handleInputSegment(DashSegment* aSegment, bool_t& aReadyForReading);
    virtual void_t seekWhenSegmentAvailable();
    virtual void_t initializeVideoSource();

protected:
    DashAdaptationSet* mAdaptationSet;
    MemoryAllocator& mMemoryAllocator;
    DashRepresentationObserver* mObserver;
    MP4VideoStreams mVideoStreams;
    MP4AudioStreams mAudioStreams;
    MP4MetadataStreams mMetadataStreams;
    streamid_t mVideoStreamId;          // we assume 1 representation carries max 1 video stream
    VideoStreamMode::Enum mStreamMode;  // need to be saved until we have stream opened
    mutable Spinlock mLock;
    bool_t mInitializeIndependently;  // separate vs common init
    BasicSourceInfo mBasicSourceInfo;
    uint64_t mSeekToUsWhenDLComplete;
    bool_t mDownloading;
    DashSegmentStream* mSegmentStream;
    uint32_t mLastSegmentId;
    uint32_t mFirstSegmentId;
    DashRepresentation* mAssociatedToRepresentation;
    SegmentContent mSegmentContent;
    DashComponents mDashComponents;

private:
    dash::mpd::ISegmentTemplate* getSegmentTemplate(DashComponents dashComponents);

    void_t loadVideoMetadata();

private:
    bool_t mOwnsParserInstance;
    MP4VRParser* mParser;

    bool_t mInitialized;

    DashStreamType::Enum mStreamType;

    bool_t mMetadataCreated;
    uint32_t mWidth;
    uint32_t mHeight;
    float64_t mFps;
    bool_t mRestarted;
    bool_t mAutoFillCache;
    bool_t mSubSegmentSupported;
    bool_t mDownloadSubSegment;

    bool_t mIsSupported;  // the representation parameters fall within the supported range in this device
    uint32_t mBitrate;

    uint8_t mQualityLevel;
};
OMAF_NS_END
