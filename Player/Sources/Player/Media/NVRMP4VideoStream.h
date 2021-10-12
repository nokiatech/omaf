
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

#include <reader/mp4vrfiledatatypes.h>

#include "Media/NVRMP4MediaStream.h"
#include "NVRNamespace.h"
#include "Platform/OMAFDataTypes.h"
#include "VideoDecoder/NVRVideoDecoderConfig.h"

OMAF_NS_BEGIN

class MediaPacket;
class MediaFormat;
class VideoDecoderManager;

namespace VideoStreamMode
{
    enum Enum
    {
        INVALID = -1,
        BACKGROUND,
        OVERLAY,
        DISABLED,

        COUNT
    };

    static const char_t* toString(VideoStreamMode::Enum aValue)
    {
        switch (aValue)
        {
        case INVALID:
            return "INVALID";
        case BACKGROUND:
            return "BACKGROUND";
        case OVERLAY:
            return "OVERLAY";
        case DISABLED:
            return "DISABLED";
        case COUNT:
            return "COUNT";
        default:
            return "";
        }
    }
}  // namespace VideoStreamMode

class MediaPacket;
class MediaFormat;
class VideoDecoderManager;

class MP4VideoStream : public MP4MediaStream
{
public:
    MP4VideoStream(MediaFormat* format);
    virtual ~MP4VideoStream();

    virtual bool_t updateTrack(MP4VR::TrackInformation* track);

    virtual bool_t isEoF() const;

    virtual bool_t hasAlternates() const;
    virtual bool_t isMetadataUpdated(int64_t framePtsUs);

    virtual bool_t isFrameLate(uint64_t sampleTimeMs, uint64_t currentTimeMs);

    const DecoderConfig& getDecoderConfig();

    virtual void_t setStreamId(streamid_t id);
    virtual void_t setMode(VideoStreamMode::Enum mode);
    virtual VideoStreamMode::Enum getMode() const;

    virtual void_t clearVideoSources();
    virtual void_t setVideoSources(const CoreProviderSources& videoSources, uint64_t aValidFromPts = 0);
    virtual bool_t hasVideoSources() const;
    virtual const CoreProviderSources& getVideoSources() const;
    virtual const CoreProviderSources& getVideoSources(uint64_t& aValidFromPts) const;

    virtual void_t dataAvailable();

    virtual bool_t isUserActivateable();


protected:
    VideoDecoderManager* mDecoder;
    bool_t mSelected;
    int64_t mLastDecodedPTSUs;

    bool_t mCheckFrameTiming;
    uint32_t mLate;

    DecoderConfig mDecoderConfig;
    VideoStreamMode::Enum mStreamMode;

    CoreProviderSources mVideoSources;
    uint64_t mSourcesValidFromPts;

    uint64_t mDLStartTimeMs;
    uint64_t mFFStartTimeMs;
};

typedef FixedArray<MP4VideoStream*, 256> MP4VideoStreams;

OMAF_NS_END
