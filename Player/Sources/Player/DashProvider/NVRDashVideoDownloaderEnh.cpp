
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
#include "DashProvider/NVRDashVideoDownloaderEnh.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "Metadata/NVRMetadataParser.h"
#include "Media/NVRMediaPacket.h"
#include "DashProvider/NVRDashLog.h"

#include "Foundation/NVRClock.h"
#include "Foundation/NVRTime.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "VAS/NVRVASTilePicker.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashVideoDownloaderEnh)

DashVideoDownloaderEnh::DashVideoDownloaderEnh()
    : DashVideoDownloader()
    , mVideoEnhTiles()
    , mVideoBaseAdaptationSetStereo(OMAF_NULL)
    , mGlobalSubSegmentsSupported(false)
{
}

DashVideoDownloaderEnh::~DashVideoDownloaderEnh()
{
    release();
}


uint32_t DashVideoDownloaderEnh::getCurrentBitrate()
{
    uint32_t bitrate = 0;
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSet->getCurrentBandwidth();
    }
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        bitrate += mVideoBaseAdaptationSetStereo->getCurrentBandwidth();
    }
    if (!mVideoEnhTiles.isEmpty())
    {
        VASTileSelection sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            bitrate += (*it)->getAdaptationSet()->getCurrentBandwidth();
        }
    }

    return bitrate;
}

void_t DashVideoDownloaderEnh::getNrRequiredVideoCodecs(uint32_t& aNrBaseLayerCodecs, uint32_t& aNrEnhLayerCodecs)
{
    aNrBaseLayerCodecs = 0;
    if (mVideoBaseAdaptationSet)
    {
        aNrBaseLayerCodecs++;
    }
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        aNrBaseLayerCodecs++;
    }

    if (!mVideoEnhTiles.isEmpty())
    {
        // we don't know the viewport yet, so we use guestimates for it
        aNrEnhLayerCodecs = (uint32_t)mTilePicker->getNrVisibleTiles(mVideoEnhTiles, 100.f, 100.f);
    }
}

Error::Enum DashVideoDownloaderEnh::completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo)
{
    Error::Enum result;

    bool_t hasEnhancementLayer = false;
    bool_t framePackedEnhLayer = false;

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (DashAdaptationSetSubPicture::isSubPicture(aDashComponents))
        {
            // a sub-picture adaptation set
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetSubPicture)(*this);
            result = ((DashAdaptationSetSubPicture*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
        }
        else
        {
            // "normal" adaptation set
            dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSet)(*this);
            result = dashAdaptationSet->initialize(aDashComponents, initializationSegmentId);
        }
        if (result == Error::OK)
        {
            mAdaptationSets.add(dashAdaptationSet);
            if (DeviceInfo::deviceSupportsLayeredVAS() != DeviceInfo::LayeredVASTypeSupport::NOT_SUPPORTED && dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_ENHANCEMENT))
            {
                if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
                {
                    framePackedEnhLayer = true;
                }
                hasEnhancementLayer = true;
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
    mVideoBaseAdaptationSetStereo = OMAF_NULL;

    if (mVideoBaseAdaptationSet == OMAF_NULL)
    {
        sourceid_t sourceId = 0;
        for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
        {
            DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::IS_MUXED))
            {
                mVideoBaseAdaptationSet = dashAdaptationSet;    // no enh layer supported in muxed case, also baselayer must be in the same adaptation set
                if (dashAdaptationSet->hasMPDVideoMetadata())
                {
                    dashAdaptationSet->createVideoSources(sourceId);
                }
                break;
            }
            else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_VIDEO))
            {
                if (dashAdaptationSet->isBaselayer())
                {
                    if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::FULL_STEREO)
                    {
                        // high end device: top-bottom or mono stream, 2-track stereo stream, or VAS (base+enh layer) stream
                        if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED || dashAdaptationSet->getVideoChannel() == StereoRole::MONO || dashAdaptationSet->getVideoChannel() == StereoRole::RIGHT)
                        {
                            // framepacked, right, or mono
                            if (mVideoBaseAdaptationSet == OMAF_NULL)
                            {
                                // not yet assigned a base adaptation set
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                            else if (hasEnhancementLayer)
                            {
                                if ((mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight()
                                    && mVideoBaseAdaptationSet->getVideoChannel() == dashAdaptationSet->getVideoChannel())
                                    || (mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO && dashAdaptationSet->getVideoChannel() != StereoRole::MONO))
                                {
                                    // select the smallest res full 360 baselayer
                                    mVideoBaseAdaptationSet = dashAdaptationSet;
                                }
                            }
                            else if ((mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                                || ((mVideoBaseAdaptationSet->getCurrentWidth() == dashAdaptationSet->getCurrentWidth() && mVideoBaseAdaptationSet->getCurrentHeight() == dashAdaptationSet->getCurrentHeight())
                                    && mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO && dashAdaptationSet->getVideoChannel() != StereoRole::MONO))
                            {
                                // no enh layer; select the highest res full 360 stream or a stereo rather than mono in case resolutions are equal
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                        }
                        else
                        {
                            // left
                            if (mVideoBaseAdaptationSetStereo == OMAF_NULL)
                            {
                                // not yet assigned a base right adaptation set
                                mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                            }
                            else if (hasEnhancementLayer)
                            {
                                if (mVideoBaseAdaptationSetStereo->getCurrentWidth() * mVideoBaseAdaptationSetStereo->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                                {
                                    // select the smallest res full 360 baselayer
                                    mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                                }
                            }
                            else if (mVideoBaseAdaptationSetStereo->getCurrentWidth() * mVideoBaseAdaptationSetStereo->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())
                            {
                                // no enh layer; select the highest res full 360 baselayer
                                mVideoBaseAdaptationSetStereo = dashAdaptationSet;
                            }
                        }
                    }
                    else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED && hasEnhancementLayer)
                    {
                        // mono/framepacked VAS, select the smallest resolution base layer
                        if (dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
                        {
                            if (mVideoBaseAdaptationSet == OMAF_NULL ||
                                (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight()))
                            {
                                mVideoBaseAdaptationSet = dashAdaptationSet;
                            }
                            // else framepacked base layer is useless for limited VAS case, if enh layer is not framepacked, ignore it
                        }
                        else if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT &&
                            (mVideoBaseAdaptationSet == OMAF_NULL ||
                            (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() > dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())))
                        {
                            mVideoBaseAdaptationSet = dashAdaptationSet;
                            // force the adaptationset to mono (if it is mono/framepacked, no impact)
                            mVideoBaseAdaptationSet->forceVideoTo(StereoRole::MONO);
                        }
                    }
                    else
                    {
                        // low end single track device: find largest resolution top-bottom, or use mono. Assuming a single track device don't support enh layer
                        if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT &&
                            (mVideoBaseAdaptationSet == OMAF_NULL ||
                            (mVideoBaseAdaptationSet->getCurrentWidth() * mVideoBaseAdaptationSet->getCurrentHeight() < dashAdaptationSet->getCurrentWidth() * dashAdaptationSet->getCurrentHeight())))
                        {
                            mVideoBaseAdaptationSet = dashAdaptationSet;
                            // force the adaptationset to mono (if it is mono/framepacked, no impact)
                            mVideoBaseAdaptationSet->forceVideoTo(StereoRole::MONO);
                        }
                    }
                }
                else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::FULL_STEREO)
                {
                    // not base layer, so must be VAS enhancement tile
                    mVideoEnhTiles.add((DashAdaptationSetSubPicture*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                }
                else if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED)
                {
                    // not base layer, so must be VAS enhancement tile
                    // take only right, mono, or framepacked, ignore left eye
                    if (dashAdaptationSet->getVideoChannel() != StereoRole::LEFT)
                    {
                        // force the adaptationset to mono (if it is mono/framepacked, no impact)
                        dashAdaptationSet->forceVideoTo(StereoRole::MONO);
                        mVideoEnhTiles.add((DashAdaptationSetSubPicture*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                    }
                }
                if (dashAdaptationSet->hasMPDVideoMetadata())
                {
                    dashAdaptationSet->createVideoSources(sourceId);
                    if (DeviceInfo::deviceSupportsLayeredVAS() == DeviceInfo::LayeredVASTypeSupport::LIMITED && dashAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED && !framePackedEnhLayer)
                    {
                        // special case: we have a frame-packed base layer, but tiles are left-right and we must use them as mono only => force base layer to mono too (download & decode full, but use only half of it)
                        // we do it after creating sources to avoid special flagging in source creation, which is already quite complicated
                        dashAdaptationSet->forceVideoTo(StereoRole::MONO);
                    }
                }
            }
        }

        for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
        {
            DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
            if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HAS_META))
            {// find which adaptation set/representation this metadata adaptation set is associated to
                RepresentationId associatedTo;
                if (dashAdaptationSet->isAssociatedToRepresentation(associatedTo))
                {
                    for (size_t j = 0; j < mAdaptationSets.getSize(); j++)
                    {
                        if (mAdaptationSets.at(j)->getCurrentRepresentationId() == associatedTo)
                        {
                        }
                    }
                }
            }
        }
    }

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
    }
    else
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }

    mTilePicker = OMAF_NEW_HEAP(VASTilePicker);

    if (!mVideoEnhTiles.isEmpty())
    {
        mGlobalSubSegmentsSupported = DeviceInfo::deviceSupportsSubsegments();
        mVideoEnhTiles.allSetsAdded(true);
        mMediaInformation.videoType = VideoType::VIEW_ADAPTIVE;

        const MediaContent& mediaContent = mVideoEnhTiles.getAdaptationSetAt(0)->getAdaptationSetContent();
        if (mediaContent.matches(MediaContent::Type::AVC))
        {
            mMediaInformation.enchancementLayerCodec = VideoCodec::AVC;
        }
        else if (mediaContent.matches(MediaContent::Type::HEVC))
        {
            mMediaInformation.enchancementLayerCodec = VideoCodec::HEVC;
        }
    }

    if (mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::MONO)
    {
        // we must not use right track if left is framepacked or mono
        mVideoBaseAdaptationSetStereo = OMAF_NULL;
    }

    if (aSourceInfo.sourceDirection != SourceDirection::Enum::MONO || mVideoBaseAdaptationSetStereo != OMAF_NULL || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
    {
        // sourceDirection is from URI indicating stereo, mVideoBaseAdaptationSetStereo comes from MPD metadata, or framepacked video type from MPD metadata
        OMAF_LOG_D("Stereo stream");
        mMediaInformation.isStereoscopic = true;
    }
    else
    {
        OMAF_LOG_D("Mono stream");
        mMediaInformation.isStereoscopic = false;
    }
    mBitrateController.initialize(
        mVideoBaseAdaptationSet,
        mVideoBaseAdaptationSetStereo,
        mVideoEnhTiles.getAdaptationSets(),
        mTilePicker);

    return Error::OK;
}

void_t DashVideoDownloaderEnh::release()
{
    DashVideoDownloader::release();

    mVideoBaseAdaptationSetStereo = OMAF_NULL;
    mVideoEnhTiles.clear();

}

Error::Enum DashVideoDownloaderEnh::startDownload(time_t aStartTime)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {

        mVideoBaseAdaptationSet->startDownload(aStartTime);
        
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
        {
            mVideoBaseAdaptationSetStereo->startDownload(aStartTime);
        }

        // this can be effective only after pause + resume. With cold start, allowEnhancement is false
        if (mBitrateController.allowEnhancement() && !mVideoEnhTiles.isEmpty())
        {
            VASTileSelection& sets = mTilePicker->getLatestTiles();
            for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
            {
                OMAF_LOG_D("Start downloading for longitude %f", (*it)->getCoveredViewport().getCenterLongitude());
                (*it)->getAdaptationSet()->startDownload(aStartTime);
            }
        }


        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

void_t DashVideoDownloaderEnh::updateStreams(uint64_t aCurrentPlayTimeUs)
{
    checkVASVideoStreams(aCurrentPlayTimeUs);
    DashVideoDownloader::updateStreams(aCurrentPlayTimeUs);
}

bool_t DashVideoDownloaderEnh::isError() const
{
    bool_t error = mVideoBaseAdaptationSet->isError();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        error |= mVideoBaseAdaptationSetStereo->isError();
    }
    return error;
}

Error::Enum DashVideoDownloaderEnh::stopDownload()
{

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mVideoBaseAdaptationSet->stopDownload();
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
        {
            mVideoBaseAdaptationSetStereo->stopDownload();
        }

        for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
        {
            mVideoEnhTiles.getAdaptationSetAt(i)->stopDownload();
        }

        return Error::OK;
    }
    else
    {
        return Error::INVALID_DATA;
    }
}

void_t DashVideoDownloaderEnh::clearDownloadedContent()
{
    DashVideoDownloader::clearDownloadedContent();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        mVideoBaseAdaptationSetStereo->clearDownloadedContent();
    }

    for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
    {
        mVideoEnhTiles.getAdaptationSetAt(i)->clearDownloadedContent();
    }
    if (mTilePicker != OMAF_NULL)
    {
        mTilePicker->reset();
    }
}

Error::Enum DashVideoDownloaderEnh::seekToMs(uint64_t& aSeekMs, uint64_t aSeekResultMs)
{
    uint64_t seekToTarget = aSeekMs;
    Error::Enum result = Error::OK;

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        result = mVideoBaseAdaptationSet->seekToMs(seekToTarget, aSeekResultMs);
        if (result == Error::OK)
        {
            aSeekMs = aSeekResultMs;
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
            {
                mVideoBaseAdaptationSetStereo->seekToMs(seekToTarget, aSeekResultMs);
            }

            for (size_t i = 0; i < mVideoEnhTiles.getNrAdaptationSets(); i++)
            {
                mVideoEnhTiles.getAdaptationSetAt(i)->seekToMs(seekToTarget, aSeekResultMs);
            }
        }
    }

    return result;
}

bool_t DashVideoDownloaderEnh::updateVideoStreams()
{
    bool_t refreshStillRequired = false;
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");
    mCurrentVideoStreams.clear();
    mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
    size_t count = 1;
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        mCurrentVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
        count++;
    }
    if (mCurrentVideoStreams.getSize() < count)
    {
        OMAF_LOG_V("updateVideoStreams refresh still required");
        refreshStillRequired = true;
    }
    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        mCurrentVideoStreams.clear();
        mCurrentVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
        {
            mCurrentVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
        }

        VASTileSelection sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            if ((*it)->getVideoStreams().getSize() == 0)
            {
                refreshStillRequired = true;
            }
            mCurrentVideoStreams.add((*it)->getVideoStreams());
        }
    }
    return refreshStillRequired;
}

void_t DashVideoDownloaderEnh::checkVASVideoStreams(uint64_t currentPTS)
{
    if (mVideoEnhTiles.isEmpty())
    {
        if (mStreamUpdateNeeded)
        {
            mStreamUpdateNeeded = updateVideoStreams();
            mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
        }
        return;
    }

    bool_t selectionUpdated = false;
    VASTileSelection droppedTiles;
    VASTileSelection additionalTiles;
    // read the latest selection, selection is done in renderer thread
    VASTileSelection tiles = mTilePicker->getLatestTiles(selectionUpdated, droppedTiles, additionalTiles);

    // First stop the download on any dropped tiles
    if (!droppedTiles.isEmpty())
    {
        // stop downloading dropped ones
        for (VASTileSelection::Iterator it = droppedTiles.begin(); it != droppedTiles.end(); ++it)
        {
            OMAF_LOG_D("stop downloading representation %s with center at %f longitude", (*it)->getAdaptationSet()->getRepresentationId(), (*it)->getCoveredViewport().getCenterLongitude());
            (*it)->getAdaptationSet()->stopDownloadAsync(true, true);   // save bandwidth and abort current downloads
        }
        mStreamUpdateNeeded = true;
    }

    if (mBitrateController.allowEnhancement())
    {
        // We have bandwidth to download the enhancement layers. Note! in OMAF case we return already above

        // If it's not yet being downloaded, start it
        uint32_t baseSegmentIndex = 0;
        time_t startTime = 0;
        uint32_t maxSegmentId = 0;
        uint64_t targetPtsUs = 0;
        bool_t initializeValues = true;
        for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            if (!(*it)->getAdaptationSet()->isActive())
            {
                if (initializeValues)
                {
                    initializeValues = false;
                    targetPtsUs = mVideoBaseAdaptationSet->getReadPositionUs(baseSegmentIndex);
                    if (targetPtsUs == 0)
                    {
                        targetPtsUs = currentPTS;
                    }
                    uint64_t baseSegmentDurationMs = 0;
                    uint32_t baseAlignment = 0;

                    uint64_t tileSegmentDurationMs = 0;
                    uint32_t tileAlignment = 0;
                    if (baseSegmentIndex == INVALID_SEGMENT_INDEX)
                    {
                        // base layer is not a suitable reference yet, let the segmentstreamer pick the first one suitable for the current playback time
                        startTime = time(0);
#if OMAF_PLATFORM_ANDROID
                        OMAF_LOG_D("Start time for tiles: %d", startTime);
#else
                        OMAF_LOG_D("Start time for tiles: %lld", startTime);
#endif
                    }
                    else if (mVideoEnhTiles.getAdaptationSetAt(0)->getAlignmentId(tileSegmentDurationMs, tileAlignment) && mVideoBaseAdaptationSet->getAlignmentId(baseSegmentDurationMs, baseAlignment) && baseAlignment == tileAlignment && baseSegmentDurationMs == tileSegmentDurationMs)
                    {
                        // base layer and enh layers seem to be aligned; use the base layer index as it should be always showing the correct read position
                        maxSegmentId = baseSegmentIndex;
                    }
                    else if (tileSegmentDurationMs > 0)
                    {
                        // Do some math
                        maxSegmentId = (uint32_t)((targetPtsUs / (tileSegmentDurationMs * 1000)) + 1);
                    }
                    else
                    {
                        // must be timeline mode; segments don't have fixed duration, so leave maxSegmentId = 0 and use the timestamp
                    }
                }

                if (baseSegmentIndex == INVALID_SEGMENT_INDEX)
                {
                    // let the segmentstreamer pick the first one suitable for the current playback time
                    (*it)->getAdaptationSet()->startDownload(startTime);
                }
                else
                {
                    // Not currently downloading, start a download
                    OMAF_LOG_V("start downloading new adaptation set %s at %f longitude",
                        (*it)->getAdaptationSet()->getRepresentationId(),
                        (*it)->getCoveredViewport().getCenterLongitude());
                    if (mGlobalSubSegmentsSupported && (*it)->getAdaptationSet()->supportsSubSegments())
                    {
                        OMAF_LOG_V("Segment ID from baselayer: %d, selected segment id for tiles: %d, target pts us %lld start by checking subsegments", baseSegmentIndex, maxSegmentId, targetPtsUs);
                        Error::Enum error = (*it)->getAdaptationSet()->hasSubSegments(targetPtsUs, maxSegmentId);
                        if (error == Error::OK)
                        {
                            (*it)->getAdaptationSet()->startSubSegmentDownload(targetPtsUs, maxSegmentId);
                            mStreamUpdateNeeded = true;
                            continue;
                        }
                        (*it)->getAdaptationSet()->startDownloadFromTimestamp(targetPtsUs, maxSegmentId, VideoStreamMode::ENHANCEMENT_FAST_FORWARD);
                    }
                    else
                    {
                        OMAF_LOG_V("Segment ID from baselayer: %d, selected segment id for tiles: %d, target pts us %lld", baseSegmentIndex, maxSegmentId, targetPtsUs);
                        (*it)->getAdaptationSet()->startDownloadFromTimestamp(targetPtsUs, maxSegmentId, VideoStreamMode::ENHANCEMENT_FAST_FORWARD);
                    }
                }
                mStreamUpdateNeeded = true;
            }
        }
    }
    else
    {
        // OMAF_LOG_D("No bandwidth for tiles so stopping them!");
        // No bandwidth for tiles so stop them all
        for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
        {
            if ((*it)->getAdaptationSet()->isActive())
            {
                OMAF_LOG_V("Stopped downloading a tile because of no bandwidth");
                (*it)->getAdaptationSet()->stopDownloadAsync(true, true);   // save bandwidth and abort
                mStreamUpdateNeeded = true;
            }
        }
    }

    if (mStreamUpdateNeeded)
    {
        mStreamUpdateNeeded = updateVideoStreams();
        mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
    }
    if (mCurrentVideoStreams.getSize() > 2)
    {
        // shuffle the stream order to balance the load; keep the base layer(s) as first
        // index must be volatile, otherwise compiler in Android release build makes removeAt as an infinite loop
        volatile size_t index = 1;
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
        {
            index = 2;
        }
        MP4VideoStream* tmp = mCurrentVideoStreams.at(index);
        mCurrentVideoStreams.removeAt(index, 1);
        mCurrentVideoStreams.add(tmp);
        //OMAF_LOG_D("First tile stream %d out of %zd", mCurrentVideoStreams[index]->getStreamId(), mCurrentVideoStreams.getSize())
    }
}

Error::Enum DashVideoDownloaderEnh::readVideoFrames(int64_t currentTimeUs)
{
    Error::Enum result = Error::OK;
    result = mVideoBaseAdaptationSet->readNextVideoFrame(currentTimeUs);

    // base 1 may return EOS, when switching ABR, but that must not prevent base2 to read video as it would trigger EOS on base 2 too
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        Error::Enum result2 = Error::OK;
        result2 = mVideoBaseAdaptationSetStereo->readNextVideoFrame(currentTimeUs);
        if (result2 != Error::OK && result2 != Error::END_OF_FILE)
        {
            // other error than end of file
            return result2;
        }
    }
    if (result != Error::OK && result != Error::END_OF_FILE)
    {
        // other error than end of file
        return result;
    }

    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        // use here the picker variant that minimizes changes to current set
        VASTileSelection& sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            // read enh tiles if there is data available. 
            // Ignore the result, as tiles are not mandatory for player operation, and connection, parsing etc problems are detected elsewhere.
            // If there is no data, it becomes visible in getNextVideoFrame as an empty packet

            (*it)->getAdaptationSet()->readNextVideoFrame(currentTimeUs);
        }
    }

    return result;
}

const CoreProviderSourceTypes& DashVideoDownloaderEnh::getVideoSourceTypes()
{
    OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No current video adaptation set");

    // we have MPD metadata; typically base + enh layers.
    if (mVideoSourceTypes.isEmpty())
    {
        // just 1 entry per type needed, so no need to check from the stereo or from many tiles
        mVideoSourceTypes.add(mVideoBaseAdaptationSet->getVideoSourceTypes());  // just 1 entry per type needed, so no need to check from the stereo

        if (!mVideoEnhTiles.isEmpty())
        {
            // just 1 entry per type needed
            mVideoSourceTypes.add(mVideoEnhTiles.getAdaptationSetAt(0)->getVideoSourceTypes());
        }

    }
    return mVideoSourceTypes;
}

bool_t DashVideoDownloaderEnh::isBuffering()
{
    const CoreProviderSourceTypes& sourceTypes = getVideoSourceTypes();
    if (sourceTypes.isEmpty())
    {
        return true;
    }
    bool_t videoBuffering = mVideoBaseAdaptationSet->isBuffering();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        videoBuffering |= mVideoBaseAdaptationSetStereo->isBuffering();
    }
    // Tiles are not checked for buffering, as it is concluded a better UX to show base layer if no tiles are available, than pause video
    if (videoBuffering)
    {
        OMAF_LOG_D("isBuffering = TRUE");
    }
    return videoBuffering;
}

bool_t DashVideoDownloaderEnh::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // this is relevant for video base layer only
    if (mVideoBaseAdaptationSet->isReadyToSignalEoS(aStream))
    {
        return true;
    }
    else if (mVideoBaseAdaptationSetStereo != OMAF_NULL)
    {
        return mVideoBaseAdaptationSetStereo->isReadyToSignalEoS(aStream);
    }
    return false;
}

const MP4VideoStreams& DashVideoDownloaderEnh::selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet)
{
    if (!mTileSetupDone && !mVideoEnhTiles.isEmpty())
    {
        mBaseLayerDecoderPixelsinSec = (uint32_t)(mVideoBaseAdaptationSet->getMinResXFps());
        if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
        {
            mBaseLayerDecoderPixelsinSec += (uint32_t)(mVideoBaseAdaptationSet->getMinResXFps());
        }

        // need to initialize the picker from renderer thread too (now we know the viewport width & height)
        mTilePicker->setupTileRendering(mVideoEnhTiles, width, height, mBaseLayerDecoderPixelsinSec);
        mTileSetupDone = true;
    }

    aViewportSet = false;
    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        if (mTilePicker->setRenderedViewPort(longitude, latitude, roll, width, height) || mRenderingVideoStreams.isEmpty() || mReselectSources || mVideoStreamsChanged.compareAndSet(false, true))
        {
            mReselectSources = false;
            mVideoStreamsChanged = false;   // reset in case some other condition triggered too

            // collect streams that match with the selected sources
            mRenderingVideoStreams.clear();

            size_t count = 1;
            mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
            if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
            {
                count = 2;
                mRenderingVideoStreams.add(mVideoBaseAdaptationSetStereo->getCurrentVideoStreams());
            }
            if (mRenderingVideoStreams.getSize() < count)
            {
                // a base stream is not available yet => trigger to recheck next time
                //OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count, mRenderingVideoStreams.getSize());
                mReselectSources = true;
            }
            // do the tile picking 
            VASTileSelection& tiles = mTilePicker->pickTiles(mVideoEnhTiles, mBaseLayerDecoderPixelsinSec);
            for (VASTileSelection::Iterator it = tiles.begin(); it != tiles.end(); ++it)
            {
                aViewportSet = true;
                count = mRenderingVideoStreams.getSize();
                mRenderingVideoStreams.add((*it)->getVideoStreams());
                if (count == mRenderingVideoStreams.getSize())
                {
                    // a stream is not available yet => trigger to recheck next time
                    OMAF_LOG_V("Stream for a tile not yet available");
                    mReselectSources = true;
                }
            }

            //OMAF_LOG_D("Updated rendering streams, count %zd", mRenderingVideoStreams.getSize());
        }
        // else no change, use previous streams
    }

    if (!mRenderingVideoStreams.isEmpty())
    {
        aBaseLayerStreamId = mRenderingVideoStreams.at(0)->getStreamId();
    }
    return mRenderingVideoStreams;
}

void_t DashVideoDownloaderEnh::processSegmentDownload()
{
    //OMAF_LOG_D("%lld processSegmentDownload in", Time::getClockTimeMs());
    bool_t videoStreamsChanged = false;
    videoStreamsChanged = mVideoBaseAdaptationSet->processSegmentDownload();
    if (mVideoBaseAdaptationSetStereo != OMAF_NULL && mVideoBaseAdaptationSetStereo->isActive())
    {
        videoStreamsChanged |= mVideoBaseAdaptationSetStereo->processSegmentDownload();
    }
    uint32_t segmentId = 0;
    if (!mVideoEnhTiles.isEmpty() && mBitrateController.allowEnhancement())
    {
        VASTileSelection& sets = mTilePicker->getLatestTiles();
        for (VASTileSelection::Iterator it = sets.begin(); it != sets.end(); ++it)
        {
            videoStreamsChanged |= (*it)->getAdaptationSet()->processSegmentDownload();
        }

        VASTileSelection &allTiles = mTilePicker->getAll(mVideoEnhTiles);
        uint32_t lastSegmentId = 0;
        for (VASTileSelection::Iterator it = allTiles.begin(); it != allTiles.end(); ++it)
        {
            lastSegmentId = (*it)->getAdaptationSet()->getLastSegmentId();
            if (lastSegmentId > segmentId) // TODO: JPH: what about after seek... will some of the tiles have permanently high segment id?
            {
                segmentId = lastSegmentId;
            }
        }
    }

    if (videoStreamsChanged)
    {
        // Some ABR stream switches done, update video streams
        updateVideoStreams();
        mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
    }

    if (mGlobalSubSegmentsSupported)
    {
        VASTileSelection &nonActiveSets = mTilePicker->getCurrentNonSelected(mVideoEnhTiles);
        for (VASTileSelection::Iterator it = nonActiveSets.begin();
                it != nonActiveSets.end(); ++it)
        {
            if (segmentId > 0 && (*it)->getAdaptationSet()->supportsSubSegments())
            {
                (*it)->getAdaptationSet()->processSegmentIndexDownload(segmentId);
            }
        }
    }
}

OMAF_NS_END
