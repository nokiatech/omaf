
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

#include "Audio/NVRAudioInputBuffer.h"
#include "Media/NVRMP4StreamManager.h"
#include "NVREssentials.h"
#include "Provider/NVRVideoProvider.h"

#include "Provider/NVRSynchronizer.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

#include "Foundation/NVRAtomicBoolean.h"
#include "Foundation/NVREvent.h"
#include "Foundation/NVRMutex.h"
#include "Foundation/NVRPathName.h"
#include "Foundation/NVRSemaphore.h"
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

namespace PendingUserAction
{
    enum Enum
    {
        INVALID,

        PLAY,
        PAUSE,
        STOP,
        CONTROL_OVERLAY,

        COUNT
    };
}

namespace PacketProcessingResult
{
    enum Enum
    {
        INVALID,

        OK,
        BUFFERING,
        END_OF_FILE,
        ERROR,

        COUNT
    };
}

const static int32_t AUDIO_REQUEST_THRESHOLD = 5;
const static int32_t MIN_AUDIO_BUFFER_LEVEL = 50;

namespace
{
    static const int ASYNC_OPERATION_TIMEOUT = 500000;
}

class ProviderBase : public AudioInputBufferObserver
{
public:
    ProviderBase();
    virtual ~ProviderBase();

public:  // CoreProvider
    virtual const CoreProviderSourceTypes& getSourceTypes() = 0;
    virtual Error::Enum prepareSources(HeadTransform currentHeadtransform,
                                       float32_t fovHorizontal,
                                       float32_t fovVertical);
    virtual const CoreProviderSources& getSources() const;

    virtual Error::Enum setAudioInputBuffer(AudioInputBuffer* inputBuffer);
    virtual void enter();
    virtual void leave();

public:  // VideoProvider
    virtual VideoProviderState::Enum getState() const;
    virtual Error::Enum loadSource(const PathName& source, uint64_t initialPositionMS);
    virtual const CoreProviderSources& getAllSources() const;
    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;

    virtual Error::Enum start();
    virtual Error::Enum stop();
    virtual Error::Enum pause();

    virtual const OverlayState overlayState(uint32_t ovlyId) const;
    virtual Error::Enum controlOverlay(const OverlayControl act);

    virtual Error::Enum next();
    virtual Error::Enum nextSourceGroup();
    virtual Error::Enum prevSourceGroup();

    virtual bool_t isSeekable() = 0;
    virtual bool_t isSeekableByFrame() = 0;
    virtual Error::Enum seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy);
    virtual Error::Enum seekToFrame(uint64_t frame);

    virtual uint64_t durationMs() const = 0;
    virtual uint64_t elapsedTimeMs() const;

    virtual const MediaInformation& getMediaInformation();

public:  // AudioInputBufferObserver
    virtual void_t onPlaybackStarted(AudioInputBuffer* caller, int64_t bufferingOffset);
    virtual void_t onSamplesConsumed(AudioInputBuffer* caller, streamid_t streamId);
    virtual void_t onOutOfSamples(AudioInputBuffer* caller, streamid_t streamId);
    virtual void_t onAudioErrorOccurred(AudioInputBuffer* caller, Error::Enum error);

    virtual const Quaternion viewingOrientationOffset() const;

protected:
    void_t destroyInstance();

    virtual uint64_t selectSources(HeadTransform headTransform,
                                   float32_t fovHorizontal,
                                   float32_t fovVertical,
                                   CoreProviderSources& required,
                                   CoreProviderSources& optional) = 0;
    virtual bool_t setInitialViewport(HeadTransform headTransform, float32_t fovHorizontal, float32_t fovVertical);
    virtual const CoreProviderSources& getPastBackgroundSources(uint64_t aPts);

    virtual MP4AudioStream* getAudioStream() = 0;

    Streams extractStreamsFromSources(const CoreProviderSources& sources);

    virtual Error::Enum handleOverlayControl(const OverlayControl act);

protected:  // Called by child classes
    void_t startPlayback(bool_t waitForAudio = true);
    void_t stopPlayback();

    void_t setState(VideoProviderState::Enum state);

    void_t initializeAudioFromMP4(const MP4AudioStreams& aMainAudioStreams,
                                  const MP4AudioStreams& aAdditionalAudioStreams);
    void_t addAudioStreams(const MP4AudioStreams& aAdditionalAudioStreams);

    PacketProcessingResult::Enum processMP4Video();

    PacketProcessingResult::Enum processMP4Audio(AudioInputBuffer* audioInputBuffer);

    /**
     * Handle dynamic changes in overlay metadata.
     *
     * Dynamic changes can be setting active overlays, overriding certain parameters and
     * completely creating new temporary overlays
     *
     * Base info about of overlays is already read in MP4Parser::readInitialMetadata from
     * video tracks povd box.
     */
    PacketProcessingResult::Enum processOverlayMetadataStream();

    // called by provider thread
    bool_t retrieveInitialViewingOrientation(int64_t aCurrentTimeUs);

    OverlaySource* getOverlaySource(uint32_t ovlyId) const;

    virtual Error::Enum controlOverlayStream(streamid_t aVideoStreamId, bool_t aEnable);

private:
    virtual void_t parserThreadCallback() = 0;

    Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);

    // called by renderer thread
    void_t setupInitialViewingOrientation(HeadTransform& aCurrentHeadTransform, uint64_t aTimestampUs);

    // Some providers might change underlying resources while rendering loop is using them
    // this lock should be used when references to resources are used by anyone and should not
    // be modified
    Semaphore mResourcesInUse;

protected:
    Thread mParserThread;
    PathName mSourceURI;

    CoreProviderSources mPreparedSources;
    Synchronizer mSynchronizer;

    MP4StreamManager* mMediaStreamManager;

    // reference, not owned
    AudioInputBuffer* mAudioInputBuffer;
    VideoDecoderManager* mVideoDecoder;

    Event mUserActionSync;
    Mutex mPendingUserActionMutex;
    PendingUserAction::Enum mPendingUserAction;
    OverlayControl mPendingOverlayControl;

    uint64_t mStreamInitialPositionMS;

    SeekAccuracy::Enum mSeekAccuracy;
    uint64_t mSeekTargetUs;
    uint64_t mSeekTargetFrame;
    uint64_t mSeekResultMs;

    Event mSeekSyncronizeEvent;
    Error::Enum mSeekResult;

    Event mParserThreadControlEvent;
    bool_t mVideoDecoderFull;
    bool_t mAudioUsed;
    bool_t mAudioFull;
    bool_t mOverlayAudioPlayPending;

    bool_t mFirstVideoPacketRead;
    uint64_t mFirstFramePTSUs;
    uint64_t mFirstDisplayFramePTSUs;

    uint64_t mElapsedTimeUs;

    int32_t mSelectedSourceGroup;

    Streams mPreviousStreams;

    MediaInformation mMediaInformation;

    mutable Spinlock mStreamLock;

    struct RetrievedOrientation
    {
        int32_t cAzimuth;
        int32_t cElevation;
        int32_t cTilt;
        bool_t refresh;

        bool_t valid;
        uint64_t timestampUs;
        streamid_t streamId;
        Spinlock lock;

        RetrievedOrientation()
            : valid(false)
            , timestampUs(0)
            , refresh(true)
            , streamId(OMAF_UINT8_MAX)
        {
        }
    };
    RetrievedOrientation mLatestSignaledViewingOrientation;

    AtomicBoolean mSignalViewport;
    AtomicBoolean mSwitchViewpoint;
    uint32_t mNextViewpointId;

private:
    uint32_t mBufferingStartTimeMS;
    VideoProviderState::Enum mState;
    ForcedOrientation mViewingOrientationOffset;

    ViewpointGlobalCoordinateSysRotation mViewpointCoordSysRotation;
    AtomicBoolean mCoordSysRotationUpdated;
};


OMAF_NS_END
