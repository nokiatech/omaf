
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

#include "Foundation/NVRFixedString.h"
#include "Media/NVRMP4ParserDatatypes.h"
#include "NVRErrorCodes.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRPathName.h"
#include "Metadata/NVRViewpointUserAction.h"
#include "Provider/NVRCoreProvider.h"

OMAF_NS_BEGIN

extern const FixedString16 MPD_SUFFIX;
extern const FixedString16 MP4_SUFFIX;

extern const FixedString16 HTTP_PREFIX;

namespace VideoProviderState
{
    enum Enum
    {
        INVALID = -1,
        IDLE,
        LOADING,
        LOADED,
        PLAYING,
        BUFFERING,
        SWITCHING_VIEWPOINT,
        PAUSED,
        END_OF_FILE,
        STOPPED,
        CLOSING,
        STREAM_NOT_FOUND,
        CONNECTION_ERROR,
        STREAM_ERROR,

        COUNT
    };
    static const char_t* toString(VideoProviderState::Enum aState)
    {
        switch (aState)
        {
        case VideoProviderState::INVALID:
            return "INVALID";
        case VideoProviderState::IDLE:
            return "IDLE";
        case VideoProviderState::LOADING:
            return "LOADING";
        case VideoProviderState::LOADED:
            return "LOADED";
        case VideoProviderState::PLAYING:
            return "PLAYING";
        case VideoProviderState::BUFFERING:
            return "BUFFERING";
        case VideoProviderState::SWITCHING_VIEWPOINT:
            return "SWITCHING VIEWPOINT";
        case VideoProviderState::PAUSED:
            return "PAUSED";
        case VideoProviderState::END_OF_FILE:
            return "END_OF_FILE";
        case VideoProviderState::STOPPED:
            return "STOPPED";
        case VideoProviderState::CLOSING:
            return "CLOSING";
        case VideoProviderState::CONNECTION_ERROR:
            return "CONNECTION_ERROR";
        case VideoProviderState::STREAM_ERROR:
            return "STREAM_ERROR";
        case VideoProviderState::STREAM_NOT_FOUND:
            return "STREAM_NOT_FOUND";
        case VideoProviderState::COUNT:
            return "COUNT";
        default:
            return "";
        }
    }
}  // namespace VideoProviderState

class ProviderBase;
class RTPProvider;
class HLSProvider;

class VideoProvider : public CoreProvider
{
public:
    VideoProvider();
    virtual ~VideoProvider();

public:  // Inherited from CoreProvider
    virtual const CoreProviderSourceTypes& getSourceTypes();
    virtual Error::Enum prepareSources(HeadTransform currentHeadtransform,
                                       float32_t fovHorizontal,
                                       float32_t fovVertical);
    virtual const CoreProviderSources& getSources();

    virtual Error::Enum setAudioInputBuffer(AudioInputBuffer* inputBuffer);
    virtual void enter();
    virtual void leave();

public:  // new methods
    virtual VideoProviderState::Enum getState() const;
    virtual Error::Enum loadSource(const PathName& source, uint64_t initialPositionMS = 0);
    virtual const CoreProviderSources& getAllSources();

    virtual bool_t getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const;


    virtual Error::Enum start();
    virtual Error::Enum stop();
    virtual Error::Enum pause();

    // as small API as possible to prevent having 100 copy-paste delegate methods
    virtual const OverlayState overlayState(uint32_t ovlyId) const;
    virtual Error::Enum controlOverlay(const OverlayControl act);

    virtual Error::Enum next();
    virtual Error::Enum nextSourceGroup();
    virtual Error::Enum prevSourceGroup();

    virtual bool_t isSeekable();
    virtual Error::Enum seekToMs(uint64_t& aSeekMs, SeekAccuracy::Enum seekAccuracy);
    virtual Error::Enum seekToFrame(uint64_t frame);

    virtual uint64_t durationMs() const;
    virtual uint64_t elapsedTimeMs() const;

    virtual const Quaternion viewingOrientationOffset() const;
    virtual MediaInformation getMediaInformation() const;

private:
    bool_t isStreamingSource(const PathName& source) const;
    bool_t isOfflineVideoSource(const PathName& source) const;
    bool_t isOfflineImageSource(const PathName& source) const;

private:
    ProviderBase* mProviderImpl;

    // Some providers might change underlying resources while rendering loop is using them
    // this lock should be used when references to resources are used by anyone and should not
    // be modified
    Semaphore mResourcesInUse;

#if OMAF_ENABLE_STREAM_VIDEO_PROVIDER
    HLSProvider* mHLSProvider;
#if OMAF_ENABLE_RTP_VIDEO_PROVIDER
    RTPProvider* mRTPProvider;
#endif  // OMAF_ENABLE_STREAM_VIDEO_PROVIDER
#endif  // OMAF_ENABLE_RTP_VIDEO_PROVIDER

    // Used when there's no actual provider created
    CoreProviderSources mVideoSourcesEmpty;
    CoreProviderSourceTypes mVideoSourceTypesEmpty;

    AudioInputBuffer* mAudioInputBuffer;  // Not owned
};
OMAF_NS_END
