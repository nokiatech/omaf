
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

#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "Media/NVRMP4StreamManager.h"
#include "Metadata/NVRViewpointCoordSysRotation.h"
#include "Metadata/NVRViewpointUserAction.h"
#include "NVREssentials.h"
#include "VAS/NVRVASTileContainer.h"
#include "VAS/NVRVASTilePicker.h"


OMAF_NS_BEGIN
namespace VASType
{
    enum Enum
    {
        INVALID = -1,
        NONE,
        SUBPICTURES,
        EXTRACTOR_PRESELECTIONS_QUALITY,     // single resolution tiles, multiple qualities, linked with Preselections
        EXTRACTOR_DEPENDENCIES_QUALITY,      // extractor has the qualities & coverage, linked with dependencyId
        EXTRACTOR_PRESELECTIONS_RESOLUTION,  // multi-resolution tiles, linked with Preselections
        EXTRACTOR_DEPENDENCIES_RESOLUTION,   // multi-resolution tiles, linked with dependencyId
        COUNT
    };

}

class DashBitrateContoller;

namespace StreamMonitorType
{
    enum Enum
    {
        INVALID = -1,
        SELECT_SOURCES,
        ALL_SOURCES,
        GET_VIDEO_STREAMS,
        COUNT
    };
}

// A helper class to monitor stream changes between threads.
// It simplifies at least the setting part: single update() call will update all the flags.
// The flags are still specific to the monitoring client (many can be in renderer thread for different use cases).
//
// Would be even cleaner to have the flags like this:
// FixedArray<AtomicBoolean, StreamMonitorType::COUNT> mMonitors;
// But as AtomicBoolean need to have explicit initialization, how can we initialize such a list in the constructor's
// initializer list?
class StreamChangeMonitor
{
public:
    StreamChangeMonitor();
    ~StreamChangeMonitor();

    void_t update();
    bool_t hasUpdated(StreamMonitorType::Enum aType);

private:
    AtomicBoolean mVideoStreamsChanged;
    AtomicBoolean mNewVideoStreamsCreated;
    AtomicBoolean mVideoSourcesChanged;
};

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
    virtual const CoreProviderSources& getAllSources();
    // all parameters in degrees
    virtual const MP4VideoStreams& selectSources(float64_t longitude,
                                                 float64_t latitude,
                                                 float64_t roll,
                                                 float64_t width,
                                                 float64_t height,
                                                 bool_t& aViewportSet);

    virtual void_t release();
    virtual void_t clearDownloadedContent();

    virtual void_t getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrAdditionalCodecs);


    virtual Error::Enum startDownload(time_t aStartTime, uint32_t aBufferingTimeMs);
    virtual Error::Enum startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs);
    virtual Error::Enum stopDownload();
    virtual Error::Enum stopDownloadAsync(uint32_t& aLastTimeMs);

    virtual void_t updateStreams(uint64_t currentPlayTimeUs);
    virtual void_t processSegments();

    virtual const MP4VideoStreams& getVideoStreams();
    virtual const MP4MetadataStreams& getMetadataStreams();

    virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);

    virtual Error::Enum readMetadataFrame(int64_t currentTimeUs);

    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream, const MP4VRMediaPacket& metadataFrame);

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

    virtual const CoreProviderSources& getPastBackgroundVideoSourcesAt(uint64_t aPts);

    virtual Error::Enum completeInitialization(DashComponents& aDashComponents,
                                               SourceDirection::Enum aUriSourceDirection,
                                               sourceid_t aSourceIdBase);

    virtual void_t processSegmentDownload();

    virtual const Bitrates& getBitrates();
    virtual void_t
    setQualityLevels(uint8_t aFgQuality, uint8_t aMarginQuality, uint8_t aBgQuality, uint8_t aNrQualityLevels);
    virtual void_t selectBitrate(uint32_t bitrate);
    virtual bool_t isABRSwitchOngoing();
    virtual void_t increaseCache();

    virtual bool_t checkAssociation(DashAdaptationSet* aAdaptationSet, const RepresentationId& aAssociatedTo);
    virtual const char_t* findDashAssociationForStream(streamid_t aStreamId, const char_t* aAssociationType) const;
    virtual bool_t hasOverlayDashAssociation(DashAdaptationSet* aAdaptationSet, const char_t* aAssociationType);

    virtual bool_t switchOverlayVideoStream(streamid_t aVideoStreamId,
                                            int64_t currentTimeUs,
                                            bool_t& aPreviousOverlayStopped);
    virtual bool_t stopOverlayVideoStream(streamid_t aVideoStreamId);

    virtual bool_t getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;
    virtual const ViewpointSwitchingList& getViewpointSwitchConfig() const;

public:  // from AdaptationSetObserver
    void_t onNewStreamsCreated(MediaContent& aContent);
    void_t onDownloadProblem(IssueType::Enum aIssueType);
    void_t onActivateMe(DashAdaptationSet* aMe);

protected:
    virtual bool_t updateVideoStreams();
    virtual void_t checkOvlyStreamAssociations();
    virtual void_t doCheckOvlyStreamAssociations(MP4MediaStream* aMetadataStream,
                                                 const DashAdaptationSet& aMetaAdaptationSet);

protected:
    AdaptationSets mAdaptationSets;
    DashAdaptationSet* mVideoBaseAdaptationSet;
    AdaptationSets mOvlyAdaptationSets;
    AdaptationSets mActiveOvlyAdaptationSets;
    AdaptationSets mOvlyMetadataAdaptationSets;
    AdaptationSets mOvlyRvpoMetadataAdaptationSets;  // recommended viewport

    DashBitrateContoller* mBitrateController;
    VASTilePicker* mTilePicker;

    MediaInformation mMediaInformation;
    bool_t mStreamUpdateNeeded;
    bool_t mTileSetupDone;
    bool_t mReselectSources;
    uint32_t mBaseLayerDecoderPixelsinSec;
    MP4VideoStreams mCurrentVideoStreams;
    MP4VideoStreams mRenderingVideoStreams;
    const CoreProviderSourceTypes mVideoSourceTypesEmpty;
    MP4MetadataStreams mCurrentMetadataStreams;

    uint32_t mBufferingTimeMs;

    StreamChangeMonitor mStreamChangeMonitor;

    // this is at least initially needed in viewpoint switching if this downloader is stopped
    // but the content is still decoded, it should not switch viewport as that would restart
    // the downloading unnecessarily
    bool_t mRunning;
    uint32_t mBandwidthOverhead;
    bool_t mABREnabled;

private:
    DashStreamType::Enum mStreamType;

    CoreProviderSources mAllSources;
    bool_t mBuffering;
};
OMAF_NS_END
