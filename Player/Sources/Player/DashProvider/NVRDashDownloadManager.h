
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

#include <IMPD.h>
#include <IDASHManager.h>

#include "NVREssentials.h"
#include "Foundation/NVREvent.h"
#include "Foundation/NVRThread.h"
#include "Foundation/NVRHttp.h"
#include "Foundation/NVRDataBuffer.h"
#include "Audio/NVRAudioInputBuffer.h"
#include "DashProvider/NVRDashAdaptationSet.h"
#include "VAS/NVRVASTilePicker.h"
#include "VAS/NVRVASTileContainer.h"
#include "Media/NVRMP4StreamManager.h"
#include "Foundation/NVRPathName.h"
#include "Foundation/NVRAtomicBoolean.h"
#include "DashProvider/NVRDashBitrateController.h"


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

        void_t clearDownloadedContent();

        Error::Enum init(const PathName &mediaURL, const BasicSourceInfo& sourceOverride);

        Error::Enum waitForInitializeCompletion();

        Error::Enum checkInitialize();

        void_t release();

        void_t updateStreams(uint64_t currentPlayTimeUs);

        DashDownloadManagerState::Enum getState();

        bool_t isVideoAndAudioMuxed();

        bool_t isDynamicStream();
        
        // called by renderer thread
        // all parameters in degrees
        const MP4VideoStreams& selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId);

        virtual bool_t hasMPDVideoMetadata() const;

        MediaInformation getMediaInformation();

        void_t getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs);

    public:

        virtual const MP4AudioStreams& getAudioStreams();
        virtual const MP4VideoStreams& getVideoStreams();
        virtual MP4VRMediaPacket* getNextAudioFrame(MP4MediaStream& stream);
        virtual MP4VRMediaPacket* getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs);


        virtual Error::Enum readVideoFrames(int64_t currentTimeUs);
        virtual Error::Enum readAudioFrames();

        virtual const CoreProviderSourceTypes& getVideoSourceTypes();

        virtual bool_t isSeekable();
        virtual Error::Enum seekToMs(uint64_t& aSeekMs);

        virtual uint64_t durationMs() const;

        virtual bool_t isBuffering();
        virtual bool_t isEOS() const;
        virtual bool_t isReadyToSwitch(MP4MediaStream& aStream) const;

    public: // from AdaptationSetObserver
        void_t onNewStreamsCreated();
        void_t onDownloadProblem(IssueType::Enum aIssueType);

    private:

        Error::Enum completeInitialization();

        Error::Enum updateMPD(bool_t synchronous);

        void_t processSegmentDownload();
        bool_t updateVideoStreams();
        void_t checkVASVideoStreams(uint64_t currentPTS);

    private:
        AdaptationSets mAdaptationSets;
        dash::mpd::IMPD *mMPD;
        dash::IDASHManager *mDashManager;
        DashBitrateContoller mBitrateController;

        DashAdaptationSet* mVideoBaseAdaptationSet; 
        DashAdaptationSet* mVideoBaseAdaptationSetStereo;
        // The two tile-lists are alternatives: first is used with base layer + enhancement layer scheme, the 2nd with OMAF HEVC tiles
        VASTilesLayer mVideoEnhTiles;
        VASTilesLayer mVideoPartialTiles;

        DashAdaptationSet *mAudioAdaptationSet;
        DashAdaptationSet *mAudioMetadataAdaptationSet;

        DashDownloadManagerState::Enum mState;
        DashStreamType::Enum mStreamType;

        PathName mMPDPath;

        int32_t mMPDUpdateTimeMS;
        uint32_t mMPDUpdatePeriodMS;
        std::string mPublishTime;
        VASTilePicker* mTilePicker;
        MP4VideoStreams mCurrentVideoStreams;
        MP4VideoStreams mRenderingVideoStreams;
        MP4AudioStreams mCurrentAudioStreams;

        CoreProviderSourceTypes mVideoSourceTypes;
        bool_t mMetadataLoaded;
        AtomicBoolean mNewVideoStreamsCreated;
        AtomicBoolean mVideoStreamsChanged;
        bool_t mStreamUpdateNeeded;
        bool_t mGlobalSubSegmentsSupported;

        MediaInformation mMediaInformation;

        uint32_t mPlaybackStartTimeMs;
        bool_t mReselectSources;
        BasicSourceInfo mSourceOverride;
        class MPDDownloadThread
        {
        public:
            MPDDownloadThread(dash::IDASHManager *aDashManager);
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
            dash::IDASHManager *mDashManager;
            uint64_t mStart;
            bool_t mBusy;
        };
        MPDDownloadThread* mMPDUpdater;
        bool_t mABREnabled;
        uint32_t mBandwidthOverhead;
        uint32_t mBaseLayerDecoderPixelsinSec;
        bool_t mTileSetupDone;
        VASType::Enum mVASType;
    };
OMAF_NS_END
