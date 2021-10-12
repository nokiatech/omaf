
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
#include "DashProvider/NVRDashVideoDownloader.h"
#include "DashProvider/NVRDashAdaptationSetAudio.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashAdaptationSetOverlay.h"
#include "DashProvider/NVRDashAdaptationSetOverlayMeta.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashMediaStream.h"
#include "DashProvider/NVRDashSpeedFactorMonitor.h"
#include "Media/NVRMediaPacket.h"
#include "Metadata/NVRMetadataParser.h"

#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRTime.h"
#include "VAS/NVRVASTilePicker.h"
#include "VAS/NVRVASTileSetPicker.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashVideoDownloader)


StreamChangeMonitor::StreamChangeMonitor()
    : mVideoSourcesChanged(false)
    , mVideoStreamsChanged(false)
    , mNewVideoStreamsCreated(false)
{
}

StreamChangeMonitor::~StreamChangeMonitor()
{
}

void_t StreamChangeMonitor::update()
{
    mVideoStreamsChanged = true;
    mNewVideoStreamsCreated = true;
    mVideoSourcesChanged = true;
}

bool_t StreamChangeMonitor::hasUpdated(StreamMonitorType::Enum aType)
{
    switch (aType)
    {
    case StreamMonitorType::SELECT_SOURCES:
        return mVideoStreamsChanged.compareAndSet(false, true);
        break;
    case StreamMonitorType::ALL_SOURCES:
        return mVideoSourcesChanged.compareAndSet(false, true);
        break;
    case StreamMonitorType::GET_VIDEO_STREAMS:
        return mNewVideoStreamsCreated.compareAndSet(false, true);
        break;
    }
    return false;
}


DashVideoDownloader::DashVideoDownloader()
    : mVideoBaseAdaptationSet(OMAF_NULL)
    , mBitrateController(OMAF_NULL)
    , mStreamType(DashStreamType::INVALID)
    , mTilePicker(OMAF_NULL)
    , mCurrentVideoStreams()
    , mStreamUpdateNeeded(false)
    , mReselectSources(false)
    , mABREnabled(true)
    , mBandwidthOverhead(0)
    , mBaseLayerDecoderPixelsinSec(0)
    , mTileSetupDone(false)
    , mBufferingTimeMs(0)
    , mBuffering(false)
{
}

DashVideoDownloader::~DashVideoDownloader()
{
    release();
}

void_t DashVideoDownloader::enableABR()
{
    mABREnabled = true;
}


void_t DashVideoDownloader::disableABR()
{
    mABREnabled = false;
}

void_t DashVideoDownloader::setBandwidthOverhead(uint32_t aBandwithOverhead)
{
    mBandwidthOverhead = aBandwithOverhead;
    for (DashAdaptationSet* ovly : mActiveOvlyAdaptationSets)
    {
        mBandwidthOverhead += ovly->getCurrentBandwidth();
    }
}

uint32_t DashVideoDownloader::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSet->getCurrentBandwidth();
    }
    for (DashAdaptationSet* ovly : mActiveOvlyAdaptationSets)
    {
        bitrate += ovly->getCurrentBandwidth();
    }

    return bitrate;
}

void_t DashVideoDownloader::getMediaInformation(MediaInformation& aMediaInformation)
{
    aMediaInformation.videoType = mMediaInformation.videoType;
    aMediaInformation.isStereoscopic = mMediaInformation.isStereoscopic;
    aMediaInformation.durationUs = 1000 * mVideoBaseAdaptationSet->durationMs();
    aMediaInformation.width = mVideoBaseAdaptationSet->getCurrentWidth();
    aMediaInformation.height = mVideoBaseAdaptationSet->getCurrentHeight();
    const MediaContent& mediaContent = mVideoBaseAdaptationSet->getAdaptationSetContent();
    if (mediaContent.matches(MediaContent::Type::AVC))
    {
        aMediaInformation.baseLayerCodec = VideoCodec::AVC;
    }
    else if (mediaContent.matches(MediaContent::Type::HEVC))
    {
        aMediaInformation.baseLayerCodec = VideoCodec::HEVC;
    }
    else
    {
        aMediaInformation.baseLayerCodec = VideoCodec::HEVC;
    }
    if (!mOvlyAdaptationSets.isEmpty())
    {
        const MediaContent& mediaContent = mOvlyAdaptationSets.at(0)->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AVC))
        {
            aMediaInformation.enchancementLayerCodec = VideoCodec::AVC;
        }
        else if (mediaContent.matches(MediaContent::Type::HEVC))
        {
            aMediaInformation.enchancementLayerCodec = VideoCodec::HEVC;
        }
        else
        {
            aMediaInformation.enchancementLayerCodec = VideoCodec::HEVC;
        }
    }
}

void_t DashVideoDownloader::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrAdditionalCodecs)
{
    aNrBaseLayerCodecs = 0;
    if (mVideoBaseAdaptationSet)
    {
        aNrBaseLayerCodecs++;
    }
    aNrAdditionalCodecs = mOvlyAdaptationSets.getSize();
}

Error::Enum DashVideoDownloader::completeInitialization(DashComponents& aDashComponents,
                                                        SourceDirection::Enum aUriSourceDirection,
                                                        sourceid_t aSourceIdBase)
{
    Error::Enum result;

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (DashAdaptationSetOverlay::isOverlay(aDashComponents.adaptationSet))
        {
            OMAF_LOG_V("Overlay %d", aDashComponents.adaptationSet->GetId());
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlay)(*this);
            result =
                ((DashAdaptationSetOverlay*) dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
            mOvlyAdaptationSets.add(dashAdaptationSet);
#if OMAF_LOOP_DASH
            dashAdaptationSet->setLoopingOn();
#endif
        }
        else if (DashAdaptationSetOverlayMeta::isOverlayMetadata(aDashComponents.adaptationSet))
        {
            OMAF_LOG_V("DYOL %d", aDashComponents.adaptationSet->GetId());
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
            result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                         ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_DYOL);
            mOvlyMetadataAdaptationSets.add(dashAdaptationSet);
#if OMAF_LOOP_DASH
            dashAdaptationSet->setLoopingOn();
#endif
        }
        else if (DashAdaptationSetOverlayMeta::isOverlayRvpoMetadata(aDashComponents.adaptationSet))
        {
            OMAF_LOG_V("Rvpo %d", aDashComponents.adaptationSet->GetId());
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
            result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                         ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_INVO);
            mOvlyRvpoMetadataAdaptationSets.add(dashAdaptationSet);
#if OMAF_LOOP_DASH
            dashAdaptationSet->setLoopingOn();
#endif
        }
        else
        {
            // "normal" adaptation set
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
            result = dashAdaptationSet->initialize(aDashComponents, initializationSegmentId);
        }
        if (result == Error::OK)
        {
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO) ||
                DashAdaptationSetOverlayMeta::isOverlayMetadata(aDashComponents.adaptationSet) ||
                DashAdaptationSetOverlayMeta::isOverlayRvpoMetadata(aDashComponents.adaptationSet))
            {
                mAdaptationSets.add(dashAdaptationSet);
            }
            else
            {
                // not for us, ignore
                OMAF_DELETE_HEAP(dashAdaptationSet);
                aDashComponents.nonVideoAdaptationSets.add(adaptationSet);
            }
        }
        else
        {
            OMAF_LOG_W("Initializing an adaptation set returned %d, ignoring it", result);
            OMAF_DELETE_HEAP(dashAdaptationSet);
        }
    }

    if (mAdaptationSets.isEmpty())
    {
        // No adaptations sets found, return failure
        return Error::NOT_SUPPORTED;
    }

    mVideoBaseAdaptationSet = OMAF_NULL;

    sourceid_t sourceId = aSourceIdBase;
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
        {
            mVideoBaseAdaptationSet = dashAdaptationSet;  // no enh layer supported in muxed case, also baselayer must
                                                          // be in the same adaptation set
            if (dashAdaptationSet->hasMPDVideoMetadata())
            {
                dashAdaptationSet->createVideoSources(sourceId);
            }
            break;
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO))
        {
            if (dashAdaptationSet->getType() == AdaptationSetType::BASELINE_DASH)
            {
                // low end single track device: find largest resolution top-bottom, or use mono. Assuming a single track
                // device don't support enh layer
                if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT &&
                    (mVideoBaseAdaptationSet == OMAF_NULL ||
                     (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() <
                      dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())))
                {
                    mVideoBaseAdaptationSet = dashAdaptationSet;
                    // force the adaptationset to mono (if it is mono/framepacked, no impact)
                    mVideoBaseAdaptationSet->forceVideoTo(StereoRole::MONO);
                }
            }
            if (dashAdaptationSet->hasMPDVideoMetadata())
            {
                dashAdaptationSet->createVideoSources(sourceId);
            }
        }
    }

    if (mVideoBaseAdaptationSet == OMAF_NULL)
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }
#if OMAF_LOOP_DASH
    mVideoBaseAdaptationSet->setLoopingOn();
#endif

    if ((aUriSourceDirection != SourceDirection::Enum::MONO && aUriSourceDirection != SourceDirection::Enum::INVALID) ||
        mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
    {
        // sourceDirection is from URI indicating stereo, mVideoBaseAdaptationSetStereo comes from MPD metadata, or
        // framepacked video type from MPD metadata
        OMAF_LOG_D("Stereo stream");
        mMediaInformation.isStereoscopic = true;
    }
    else
    {
        OMAF_LOG_D("Mono stream");
        mMediaInformation.isStereoscopic = false;
    }
    mBitrateController = OMAF_NEW_HEAP(DashBitrateContoller);
    mBitrateController->initialize(this);

    return Error::OK;
}

void_t DashVideoDownloader::release()
{
    stopDownload();

    while (!mAdaptationSets.isEmpty())
    {
        DashAdaptationSet* set = mAdaptationSets.at(0);
        mAdaptationSets.removeAt(0);
        OMAF_DELETE_HEAP(set);
    }

    mAdaptationSets.clear();
    // Make sure these are NULL
    mVideoBaseAdaptationSet = OMAF_NULL;
    OMAF_DELETE_HEAP(mTilePicker);
    mTilePicker = OMAF_NULL;

    OMAF_DELETE_HEAP(mBitrateController);
    mBitrateController = OMAF_NULL;
}

Error::Enum DashVideoDownloader::startDownload(time_t aStartTime, uint32_t aBufferingTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        if (mBufferingTimeMs == 0)
        {
            mBufferingTimeMs = 20 * aBufferingTimeMs;
            // 20 is just a very initial guess; MPD is generally N times smaller than a media segment. In OMAF we've
            // seen MPD's of order 100 kbytes; hence one segment could be in the order of 2Mbytes; depends heavily on
            // selected segment duration, target bitrate etc
        }
        mVideoBaseAdaptationSet->startDownload(aStartTime, mBufferingTimeMs);

        if (!mOvlyAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
            {
                (*it)->startDownload(aStartTime, mBufferingTimeMs);
            }
        }
        if (!mOvlyMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin();
                 it != mOvlyMetadataAdaptationSets.end(); ++it)
            {
                (*it)->startDownload(aStartTime, mBufferingTimeMs);
            }
        }
        if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
                 it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
            {
                (*it)->startDownload(aStartTime, mBufferingTimeMs);
            }
        }

        DashSpeedFactorMonitor::start(1);  // start with 1 adaptation set to monitor - children may restart with more
        mRunning = true;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

Error::Enum DashVideoDownloader::startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        if (mBufferingTimeMs == 0)
        {
            mBufferingTimeMs = 20 * aBufferingTimeMs;
            // 20 is just a very initial guess; MPD is generally N times smaller than a media segment. In OMAF we've
            // seen MPD's of order 100 kbytes; hence one segment could be in the order of 2Mbytes; depends heavily on
            // selected segment duration, target bitrate etc
        }
        mVideoBaseAdaptationSet->startDownloadFromTimestamp(aTargetTimeMs, mBufferingTimeMs);

        if (!mOvlyAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
            {
                (*it)->startDownloadFromTimestamp(aTargetTimeMs, mBufferingTimeMs);
            }
        }
        if (!mOvlyMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin();
                 it != mOvlyMetadataAdaptationSets.end(); ++it)
            {
                (*it)->startDownloadFromTimestamp(aTargetTimeMs, mBufferingTimeMs);
            }
        }
        if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
                 it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
            {
                (*it)->startDownloadFromTimestamp(aTargetTimeMs, mBufferingTimeMs);
            }
        }

        DashSpeedFactorMonitor::start(1);  // start with 1 adaptation set to monitor - children may restart with more
        mRunning = true;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

bool_t DashVideoDownloader::isMpdUpdateRequired() const
{
    return mVideoBaseAdaptationSet->mpdUpdateRequired();
}

bool_t DashVideoDownloader::isEndOfStream() const
{
    if (mVideoBaseAdaptationSet->isEndOfStream())
    {
        return true;
    }
    return false;
}

bool_t DashVideoDownloader::isError() const
{
    return mVideoBaseAdaptationSet->isError();
}

void_t DashVideoDownloader::updateStreams(uint64_t currentPlayTimeUs)
{
    processSegmentDownload();

    if (!mCurrentVideoStreams.isEmpty() && mABREnabled)
    {
        if (mBitrateController->update(mBandwidthOverhead))
        {
            mStreamUpdateNeeded = true;
        }
    }
}
void_t DashVideoDownloader::processSegments()
{
    // nothing to do with non-extractor cases
}

Error::Enum DashVideoDownloader::stopDownload()
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->stopDownload();

        if (!mOvlyAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
            {
                (*it)->stopDownload();
            }
        }
        if (!mOvlyMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin();
                 it != mOvlyMetadataAdaptationSets.end(); ++it)
            {
                (*it)->stopDownload();
            }
        }
        if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
                 it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
            {
                (*it)->stopDownload();
            }
        }

        DashSpeedFactorMonitor::stop();
        mRunning = false;
        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

Error::Enum DashVideoDownloader::stopDownloadAsync(uint32_t& aLastTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->stopDownloadAsync(false, true);
        aLastTimeMs = mVideoBaseAdaptationSet->getTimestampMsOfLastDownloadedFrame();

        if (!mOvlyAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
            {
                (*it)->stopDownloadAsync(false, true);
            }
        }
        if (!mOvlyMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin();
                 it != mOvlyMetadataAdaptationSets.end(); ++it)
            {
                (*it)->stopDownloadAsync(false, true);
            }
        }
        if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
                 it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
            {
                (*it)->stopDownloadAsync(false, true);
            }
        }

        DashSpeedFactorMonitor::stop();
        mRunning = false;
        return Error::OK;
    }
    return Error::INVALID_STATE;
}

void_t DashVideoDownloader::clearDownloadedContent()
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->clearDownloadedContent();

        if (mTilePicker != OMAF_NULL)
        {
            mTilePicker->reset();
        }
    }

    if (!mOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
        {
            (*it)->clearDownloadedContent();
        }
    }
    if (!mOvlyMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin(); it != mOvlyMetadataAdaptationSets.end();
             ++it)
        {
            (*it)->clearDownloadedContent();
        }
    }
    if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
             it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
        {
            (*it)->clearDownloadedContent();
        }
    }
}

Error::Enum DashVideoDownloader::seekToMs(uint64_t& aSeekMs, uint64_t aSeekResultMs)
{
    uint64_t seekToTarget = aSeekMs;
    Error::Enum result = Error::OK;

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        result = mVideoBaseAdaptationSet->seekToMs(seekToTarget, aSeekResultMs);
        if (result == Error::OK)
        {
            aSeekMs = aSeekResultMs;
        }
    }
    if (!mOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
        {
            uint64_t resultMs = 0;
            (*it)->seekToMs(seekToTarget, resultMs);
        }
    }
    if (!mOvlyMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin(); it != mOvlyMetadataAdaptationSets.end();
             ++it)
        {
            uint64_t resultMs = 0;
            (*it)->seekToMs(seekToTarget, resultMs);
        }
    }
    if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
             it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
        {
            uint64_t resultMs = 0;
            (*it)->seekToMs(seekToTarget, resultMs);
        }
    }

    return result;
}

bool_t DashVideoDownloader::isSeekable()
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        // we assume enhancement layer follows the same seeking properties as the base layer
        return mVideoBaseAdaptationSet->isSeekable();
    }
    else
    {
        return false;
    }
}

uint64_t DashVideoDownloader::durationMs() const
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        // we assume enhancement layer has the same duration as the base layer
        return mVideoBaseAdaptationSet->durationMs();
    }
    else
    {
        return 0;
    }
}

bool_t DashVideoDownloader::updateVideoStreams()
{
    bool_t refreshStillRequired = false;
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");
    mCurrentVideoStreams.clear();
    mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
    if (mCurrentVideoStreams.isEmpty())
    {
        OMAF_LOG_V("updateVideoStreams refresh still required");
        refreshStillRequired = true;
    }
    if (!mOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mActiveOvlyAdaptationSets.begin(); it != mActiveOvlyAdaptationSets.end();
             ++it)
        {
            mCurrentVideoStreams.add((*it)->getCurrentVideoStreams());
        }
    }

    return refreshStillRequired;
}

const MP4VideoStreams& DashVideoDownloader::getVideoStreams()
{
    if (mCurrentVideoStreams.isEmpty() || mStreamChangeMonitor.hasUpdated(StreamMonitorType::GET_VIDEO_STREAMS))
    {
        updateVideoStreams();
    }
    return mCurrentVideoStreams;
}

const MP4MetadataStreams& DashVideoDownloader::getMetadataStreams()
{
    return mCurrentMetadataStreams;
}

Error::Enum DashVideoDownloader::readVideoFrames(int64_t currentTimeUs)
{
    Error::Enum result = mVideoBaseAdaptationSet->readNextVideoFrame(currentTimeUs);
    if (!mActiveOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mActiveOvlyAdaptationSets.begin(); it != mActiveOvlyAdaptationSets.end();
             ++it)
        {
            (*it)->readNextVideoFrame(currentTimeUs);
        }
    }
    return result;
}

MP4VRMediaPacket* DashVideoDownloader::getNextVideoFrame(MP4MediaStream& stream, int64_t currentTimeUs)
{
    // With Dash we don't need to do anything special here
    return stream.peekNextFilledPacket();
}

Error::Enum DashVideoDownloader::readMetadataFrame(int64_t aCurrentTimeUs)
{
    Error::Enum result = Error::OK_SKIPPED;
    if (!mOvlyMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin(); it != mOvlyMetadataAdaptationSets.end();
             ++it)
        {
            Error::Enum singleResult = (*it)->readMetadataFrame(aCurrentTimeUs);
            if (singleResult != result)
            {
                result = singleResult;
            }
        }
    }
    if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
             it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
        {
            Error::Enum singleResult = (*it)->readMetadataFrame(aCurrentTimeUs);
            if (singleResult != result)
            {
                result = singleResult;
            }
        }
    }

    return result;
}

void_t DashVideoDownloader::checkOvlyStreamAssociations()
{
    if ((mOvlyMetadataAdaptationSets.isEmpty() && mOvlyRvpoMetadataAdaptationSets.isEmpty()) ||
        mOvlyAdaptationSets.isEmpty())
    {
        return;
    }
    for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin(); it != mOvlyMetadataAdaptationSets.end();
         ++it)
    {
        if (!(*it)->getCurrentMetadataStreams().isEmpty())
        {
            doCheckOvlyStreamAssociations((*it)->getCurrentMetadataStreams().at(0), *(*it));
        }
    }
    for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
         it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
    {
        if (!(*it)->getCurrentMetadataStreams().isEmpty())
        {
            doCheckOvlyStreamAssociations((*it)->getCurrentMetadataStreams().at(0), *(*it));
        }
    }
}

void_t DashVideoDownloader::doCheckOvlyStreamAssociations(MP4MediaStream* aMetadataStream,
                                                          const DashAdaptationSet& aMetaAdaptationSet)
{
    // check if not yet associated in stream level
    if (aMetadataStream->getRefStreams(MP4VR::FourCC("cdsc")).isEmpty())
    {
        // find what metadata adaptation set this is

        bool_t associated = false;
        for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
        {
            DashAdaptationSet* mediaAdaptationSet = *it;

            if (mediaAdaptationSet->getCurrentVideoStreams().isEmpty())
            {
                continue;
            }
            // Do the track association based on MPD metadata
            if (StringCompare(mediaAdaptationSet->getCurrentRepresentationId().getData(),
                              aMetaAdaptationSet.isAssociatedTo(MPDAttribute::kAssociationTypeValueCdsc)) ==
                ComparisonResult::EQUAL)
            {
                DashVideoStream* referredStream = (DashVideoStream*) mediaAdaptationSet->getCurrentVideoStreams().at(
                    0);  // only 1 media track per adaptation set supported
                referredStream->setAdaptationSetReference(mediaAdaptationSet);
                referredStream->addMetadataStream(aMetadataStream);
                aMetadataStream->addRefStream(MP4VR::FourCC("cdsc"), referredStream);
            }
        }
    }
}

Error::Enum DashVideoDownloader::updateMetadata(const MP4MediaStream& metadataStream,
                                                const MP4VRMediaPacket& metadataFrame)
{
    if (metadataStream.getRefStreams(MP4VR::FourCC("cdsc")).isEmpty())
    {
        return Error::OK_SKIPPED;
    }

    // Do the actual metadata update 
    for (auto refStream : metadataStream.getRefStreams(MP4VR::FourCC("cdsc")))
    {
        auto cdscStream = dynamic_cast<DashVideoStream*>(refStream);
		dynamic_cast<DashVideoStream*>(cdscStream)->getAdaptationSetReference()
            ->updateMetadata(metadataStream, *cdscStream, metadataFrame);
    }

	return Error::OK;
}

const CoreProviderSourceTypes& DashVideoDownloader::getVideoSourceTypes()
{
    if (mVideoBaseAdaptationSet)
    {
        // just 1 entry per type needed, so no need to check from the stereo or from many tiles
        return mVideoBaseAdaptationSet->getVideoSourceTypes();
    }
    return mVideoSourceTypesEmpty;
}

bool_t DashVideoDownloader::isBuffering()
{
    if (mVideoBaseAdaptationSet->isBuffering())
    {
        OMAF_LOG_D("[%p] isBuffering = TRUE", this);
        if (!mBuffering && mVideoBaseAdaptationSet->getNrProcessedVideoSegments() > 0)
        {
            mBuffering = true;
            mBitrateController->reportDownloadProblem(IssueType::BASELAYER_BUFFERING);
        }
        return true;
    }
    mBuffering = false;
    return false;
}

bool_t DashVideoDownloader::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // this is relevant for video base layer only
    if (mVideoBaseAdaptationSet->isReadyToSignalEoS(aStream))
    {
        return true;
    }
    return false;
}

const CoreProviderSources& DashVideoDownloader::getPastBackgroundVideoSourcesAt(uint64_t aPts)
{
    return mVideoBaseAdaptationSet->getCurrentVideoStreams().at(0)->getVideoSources();
}

const CoreProviderSources& DashVideoDownloader::getAllSources()
{
    // Technically, this is only adding current representation / adaptation set sources. But so far this is relevant
    // with overlays only
    if (mStreamChangeMonitor.hasUpdated(StreamMonitorType::ALL_SOURCES))
    {
        mAllSources.clear();
        for (DashAdaptationSet* overlay : mOvlyAdaptationSets)
        {
            mAllSources.add(overlay->getVideoSources());
        }
        mAllSources.add(mVideoBaseAdaptationSet->getVideoSources());
    }
    return mAllSources;
}

const MP4VideoStreams& DashVideoDownloader::selectSources(float64_t longitude,
                                                          float64_t latitude,
                                                          float64_t roll,
                                                          float64_t width,
                                                          float64_t height,
                                                          bool_t& aViewportSet)
{
    // Non-enh layer case
    if ((mStreamChangeMonitor.hasUpdated(StreamMonitorType::SELECT_SOURCES) || mRenderingVideoStreams.isEmpty() ||
         mReselectSources) &&
        mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mRenderingVideoStreams.clear();
        mReselectSources = false;
        size_t count = 1;
        mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
        if (mRenderingVideoStreams.getSize() < count)
        {
            // OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count,
            // mRenderingVideoStreams.getSize());
            mReselectSources = true;
        }
        for (AdaptationSets::Iterator it = mActiveOvlyAdaptationSets.begin(); it != mActiveOvlyAdaptationSets.end();
             ++it)
        {
            mRenderingVideoStreams.add((*it)->getCurrentVideoStreams());
        }
    }

    aViewportSet = true;

    return mRenderingVideoStreams;
}

void_t DashVideoDownloader::processSegmentDownload()
{
    // OMAF_LOG_D("%lld processSegmentDownload in", Time::getClockTimeMs());

    if (mVideoBaseAdaptationSet->processSegmentDownload())
    {
        // Some ABR stream switches done, update video streams
        mStreamUpdateNeeded = updateVideoStreams();
        mStreamChangeMonitor.update();
    }
    if (!mOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
        {
            (*it)->processSegmentDownload();
        }
    }
    if (!mOvlyMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin(); it != mOvlyMetadataAdaptationSets.end();
             ++it)
        {
            (*it)->processSegmentDownload();
        }
    }
    if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
             it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
        {
            (*it)->processSegmentDownload();
        }
    }

	std::map<uint32_t, MP4MediaStream*> mediaStreamLookup;
}


const Bitrates& DashVideoDownloader::getBitrates()
{
    return mVideoBaseAdaptationSet->getBitrates();
}
void_t DashVideoDownloader::setQualityLevels(uint8_t aFgQuality,
                                             uint8_t aMarginQuality,
                                             uint8_t aBgQuality,
                                             uint8_t aNrQualityLevels)
{
    mVideoBaseAdaptationSet->setQualityLevels(aFgQuality, aMarginQuality, aBgQuality, aNrQualityLevels);
}
void_t DashVideoDownloader::selectBitrate(uint32_t bitrate)
{
    mVideoBaseAdaptationSet->selectBitrate(bitrate);
}
bool_t DashVideoDownloader::isABRSwitchOngoing()
{
    return mVideoBaseAdaptationSet->isABRSwitchOngoing();
}

void_t DashVideoDownloader::increaseCache()
{
    mBufferingTimeMs *= 1.05f;  // Add 5% more buffering/caching time
    mVideoBaseAdaptationSet->setBufferingTime(mBufferingTimeMs);
}

bool_t DashVideoDownloader::checkAssociation(DashAdaptationSet* aAdaptationSet, const RepresentationId& aAssociatedTo)
{
    for (size_t j = 0; j < mAdaptationSets.getSize(); j++)
    {
        if (mAdaptationSets.at(j)->getCurrentRepresentationId() == aAssociatedTo)
        {
            if (aAdaptationSet->createMetadataParserLink(mAdaptationSets.at(j)) == Error::OK)
            {
                return true;
            }
        }
    }
    return false;
}

const char_t* DashVideoDownloader::findDashAssociationForStream(streamid_t aStreamId,
                                                                const char_t* aAssociationType) const
{
    for (DashAdaptationSet* video : mAdaptationSets)
    {
        MP4VideoStreams vStreams = video->getCurrentVideoStreams();
        OMAF_ASSERT(vStreams.getSize() <= 1, "Only 1 video stream per adaptation set supported");
        if (!vStreams.isEmpty() && vStreams.at(0)->getStreamId() == aStreamId)  // 1 stream per adaptation set supported
        {
            return video->isAssociatedTo(aAssociationType);
        }
    }
    return OMAF_NULL;
}

bool_t DashVideoDownloader::hasOverlayDashAssociation(DashAdaptationSet* aAdaptationSet, const char_t* aAssociationType)
{
    for (DashAdaptationSet* video : mOvlyAdaptationSets)
    {
        if (StringCompare(video->isAssociatedTo(aAssociationType), aAdaptationSet->getCurrentRepresentationId()) ==
            ComparisonResult::EQUAL)
        {
            if (StringCompare(aAssociationType, MPDAttribute::kAssociationTypeValueAudio) == ComparisonResult::EQUAL)
            {
                // link associated audio as we may need to access audio stream once we have the streams open
                // (to set user activateable flag)
                DashAdaptationSetOverlay* overlay = (DashAdaptationSetOverlay*) video;
                overlay->setAssociatedSet(aAdaptationSet);
            }
            return true;
        }
    }
    return false;
}

static DashAdaptationSet* findAdaptationSet(streamid_t aVideoStreamId, AdaptationSets& aSets)
{
    for (DashAdaptationSet* overlay : aSets)
    {
        MP4VideoStreams vStreams = overlay->getCurrentVideoStreams();
        OMAF_ASSERT(vStreams.getSize() <= 1, "Only 1 video stream per overlay adaptation set supported");
        if (!vStreams.isEmpty() &&
            vStreams.at(0)->getStreamId() == aVideoStreamId)  // 1 stream per adaptation set supported
        {
            return overlay;
        }
    }
    return OMAF_NULL;
}

bool_t DashVideoDownloader::switchOverlayVideoStream(streamid_t aVideoStreamId,
                                                     int64_t currentTimeUs,
                                                     bool_t& aPreviousOverlayStopped)
{
    aPreviousOverlayStopped = false;
    for (DashAdaptationSet* overlay : mActiveOvlyAdaptationSets)
    {
        if (overlay->getCurrentVideoStreams().at(0)->isUserActivateable())
        {
            stopOverlayVideoStream(overlay->getCurrentVideoStreams().at(0)->getStreamId());
            // need to set the active flag also here; in direct stopping cases the layers above do that as they know
            // what overlay is in question
            const CoreProviderSources sources = overlay->getCurrentVideoStreams().at(0)->getVideoSources();
            for (CoreProviderSource* source : sources)
            {
                if (source->type == SourceType::OVERLAY)
                {
                    OverlaySource* ovly = (OverlaySource*) source;
                    ovly->active = false;
                    overlay->getCurrentVideoStreams().at(0)->setVideoSources(sources);
                    mBandwidthOverhead -= overlay->getCurrentBandwidth();
                    aPreviousOverlayStopped = true;
                    break;
                }
            }

            break;
        }
    }
    DashAdaptationSet* overlay = findAdaptationSet(aVideoStreamId, mOvlyAdaptationSets);
    if (overlay != OMAF_NULL && !mActiveOvlyAdaptationSets.contains(overlay))
    {
        mActiveOvlyAdaptationSets.add(overlay);

        // assumes that the overlay download was started at the beginning, so stream is ready
        OMAF_ASSERT(!overlay->getCurrentVideoStreams().isEmpty(),
                    "Expected to have overlay download started automatically!");
        overlay->getCurrentVideoStreams().at(0)->setStartOffset(currentTimeUs);

        // make sure the stream is reset (if it was running until the end without stopping)
        if (overlay->isEndOfStream())
        {
            overlay->clearDownloadedContent();

            overlay->stopDownload();
            uint32_t sampleId = 0;
            MP4VideoStreams vStreams = overlay->getCurrentVideoStreams();
            vStreams.at(0)->findSampleIdByIndex(0, sampleId);
            bool_t segmentChanged = false;
            vStreams.at(0)->setNextSampleId(sampleId, segmentChanged);
        }

        // this is needed in restart cases; startTime is relevant with live, but assuming overlays always start from 0
        overlay->startDownload(0);
        mBandwidthOverhead += overlay->getCurrentBandwidth();
        mStreamChangeMonitor.update();
        OMAF_LOG_D("startOverlayVideoStream %d", overlay->getCurrentVideoStreams().at(0)->getStreamId());
        return true;
    }
    return false;
}

bool_t DashVideoDownloader::stopOverlayVideoStream(streamid_t aVideoStreamId)
{
    DashAdaptationSet* overlay = findAdaptationSet(aVideoStreamId, mActiveOvlyAdaptationSets);
    if (overlay != OMAF_NULL)
    {
        mActiveOvlyAdaptationSets.remove(overlay);

        // reset video playhead to the beginning of the stream
        uint64_t start = 0;
        uint64_t actual = 0;
        overlay->seekToMs(start, actual);
        overlay->stopDownloadAsync(false, true);
        mBandwidthOverhead -= overlay->getCurrentBandwidth();

        uint32_t sampleId = 0;
        MP4VideoStreams vStreams = overlay->getCurrentVideoStreams();
        vStreams.at(0)->findSampleIdByIndex(0, sampleId);
        bool_t segmentChanged = false;
        vStreams.at(0)->setNextSampleId(sampleId, segmentChanged);

        mStreamChangeMonitor.update();
        OMAF_LOG_D("stopOverlayVideoStream %d", overlay->getCurrentVideoStreams().at(0)->getStreamId());
        return true;
    }
    return false;
}

bool_t DashVideoDownloader::getViewpointGlobalCoordSysRotation(ViewpointGlobalCoordinateSysRotation& aRotation) const
{
    if (mCurrentVideoStreams.isEmpty())
    {
        return false;
    }
    aRotation = mCurrentVideoStreams.at(0)->getViewpointGCSRotation();
    return true;
}

bool_t DashVideoDownloader::getViewpointUserControls(ViewpointSwitchControls& aSwitchControls) const
{
    if (mCurrentVideoStreams.isEmpty())
    {
        return false;
    }
    aSwitchControls = mCurrentVideoStreams.at(0)->getViewpointSwitchControls();
    return true;
}

const ViewpointSwitchingList& DashVideoDownloader::getViewpointSwitchConfig() const
{
    OMAF_ASSERT(!mCurrentVideoStreams.isEmpty(), "No video streams when calling getViewpointSwitchConfig");
    return mCurrentVideoStreams.at(0)->getViewpointSwitchConfig();
}

void_t DashVideoDownloader::onNewStreamsCreated(MediaContent& aContent)
{
    if (aContent.matches(MediaContent::Type::HAS_META))
    {
        mCurrentMetadataStreams.clear();
        if (!mOvlyMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyMetadataAdaptationSets.begin();
                 it != mOvlyMetadataAdaptationSets.end(); ++it)
            {
                mCurrentMetadataStreams.add((*it)->getCurrentMetadataStreams());
            }
        }
        if (!mOvlyRvpoMetadataAdaptationSets.isEmpty())
        {
            for (AdaptationSets::Iterator it = mOvlyRvpoMetadataAdaptationSets.begin();
                 it != mOvlyRvpoMetadataAdaptationSets.end(); ++it)
            {
                mCurrentMetadataStreams.add((*it)->getCurrentMetadataStreams());
            }
        }
    }

    // check if metadatastream association can be done
    checkOvlyStreamAssociations();

    if (aContent.matches(MediaContent::Type::VIDEO_OVERLAY) && !mOvlyAdaptationSets.isEmpty())
    {
        for (AdaptationSets::Iterator it = mOvlyAdaptationSets.begin(); it != mOvlyAdaptationSets.end(); ++it)
        {
            MP4VideoStreams overlayVideos = (*it)->getCurrentVideoStreams();
            OMAF_ASSERT(overlayVideos.getSize() <= 1, "Only 1 video stream per overlay adaptation set supported");
            if (overlayVideos.isEmpty())
            {
                continue;
            }
            else if (overlayVideos.at(0)->isUserActivateable())
            {
                // pass the info to audio side via the associated set
                DashAdaptationSetOverlay* overlay = (DashAdaptationSetOverlay*) (*it);
                DashAdaptationSetAudio* set = (DashAdaptationSetAudio*) (overlay->getAssociatedSet());
                if (set != OMAF_NULL)
                {
                    set->setUserActivateable(true);
                }
            }
            else
            {
                // not user activateable, set it active
                DashAdaptationSetOverlay* overlay = (DashAdaptationSetOverlay*) (*it);
                DashAdaptationSetAudio* set = (DashAdaptationSetAudio*) (overlay->getAssociatedSet());
                if (set != OMAF_NULL)
                {
                    set->setUserActivateable(false);
                }
                if (!mActiveOvlyAdaptationSets.contains(*it))
                {
                    mActiveOvlyAdaptationSets.add(*it);
                }
            }
        }
    }

    // need to update mCurrentVideoStreams
    mStreamChangeMonitor.update();
}

void_t DashVideoDownloader::onDownloadProblem(IssueType::Enum aIssueType)
{
    DashSpeedFactorMonitor::reportDownloadProblem();
    if (mBitrateController->reportDownloadProblem(aIssueType))
    {
        mStreamUpdateNeeded = true;
    }
}

void_t DashVideoDownloader::onActivateMe(DashAdaptationSet* aMe)
{
    OMAF_ASSERT(false, "Not supported for video");
}

OMAF_NS_END
