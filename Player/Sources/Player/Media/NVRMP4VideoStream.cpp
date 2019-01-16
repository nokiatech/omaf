
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
#include "Media/NVRMP4VideoStream.h"

#include "VideoDecoder/NVRVideoDecoderManager.h"

#include "Media/NVRMediaFormat.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRLogger.h"
#include "VAS/NVRLatencyLog.h"

OMAF_NS_BEGIN

#if OMAF_PLATFORM_ANDROID
// On Android we typically need > 200 ms lead time for decoding a stream with P and B frames. On fast devices the time can be occassionally even > 500 ms.
// On Windows the lead times seem to be > 1 sec on powerful PC, but there is no experience what the critical level may be.
const int64_t kAvgLeadTimeMs = 200;
#else
const int64_t kAvgLeadTimeMs = -200;
#endif

    OMAF_LOG_ZONE(MP4VideoStream)

    MP4VideoStream::MP4VideoStream(MediaFormat* format)
    : MP4MediaStream(format)
    , mDecoder(OMAF_NULL)
    , mSelected(true)
    , mLastDecodedPTSUs(-1)
    , mCheckFrameTiming(false)
    , mLate(0)
    , mStreamMode(VideoStreamMode::BASE)
    , mDLStartTimeMs(0)
    , mFFStartTimeMs(0)
    {

        mDecoderConfig.mimeType = format->getMimeType();
        mDecoderConfig.width = format->width();
        mDecoderConfig.height = format->height();

        size_t infoSize = 0;
        uint8_t* infoBuffer = mFormat->getDecoderConfigInfo(infoSize);
        mDecoderConfig.configInfoSize = infoSize;
        MemoryCopy(mDecoderConfig.configInfoData, infoBuffer, mDecoderConfig.configInfoSize);

        const MediaFormat::DecoderConfigMap& configInfo = format->getDecoderConfigInfoMap();
        MediaFormat::DecoderConfigMap::ConstIterator it = configInfo.find(MediaFormat::HEVC_VPS);
        if (it != MediaFormat::DecoderConfigMap::InvalidIterator)
        {
            mDecoderConfig.vpsSize = (*it)->getSize();
            MemoryCopy(mDecoderConfig.vpsData, (*it)->getDataPtr(), mDecoderConfig.vpsSize);
        }
        it = configInfo.find(MediaFormat::AVC_HEVC_SPS);
        if (it != MediaFormat::DecoderConfigMap::InvalidIterator)
        {
            mDecoderConfig.spsSize = (*it)->getSize();
            MemoryCopy(mDecoderConfig.spsData, (*it)->getDataPtr(), mDecoderConfig.spsSize);
        }
        it = configInfo.find(MediaFormat::AVC_HEVC_PPS);
        if (it != MediaFormat::DecoderConfigMap::InvalidIterator)
        {
            mDecoderConfig.ppsSize = (*it)->getSize();
            MemoryCopy(mDecoderConfig.ppsData, (*it)->getDataPtr(), mDecoderConfig.ppsSize);
        }
        mIsVideo = true;
        mIsAudio = false;
        // if the device can do 2 video tracks, it's perf has been verified and should not be on the limits, so don't check
        mCheckFrameTiming = !DeviceInfo::deviceSupports2VideoTracks();
    }

    MP4VideoStream::~MP4VideoStream()
    {
        if (mDecoder)
        {
            mDecoder->shutdownStream(mStreamId);
        }
    }

    void_t MP4VideoStream::setStreamId(streamid_t id)
    {
        mDecoder = VideoDecoderManager::getInstance();
        mDecoderConfig.streamId = id;
        mDecoder->initializeStream(id, mDecoderConfig);
        MP4MediaStream::setStreamId(id);
    }

    const DecoderConfig& MP4VideoStream::getDecoderConfig()
    {
        return mDecoderConfig;
    }

    // EoF or end of data in available segments
    bool_t MP4VideoStream::isEoF() const
    {
        if (mDecoder != OMAF_NULL)
        {
            return mDecoder->isEOS(mStreamId);
        }
        return false;
    }

    bool_t MP4VideoStream::matchAndAssignMetadataStream(MP4MediaStream* stream, MP4VR::DynArray<uint32_t> ids)
    {
        for (uint32_t i = 0; i < ids.size; i++)
        {
            if (getTrackId() == ids[i])
            {
                setMetadataStream(stream);
                return true;
            }
        }
        return false;
    }

    bool_t MP4VideoStream::updateMetadataStream(MP4VR::TrackInformation* track, MP4VR::MP4VRFileReaderInterface* reader)
    {
        MP4MediaStream* stream = getMetadataStream();
        if (stream != OMAF_NULL && stream->getTrackId() == track->trackId)
        {
            getMetadataStream()->setTrack(track);
            getMetadataStream()->setTimestamps(reader);
            return true;
        }
        return false;
    }

    bool_t MP4VideoStream::updateTrack(MP4VR::TrackInformation* track)
    {
        if (track == OMAF_NULL || getTrackId() == track->trackId)
        {
            setTrack(track);
            return true;
        }
        return false;
    }


    bool_t MP4VideoStream::hasAlternates() const
    {
        return false;
    }

    bool_t MP4VideoStream::isMetadataUpdated(int64_t framePtsUs)
    {
        OMAF_UNUSED_VARIABLE(framePtsUs);

        return false;
    }

    bool_t MP4VideoStream::isFrameLate(uint64_t sampleTimeMs, uint64_t currentTimeMs)
    {
        if (mStreamMode == VideoStreamMode::ENHANCEMENT_FAST_FORWARD)
        {
            if (currentTimeMs + kAvgLeadTimeMs > sampleTimeMs)
            {
                return true;
            }
        }
        else if ((currentTimeMs+kAvgLeadTimeMs) > 0 && mCheckFrameTiming)
        {
            int64_t currentLeadTimeMs = (int64_t)sampleTimeMs - (int64_t)currentTimeMs;
            //OMAF_LOG_D("read sample time %lld - current time %lld = %lld  avg %lld", sampleTimeMs, currentTimeMs, currentLeadTimeMs, kAvgLeadTimeMs);

            // PTS for the read frames should have a lead time, which is platform specific; > 0 is checked above
            uint64_t oldestAcceptedTimeMs = (uint64_t)(currentTimeMs + kAvgLeadTimeMs);

            if (sampleTimeMs < oldestAcceptedTimeMs)
            {
                OMAF_LOG_D("sample is late: %lld - %lld = %lld", sampleTimeMs, currentTimeMs, currentLeadTimeMs);
                mLate++;
                if (mLate > 2)
                {
                    OMAF_LOG_D("%d consecutive samples are late", mLate);
                    return true;
                }
            }
            else
            {
                mLate = 0;
            }
        }
        return false;
    }

    void_t MP4VideoStream::setMode(VideoStreamMode::Enum mode)
    {
        OMAF_LOG_D("setMode of %d to %s", mStreamId, VideoStreamMode::toString(mode));
        if (mode == VideoStreamMode::ENHANCEMENT_FAST_FORWARD)
        {
            // we have just decided that we need this stream, after user has turned her head
            mDLStartTimeMs = Time::getClockTimeMs();
        }
        else if (mStreamMode == VideoStreamMode::ENHANCEMENT_FAST_FORWARD && mode == VideoStreamMode::ENHANCEMENT_NORMAL && mFFStartTimeMs > 0)
        {
            // now we have catched the playhead
            uint64_t spend = Time::getClockTimeMs() - mFFStartTimeMs;
            //OMAF_LOG_D("Stream %d\tcatching playhead took\t%llu ms", mStreamId, spend);
            LATENCY_LOG_D("Stream %d\tcatching playhead took\t%llu ms", mStreamId, spend);
            if (spend > 1000)
            {
                OMAF_LOG_V("Catching playhead SLOW: %llu", spend);
            }
        }

        if (mStreamMode == VideoStreamMode::DISABLED && mode == VideoStreamMode::ENHANCEMENT_NORMAL)
        {
            // not possible to switch from disabled to normal
            // normal mode can be entered via ff mode only. 
            return;
        }
        mStreamMode = mode;
    }

    VideoStreamMode::Enum MP4VideoStream::getMode() const
    {
        return mStreamMode;
    }

    void_t MP4VideoStream::dataAvailable()
    {
        if (mDLStartTimeMs > 0)
        {
            mFFStartTimeMs = Time::getClockTimeMs();
            uint64_t spend = Time::getClockTimeMs() - mDLStartTimeMs;
            mDLStartTimeMs = 0;
            //OMAF_LOG_D("Stream %d\tsegment download took\t%llu ms", mStreamId, spend);
            LATENCY_LOG_D("Stream %d\tsegment download took\t%llu ms", mStreamId, spend);
        }
    }


    void_t MP4VideoStream::clearVideoSources()
    {
        mVideoSources.clear();
    }

    void_t MP4VideoStream::setVideoSources(const CoreProviderSources& videoSources, uint64_t aValidFromPts)
    {
        if (!mVideoSources.isEmpty())
        {
            //Should we do something else here?, could the sources actually change?
            mVideoSources.clear();
        }
        mSourcesValidFromPts = aValidFromPts;
        mVideoSources.add(videoSources);
    }

    bool_t MP4VideoStream::hasVideoSources() const
    {
        return !mVideoSources.isEmpty();
    }

    const CoreProviderSources& MP4VideoStream::getVideoSources() const
    {
        return mVideoSources;
    }

    const CoreProviderSources& MP4VideoStream::getVideoSources(uint64_t& aValidFromPts) const
    {
        aValidFromPts = mSourcesValidFromPts;
        return getVideoSources();
    }

OMAF_NS_END
