
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

#include <IDASHManager.h>
#include <IMPD.h>

#include "Audio/NVRAudioInputBuffer.h"
#include "DashProvider/NVRDashAdaptationSet.h"
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashVideoDownloader.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVRDataBuffer.h"
#include "Foundation/NVREvent.h"
#include "Foundation/NVRHttp.h"
#include "Foundation/NVRPathName.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVRThread.h"
#include "Media/NVRMP4StreamManager.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN
namespace DashDownloadManagerState
{
    enum Enum
    {
        INVALID = -1,

        IDLE,
        INITIALIZING,
        INITIALIZED,
        DOWNLOADING,
        STOPPED,
        END_OF_STREAM,
        CONNECTION_ERROR,
        STREAM_ERROR,

        COUNT
    };
}
namespace SourcesOrigin
{
    enum Enum
    {
        INVALID = -1,
        FILENAME,
        METADATA,
        COUNT
    };
}

class DashDownloadManager : public DashAdaptationSetObserver, public MP4StreamManager
{
public:
    DashDownloadManager();
    virtual ~DashDownloadManager();

    void_t enableABR();
    void_t disableABR();
    void_t setBandwidthOverhead(uint32_t aBandwidthOverhead);

    uint32_t getCurrentBitrate();

    Error::Enum startDownload();
    Error::Enum stopDownload();
    Error::Enum stopDownloadCurrentViewpointAsync(uint32_t& aLastTimeMs);

    void_t clearDownloadedContent();

    Error::Enum init(const PathName& mediaURL, const BasicSourceInfo& sourceOverride);

    Error::Enum waitForInitializeCompletion();

    Error::Enum checkInitialize();

    void_t release();

    void_t updateStreams(uint64_t currentPlayTimeUs);
    void_t processSegments();

    DashDownloadManagerState::Enum getState();

    bool_t isDynamicStream();
    bool_t notReadyToGo();

    // called by renderer thread
    virtual const CoreProviderSources& getAllSources();

    // all parameters in degrees
    const MP4VideoStreams&
    selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height);

    bool_t
    setInitialViewport(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height);

    MediaInformation getMediaInformation();

    void_t getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrAdditionalCodecs);

    virtual Error::Enum setViewpoint(uint32_t aDestinationId);

    virtual bool_t isSeekable();
    virtual Error::Enum seekToMs(uint64_t& aSeekMs);

    virtual uint64_t durationMs() const;

    virtual bool_t isViewpointSwitchInitiated();
    virtual bool_t isViewpointSwitchReadyToComplete(uint32_t aCurrentPlayTimeMs);
    virtual void_t completeViewpointSwitch(bool_t& aAudioContinues);

public:  // From MP4StreamManager
    virtual void_t getAudioStreams(MP4AudioStreams& aMainAudioStreams, MP4AudioStreams& aAdditionalAudioStreams);
    virtual const MP4AudioStreams& getAudioStreams();
    virtual const MP4VideoStreams& getVideoStreams();
    virtual const MP4MetadataStreams& getMetadataStreams();

    virtual Error::Enum readVideoFrames(int64_t currentTimeUs);
    virtual Error::Enum readAudioFrames();
    virtual Error::Enum readMetadata(int64_t aCurrentTimeUs);

    virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream);
    virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);
    virtual MP4VRMediaPacket* getMetadataFrame(MP4MediaStream& stream, int64_t currentTimeUs);

    virtual const CoreProviderSourceTypes& getVideoSourceTypes();

    virtual bool_t stopOverlayAudioStream(streamid_t aVideoStreamId, streamid_t& aStoppedAudioStreamId);
    virtual MP4AudioStream* switchAudioStream(streamid_t aVideoStreamId,
                                              streamid_t& aStoppedAudioStreamId,
                                              bool_t aPreviousOverlayStopped);
    virtual bool_t switchOverlayVideoStream(streamid_t aVideoStreamId,
                                            int64_t currentTimeUs,
                                            bool_t& aPreviousOverlayStopped);
    virtual bool_t stopOverlayVideoStream(streamid_t aVideoStreamId);

    virtual Error::Enum updateMetadata(const MP4MediaStream& metadataStream, const MP4VRMediaPacket& metadataFrame);

    virtual bool_t getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;
    virtual size_t getNrOfViewpoints() const;
    virtual Error::Enum getViewpointId(size_t aIndex, uint32_t& aId) const;
    virtual size_t getCurrentViewpointIndex() const;

    virtual bool_t isBuffering();
    virtual bool_t isEOS() const;
    virtual bool_t isReadyToSignalEoS(MP4MediaStream& aStream) const;

    virtual const CoreProviderSources& getPastBackgroundVideoSourcesAt(uint64_t aPts);

public:  // from AdaptationSetObserver
    void_t onNewStreamsCreated(MediaContent& aContent);
    void_t onDownloadProblem(IssueType::Enum aIssueType);
    void_t onActivateMe(DashAdaptationSet* aMe);

private:
	/// Shared context that is used by all MP4Parsers to pass information for example
	/// of which stream represents certain track id / track group id
    ParserContext mParserCtx;

    struct DashViewpoint
    {
        DashViewpoint()
            : videoDownloader(OMAF_NULL)
            , backgroundAudioAdaptationSet(OMAF_NULL)
            , invoMetadataAdaptationSet(OMAF_NULL)
        {
        }

        DashVideoDownloader* videoDownloader;
        AdaptationSets nonVideoAdaptationSets;
        DashAdaptationSet* backgroundAudioAdaptationSet;
        AdaptationSets activeAudioAdaptationSets;
        AdaptationSets allAudioAdaptationSets;
        DashAdaptationSet* invoMetadataAdaptationSet;
    };


    Error::Enum completeInitialization();
    Error::Enum initializeViewpoint(DashPeriodViewpoint* aViewpoint,
                                    bool_t aRequireVideo,
                                    SourceDirection::Enum aSourceDirection);
    void_t releaseViewpoint(DashViewpoint*& aViewpoint);
    bool_t isVideoAndAudioMuxed(DashViewpoint* aViewpoint);
    Error::Enum startDownload(DashViewpoint* aViewpoint, uint32_t& aTargetTimeMs);

    void_t doClearDownloadedContent(DashViewpoint* aViewpoint);

    Error::Enum updateMPD(bool_t synchronous);

    void_t processSegmentDownload(DashViewpoint* aViewpoint);

    DashAdaptationSet* findAssociatedAdaptationSet(DashViewpoint* aViewpoint, streamid_t aStreamId);

    void_t doGetAudioStreams(DashViewpoint* aViewpoint,
                             MP4AudioStreams& aMainAudioStreams,
                             MP4AudioStreams& aAdditionalAudioStreams);
    void_t doGetAudioStreams(DashViewpoint* aViewpoint);
    const MP4VideoStreams& doGetVideoStreams(DashViewpoint* aViewpoint);
    void_t doGetMetadataStreams(DashViewpoint* aViewpoint);

    bool_t doStopOverlayAudioStream(DashViewpoint* aViewpoint,
                                    streamid_t aVideoStreamId,
                                    streamid_t& aStoppedAudioStreamId);
    MP4AudioStream* doSwitchAudioStream(DashViewpoint* aViewpoint,
                                        streamid_t aVideoStreamId,
                                        streamid_t& aStoppedAudioStreamId,
                                        bool_t aPreviousOverlayStopped);

    Error::Enum doSeekToMs(DashViewpoint* aViewpoint, uint64_t& aSeekMs);

    sourceid_t getNextSourceIdBase();
    void_t updateBandwidth();

private:
    dash::mpd::IMPD* mMPD;
    dash::IDASHManager* mDashManager;

    FixedArray<DashPeriodViewpoint*, 64> mViewpointPeriods;
    size_t mCurrentVPPeriodIndex;

    AtomicBoolean mVideoDownloaderCreated;
    Event mViewportSetEvent;
    AtomicBoolean mViewportConsumed;

    // Current viewpoint, used as source for decoding / rendering
    DashViewpoint* mCurrentViewpoint;
    // Viewpoint in init phase in mViewpointInitThread; expects viewport info from renderer
    DashViewpoint* mInitializingViewpoint;
    // Viewpoint that is downloaded, can be the same as mCurrentViewpoint if switch not active
    DashViewpoint* mDownloadingViewpoint;
    // The time until the current viewpoint is played before switching.
    // It may or may not be the same as the start time of the new viewpoint.
    uint32_t mTargetSwitchTimeMs;
    DashViewpoint* mCommonToViewpoints;  // not actually a viewpoint at all

    MP4AudioStreams mCurrentAudioStreams;
    MP4MetadataStreams mCurrentMetadataStreams;

    DashDownloadManagerState::Enum mState;
    DashStreamType::Enum mStreamType;

    PathName mMPDPath;

    int32_t mMPDUpdateTimeMS;
    uint32_t mMPDUpdatePeriodMS;
    std::string mPublishTime;

    bool_t mMetadataLoaded;

    MediaInformation mMediaInformation;

    BasicSourceInfo mSourceOverride;
    sourceid_t mNextSourceIdBase;

    CoreProviderSourceTypes mVideoSourceTypes;
    CoreProviderSources mVideoSources;
    MP4VideoStreams mVideoStreams;

    mutable Spinlock mViewpointSwitchLock;

    class MPDDownloadThread
    {
    public:
        MPDDownloadThread(dash::IDASHManager* aDashManager);
        ~MPDDownloadThread();
        bool_t setUri(const PathName& aUri);
        void trigger();
        void waitCompletion();
        bool isBusy();
        bool isReady();
        dash::mpd::IMPD* getResult(Error::Enum& aResult);

    private:
        MemoryAllocator& mAllocator;
        DataBuffer<uint8_t> mData;
        HttpConnection* mConnection;
        PathName mMPDPath;
        dash::IDASHManager* mDashManager;
        uint64_t mStart;
        bool_t mBusy;
        uint32_t mMpdDownloadTimeMs;
        friend class DashDownloadManager;
    };
    MPDDownloadThread* mMPDUpdater;

    class ViewpointInitThread
    {
    public:
        ViewpointInitThread(DashDownloadManager& aHost);
        ~ViewpointInitThread();

        Error::Enum start(uint32_t aStartTimeMs);

    private:
        Thread::ReturnValue viewpointThreadEntry(const Thread& thread, void_t* userData);

    private:
        DashDownloadManager& mHost;
        Thread mViewpointInitThread;
        Event mTrigger;
        bool_t mRunning;
        uint32_t mStartTimeMs;
    };
    ViewpointInitThread mViewpointInitThread;
};
OMAF_NS_END
