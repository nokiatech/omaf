
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
#include "DashProvider/NVRDashVideoDownloaderExtractorMultiRes.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashAdaptationSetOverlay.h"
#include "DashProvider/NVRDashAdaptationSetOverlayMeta.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashLog.h"
#include "Media/NVRMediaPacket.h"
#include "Metadata/NVRMetadataParser.h"

#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRClock.h"
#include "Foundation/NVRDeviceInfo.h"
#include "Foundation/NVRTime.h"
#include "VAS/NVRVASTilePicker.h"
#include "VAS/NVRVASTileSetPicker.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashVideoDownloaderExtractorMultiRes)

DashVideoDownloaderExtractorMultiRes::DashVideoDownloaderExtractorMultiRes()
    : DashVideoDownloaderExtractor()
{
}

DashVideoDownloaderExtractorMultiRes::~DashVideoDownloaderExtractorMultiRes()
{
    release();
}

void_t DashVideoDownloaderExtractorMultiRes::release()
{
    mAlternateVideoBaseAdaptationSets.clear();
    mNextVideoBaseAdaptationSets.clear();

    DashVideoDownloaderExtractor::release();
}

Error::Enum DashVideoDownloaderExtractorMultiRes::completeInitialization(DashComponents& aDashComponents,
                                                                         SourceDirection::Enum aUriSourceDirection,
                                                                         sourceid_t aSourceIdBase)
{
    Error::Enum result;

    mBitrateController = OMAF_NEW_HEAP(DashBitrateContollerExtractor);

    SupportingAdaptationSetIds supportingSets;

    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        if (DashAdaptationSetExtractor::isExtractor(adaptationSet))
        {
            supportingSets.add(DashAdaptationSetExtractor::hasPreselection(
                adaptationSet, DashAdaptationSetExtractor::parseId(adaptationSet)));
        }
    }

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    // schemes with 2 resolutions (6K cubemap, 5K equirect)
    FixedArray<uint32_t, 4> tileResolutions;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (!supportingSets.isEmpty())
        {
            if (DashAdaptationSetExtractor::isExtractor(adaptationSet))
            {
                // extractor with Preselections-descriptor
                if (DashAdaptationSetExtractor::hasMultiResolution(adaptationSet))
                {
                    dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractorMR)(*this);
                    result = ((DashAdaptationSetExtractorMR*) dashAdaptationSet)
                                 ->initialize(aDashComponents, initializationSegmentId);
                }
            }
            else if (DashAdaptationSetTile::isTile(adaptationSet, supportingSets))
            {
                // a tile that an extractor depends on through Preselections-descriptor
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result =
                    ((DashAdaptationSetTile*) dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
                uint32_t resolution = dashAdaptationSet->getCurrentHeight() * dashAdaptationSet->getCurrentWidth();
                if (!tileResolutions.contains(resolution))
                {
                    bool_t added = false;
                    for (size_t i = 0; i < tileResolutions.getSize(); i++)
                    {
                        if (resolution > tileResolutions[i])
                        {
                            tileResolutions.add(resolution, i);
                            added = true;
                            break;
                        }
                    }
                    if (!added)
                    {
                        tileResolutions.add(resolution);
                    }
                }
            }
            else if (DashAdaptationSetOverlay::isOverlay(adaptationSet))
            {
                OMAF_LOG_V("Overlay %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlay)(*this);
                result = ((DashAdaptationSetOverlay*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId);
                mOvlyAdaptationSets.add(dashAdaptationSet);
            }
            else if (DashAdaptationSetOverlayMeta::isOverlayMetadata(adaptationSet))
            {
                OMAF_LOG_V("DYOL %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
                result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_DYOL);
                mOvlyMetadataAdaptationSets.add(dashAdaptationSet);
            }
            else if (DashAdaptationSetOverlayMeta::isOverlayRvpoMetadata(adaptationSet))
            {
                OMAF_LOG_V("REVP %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
                result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_INVO);
                mOvlyRvpoMetadataAdaptationSets.add(dashAdaptationSet);
            }
            else
            {
                aDashComponents.nonVideoAdaptationSets.add(adaptationSet);
                continue;
            }
        }

#if OMAF_LOOP_DASH
        if (dashAdaptationSet != OMAF_NULL)
        {
            dashAdaptationSet->setLoopingOn();
        }
#endif

        if (result == Error::OK)
        {
            mAdaptationSets.add(dashAdaptationSet);
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

    sourceid_t sourceIdBase = aSourceIdBase;
    sourceid_t sourceIdsPerAdaptationSet =
        (sourceid_t)(supportingSets.at(0).partialAdSetIds.getSize() *
                     2);  // the number of partial adaptation sets gives the limit for the number of sources, *2 for
                          // stereo; mono cubemap typically requires 24 (6*4)
    // Check if there is an extractor set with supporting partial sets, and if so, link them.
    // This is not (full) dupe of what is done above with supportingSets, as here we have the adaptation set objects
    // created and really link the objects, not just the ids
    uint8_t nrQualityLevels = 1;
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getType() == AdaptationSetType::EXTRACTOR &&
            dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HEVC))
        {
            mAlternateVideoBaseAdaptationSets.add((DashAdaptationSetExtractorMR*) dashAdaptationSet);
            mVideoBaseAdaptationSet = dashAdaptationSet;
            OMAF_LOG_V("Configuring %d", dashAdaptationSet->getId());
            dashAdaptationSet->createVideoSources(sourceIdBase);
            sourceIdBase += sourceIdsPerAdaptationSet;
            for (size_t k = 0; k < supportingSets.getSize(); k++)
            {
                if (supportingSets[k].mainAdSetId == dashAdaptationSet->getId())
                {
                    for (uint32_t l = 0; l < supportingSets[k].partialAdSetIds.getSize(); l++)
                    {
                        for (uint32_t j = 0; j < mAdaptationSets.getSize(); j++)
                        {
                            DashAdaptationSet* otherAdaptationSet = mAdaptationSets.at(j);
                            if (otherAdaptationSet != dashAdaptationSet &&
                                otherAdaptationSet->getType() == AdaptationSetType::TILE &&
                                (supportingSets[k].partialAdSetIds.at(l) == otherAdaptationSet->getId()))
                            {
                                ((DashAdaptationSetExtractor*) dashAdaptationSet)
                                    ->addSupportingSet((DashAdaptationSetTile*) otherAdaptationSet);
                                uint8_t qualities = ((DashAdaptationSetTile*) otherAdaptationSet)->getNrQualityLevels();
                                // find the max # of levels and store it to bitrate controller. The levels are for ABR
                                // only, not for fg/bg in this variant.
                                if (qualities > nrQualityLevels)
                                {
                                    nrQualityLevels = qualities;
                                }
                                uint32_t resolution =
                                    otherAdaptationSet->getCurrentHeight() * otherAdaptationSet->getCurrentWidth();
                                if (resolution == tileResolutions.at(0))
                                {
                                    ((DashAdaptationSetTile*) otherAdaptationSet)->setRole(TileRole::FOREGROUND);
                                }
                                else if (resolution == tileResolutions.at(1))
                                {
                                    ((DashAdaptationSetTile*) otherAdaptationSet)->setRole(TileRole::BACKGROUND);
                                }
                                else if (resolution == tileResolutions.at(2))
                                {
                                    ((DashAdaptationSetTile*) otherAdaptationSet)->setRole(TileRole::FOREGROUND_POLE);
                                }
                                else
                                {
                                    ((DashAdaptationSetTile*) otherAdaptationSet)->setRole(TileRole::BACKGROUND_POLE);
                                }
                            }
                        }
                    }
                    break;
                }
            }
            // check if it has coverage definitions
            const FixedArray<VASTileViewport*, 32> viewports =
                ((DashAdaptationSetExtractorMR*) dashAdaptationSet)->getCoveredViewports();
            if (!viewports.isEmpty())
            {
                for (FixedArray<VASTileViewport*, 32>::ConstIterator it = viewports.begin(); it != viewports.end();
                     ++it)
                {
                    mVideoPartialTiles.add(
                        OMAF_NEW_HEAP(VASTileSetContainer)((DashAdaptationSetSubPicture*) dashAdaptationSet, **it,
                                                           ((DashAdaptationSetExtractorMR*) dashAdaptationSet)
                                                               ->getSupportingSetsForRole(TileRole::FOREGROUND)));
                }
            }
        }
        if (nrQualityLevels > 0)
        {
            mBitrateController->setNrQualityLevels(nrQualityLevels);
        }
    }

    if (mVideoBaseAdaptationSet == OMAF_NULL)
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }

    for (uint32_t index = 0; index < mOvlyAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mOvlyAdaptationSets.at(index);
        if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_OVERLAY))
        {
            if (dashAdaptationSet->hasMPDVideoMetadata())
            {
                dashAdaptationSet->createVideoSources(sourceIdBase);
            }
        }
    }

    mTilePicker = OMAF_NEW_HEAP(VASTileSetPicker);

    if (!mVideoPartialTiles.isEmpty())
    {
        // server has defined the tilesets per viewing orientation => 1 tileset at a time => overlap is often
        // intentional and makes no harm
        mVideoPartialTiles.allSetsAdded(false);
        mMediaInformation.videoType = VideoType::VIEW_ADAPTIVE;
    }

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

    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->calculateBitrates(nrQualityLevels);

    mBitrateController->initialize(this);

    // Video stream id plays a special role with extractors: in multi-res all extractors must share it within a
    // viewpoint. But it must change between viewpoints.
    streamid_t videoStreamId = VideoDecoderManager::getInstance()->generateUniqueStreamID(true);
    for (DashAdaptationSetExtractor* extractor : mAlternateVideoBaseAdaptationSets)
    {
        extractor->setVideoStreamId(videoStreamId);
    }


    return Error::OK;
}

Error::Enum DashVideoDownloaderExtractorMultiRes::startDownload(time_t aStartTime, uint32_t aBufferingTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL && mVideoBaseAdaptationSet->getLastSegmentId() == 0)
    {
        bool_t selectionUpdated = false;
        VASTileSetContainer* droppedTile = OMAF_NULL;
        VASTileSetContainer* additionalTile = OMAF_NULL;
        VASTileSetContainer* set =
            ((VASTileSetPicker*) mTilePicker)->getLatestTileSet(selectionUpdated, &droppedTile, &additionalTile);
        if (additionalTile)
        {
            mVideoBaseAdaptationSet = additionalTile->getAdaptationSet();
            if (!mAlternateVideoBaseAdaptationSets.isEmpty())
            {
                // @note real segment id resolved ni DashAdaptationSetExtractor::concatenateAndParseSegments()
                ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchToThis(1);
            }
        }
        return DashVideoDownloaderExtractor::startDownload(aStartTime, aBufferingTimeMs);
    }
    else if (mNextVideoBaseAdaptationSets.getSize() > 0)
    {
        return mNextVideoBaseAdaptationSets.back()->startDownload(aStartTime, aBufferingTimeMs);
    }
    else
    {
        return DashVideoDownloaderExtractor::startDownload(aStartTime, aBufferingTimeMs);
    }
}

Error::Enum DashVideoDownloaderExtractorMultiRes::startDownloadFromTimestamp(uint32_t& aTargetTimeMs,
                                                                             uint32_t aBufferingTimeMs)
{
    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        bool_t selectionUpdated = false;
        VASTileSetContainer* droppedTile = OMAF_NULL;
        VASTileSetContainer* additionalTile = OMAF_NULL;
        VASTileSetContainer* set =
            ((VASTileSetPicker*) mTilePicker)->getLatestTileSet(selectionUpdated, &droppedTile, &additionalTile);
        if (additionalTile)
        {
            mVideoBaseAdaptationSet = additionalTile->getAdaptationSet();
            if (!mAlternateVideoBaseAdaptationSets.isEmpty())
            {
                ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchToThis(1);
            }
        }
        return DashVideoDownloaderExtractor::startDownloadFromTimestamp(aTargetTimeMs, aBufferingTimeMs);
    }
    return Error::INVALID_STATE;
}

bool_t DashVideoDownloaderExtractorMultiRes::isEndOfStream() const
{
    if (((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->isEndOfStream())
    {
        return true;
    }
    else if (!mNextVideoBaseAdaptationSets.isEmpty() && mNextVideoBaseAdaptationSets.back()->isEndOfStream())
    {
        return true;
    }
    return false;
}

bool_t DashVideoDownloaderExtractorMultiRes::isBuffering()
{
    // check if we have frames left from the current segment, and if not, can we concatenate the next segment within any
    // of the candidate extractors
    uint32_t doneSegment = 0;
    uint32_t newestSegment = 0;
    if (((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->isAtSegmentBoundary(doneSegment, newestSegment))
    {
        uint32_t nextSegment = ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();

        if (doneSegment == newestSegment &&
            !((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->segmentAvailable(nextSegment))
        {
            OMAF_LOG_V("Current one doesn't have segment %d", nextSegment);
            if (mNextVideoBaseAdaptationSets.isEmpty())
            {
                OMAF_LOG_D("isBuffering = TRUE - no switching extractors");
                return true;
            }
            if (!hasSegmentAvailable(nextSegment, mNextVideoBaseAdaptationSets.getSize()))
            {
                OMAF_LOG_D("isBuffering = TRUE - switching not possible");
                return true;
            }
        }
    }

    return false;
}

Error::Enum DashVideoDownloaderExtractorMultiRes::stopDownload()
{
    Error::Enum result = DashVideoDownloaderExtractor::stopDownload();

    if (!mNextVideoBaseAdaptationSets.isEmpty())
    {
        for (DashAdaptationSet* adaptationSet : mNextVideoBaseAdaptationSets)
        {
            adaptationSet->stopDownload();
        }
    }
    return result;
}

void_t DashVideoDownloaderExtractorMultiRes::clearDownloadedContent()
{
    DashVideoDownloaderExtractor::clearDownloadedContent();
    if (!mNextVideoBaseAdaptationSets.isEmpty())
    {
        for (DashAdaptationSet* adaptationSet : mNextVideoBaseAdaptationSets)
        {
            adaptationSet->clearDownloadedContent();
        }
        mNextVideoBaseAdaptationSets.clear();
    }
}

bool_t DashVideoDownloaderExtractorMultiRes::isSeekable()
{
    // not seekable for now, until we get get extractor switch incorporated with seek
    return true;
}

void_t DashVideoDownloaderExtractorMultiRes::checkVASVideoStreams(uint64_t currentPTS)
{
    if (isBuffering())
    {
        OMAF_LOG_V("Buffering, don't change VAS video streams now");
        return;
    }
    if (!mRunning)
    {
        OMAF_LOG_V("Already stopped (due to viewpoint switch?), don't change VAS video streams now");
        return;
    }

    if (!mVideoPartialTiles.isEmpty())
    {
        // OMAF
        bool_t selectionUpdated = false;
        VASTileSetContainer* droppedTile = OMAF_NULL;
        VASTileSetContainer* additionalTile = OMAF_NULL;
        VASTileSetContainer* set =
            ((VASTileSetPicker*) mTilePicker)->getLatestTileSet(selectionUpdated, &droppedTile, &additionalTile);

        if (selectionUpdated)
        {
            // select the best base adaptation set
            if (additionalTile)
            {
                if (!mNextVideoBaseAdaptationSets.isEmpty())
                {
                    OMAF_LOG_V("Stop the next one under preparation (%d), as new next is set",
                               mNextVideoBaseAdaptationSets.back()->getId());
                    mNextVideoBaseAdaptationSets.back()->stopDownloadAsync(
                        false, (DashAdaptationSetExtractorMR*) (additionalTile->getAdaptationSet()));
                }

                uint64_t targetPtsUs = OMAF_UINT64_MAX;
                uint32_t nextProcessedSegmentIndex =
                    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
                if (nextProcessedSegmentIndex == 1)
                {
                    // not even started the very first set yet, switch immediately
                    mVideoBaseAdaptationSet->stopDownloadAsync(true, false);  // abort
                    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchingToAnother();
                    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchedToAnother();
                    mVideoBaseAdaptationSet = (DashAdaptationSetExtractorMR*) (additionalTile->getAdaptationSet());
                    time_t startTime = time(0);
                    OMAF_LOG_D("Start time: %d", (uint32_t) startTime);
                    mVideoBaseAdaptationSet->startDownload(startTime, mBufferingTimeMs);
                    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchToThis(nextProcessedSegmentIndex);
                }
                else
                {
                    uint32_t lastDownloadedSegmentIndex =
                        ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getLastSegmentId();
                    uint32_t nextDLSegmentIndex = nextProcessedSegmentIndex;
                    if (lastDownloadedSegmentIndex >= nextProcessedSegmentIndex)
                    {
                        // normal case: there are segments waiting to be concatenated already for the current adaptation
                        // set, at least this segment must be downloaded for the next adaptation set
                        nextDLSegmentIndex = lastDownloadedSegmentIndex + 1;
                    }
                    // Else some tiles etc may be missing the next segment index and they are now stopped, so they won't
                    // get downloaded at this point. Hence the starting segment for the new adaptation set should be the
                    // next segment to be processed

                    DashAdaptationSetExtractorMR* nextVideoBaseAdaptationSet =
                        (DashAdaptationSetExtractorMR*) (additionalTile->getAdaptationSet());
                    // first remove possible older instances
                    mNextVideoBaseAdaptationSets.remove(nextVideoBaseAdaptationSet);
                    // then add this to the end
                    mNextVideoBaseAdaptationSets.add(nextVideoBaseAdaptationSet);

                    uint8_t fgLevel, bgLevel, marginLevel, nrLevels;
                    mBitrateController->getQualityLevelForeground(fgLevel, nrLevels);
                    mBitrateController->getQualityLevelBackground(bgLevel, nrLevels);
                    mBitrateController->getQualityLevelMargin(marginLevel);
                    nextVideoBaseAdaptationSet->prepareQualityLevels(set->getFgActiveSets(), set->getFgMarginSets(),
                                                                     fgLevel, bgLevel, marginLevel, nrLevels);

                    uint32_t segmentIndex =
                        ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)
                            ->estimateSegmentIdForSwitch(nextProcessedSegmentIndex, nextDLSegmentIndex);
                    nextVideoBaseAdaptationSet->startDownloadFromSegment(segmentIndex, nextProcessedSegmentIndex,
                                                                         mBufferingTimeMs);
                    if (droppedTile)
                    {
                        ((DashAdaptationSetExtractorMR*) droppedTile->getAdaptationSet())
                            ->stopDownloadAsync(false, nextVideoBaseAdaptationSet);
                    }

                    if (mVideoBaseAdaptationSet == nextVideoBaseAdaptationSet)
                    {
                        // switching back to the currently active one before the previous switch away from it took
                        // place; no need to change, but need to restart the download in it. restart
                        OMAF_LOG_V("Don't switch but restart the old extractor adaptation set %d",
                                   mVideoBaseAdaptationSet->getId());
                    }
                    else
                    {
                        OMAF_LOG_V("Start switch to extractor adaptation set %d", nextVideoBaseAdaptationSet->getId());
                    }
                }
            }
        }
        else if (mStreamUpdateNeeded)
        {
            mStreamUpdateNeeded = updateVideoStreams();
            mStreamChangeMonitor.update();  // trigger also renderer thread to update streams
        }

        return;
    }
}


bool_t DashVideoDownloaderExtractorMultiRes::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // in multi-resolution case, we should not signal EOS when switching extractors, since we reuse video decoder
    return false;
}

const CoreProviderSources& DashVideoDownloaderExtractorMultiRes::getPastBackgroundVideoSourcesAt(uint64_t aPts)
{
    for (size_t i = 0; i < (int32_t) mPastVideoSources.getSize(); i++)
    {
        size_t index = mPastVideoSources.getSize() - i - 1;
        if (aPts >= mPastVideoSources[index].validFrom)
        {
            OMAF_LOG_V("Source valid from %lld until %lld", mPastVideoSources[index].validFrom);
            return mPastVideoSources[index].sources;
        }
    }
    return mVideoBaseAdaptationSet->getCurrentVideoStreams().at(0)->getVideoSources();
}

void_t DashVideoDownloaderExtractorMultiRes::processSegmentDownload()
{
    if (!mNextVideoBaseAdaptationSets.isEmpty())
    {
        // use the specific version to avoid tiles to get extracted in parallel, as that can cause kind of race
        // condition with the main adaptation set with common tiles
        mNextVideoBaseAdaptationSets.back()->processSegmentDownload();
        bool_t switched = false;

        uint32_t nextSegment = ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
        uint32_t doneSegment = nextSegment;
        uint32_t newestSegment = 0;
        // if the current extractor has finished a segment (doneSegment gets updated) and is now on segment boundary
        // all concatenated segments have been read
        // and the finished segment is the last concatenated one
        // and the next one is ready to take over
        //     or the current one cannot continue any more (no data cached nor under download)
        if (((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->isAtSegmentBoundary(doneSegment, newestSegment))
        {
            OMAF_LOG_V("Done %d newest %d", doneSegment, newestSegment);
            if (doneSegment < newestSegment)
            {
                // keep on processing all concatenated segments
                // Note: nextSegment is global, it is shared when switching. doneSegment and newestSegment are local to
                // the extractor. In fast switching back-and-forth cases, extractor may not have any segments when it is
                // being switched away already. Then we can switch immediately to the next one
                OMAF_LOG_V("Keep on using the current extractor adaptation set %d, done %d, newest %d, next %d",
                           mVideoBaseAdaptationSet->getId(), doneSegment, newestSegment, nextSegment);
            }
            else if (mNextVideoBaseAdaptationSets.back() == mVideoBaseAdaptationSet &&
                     mNextVideoBaseAdaptationSets.back()->segmentAvailable(nextSegment))
            {
                // restarted-case, has the next needed segment, sounds safe to clear the list of next sets
                OMAF_LOG_V("Switch back to %d done, clear waiting list", mNextVideoBaseAdaptationSets.back()->getId());
                mNextVideoBaseAdaptationSets.clear();

                switched = true;
            }
            else if (mNextVideoBaseAdaptationSets.back()->readyToSwitch(nextSegment))
            {
                OMAF_LOG_V("Switch to %d, waiting %lld", mNextVideoBaseAdaptationSets.back()->getId(),
                           mNextVideoBaseAdaptationSets.getSize());
                // ideal case: we can switch to the last waiting adaptation set
                doSwitch(mNextVideoBaseAdaptationSets.back());

                // remove all waiting ones
                mNextVideoBaseAdaptationSets.clear();

                switched = true;
            }
            // OR if the current one cannot continue any more (no data cached nor under download)
            // we pass the currently downloading adaptation set in as it is not worth checking segments that are
            // being downloaded as they will come eventually
            else if (!((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)
                          ->segmentAvailable(nextSegment, mNextVideoBaseAdaptationSets.back()))
            {
                OMAF_LOG_V("Should switch as current one %d gets empty, waiting %lld", mVideoBaseAdaptationSet->getId(),
                           mNextVideoBaseAdaptationSets.getSize());
                if (mNextVideoBaseAdaptationSets.back()->segmentAvailable(nextSegment))
                {
                    // This is the target one, it is not ready, but has something.
                    // As it was started last, the older ones are useless from now on
                    // remove all waiting ones
                    OMAF_LOG_V("Switch early to %d", mNextVideoBaseAdaptationSets.back()->getId());
                    doSwitch(mNextVideoBaseAdaptationSets.back());
                    mNextVideoBaseAdaptationSets.clear();
                    switched = true;
                }
                else
                {
                    // The last waiting one is not ready yet for switching, try if any other waiting one has even the
                    // needed segment
                    DashAdaptationSetExtractorMR* nextVideoBaseAdaptationSet =
                        hasSegmentAvailable(nextSegment, mNextVideoBaseAdaptationSets.getSize() - 1);
                    if (nextVideoBaseAdaptationSet)
                    {
                        // Found an extractor that could serve at least 1 segment, remove it from the list
                        OMAF_LOG_V("Switch temporarily to %d", nextVideoBaseAdaptationSet->getId());
                        doSwitch(nextVideoBaseAdaptationSet);
                        // remove this one
                        mNextVideoBaseAdaptationSets.remove(nextVideoBaseAdaptationSet);
                        switched = true;
                    }
                    else
                    {
                        // None of them had the required segment, can we expect that they won't, as they should be
                        // stopped? If some of them share a tile with the latest one, which is downloading, that one
                        // could become available, but as it eats segments from the target set, is it worth using it?
                        // Hence, can we remove them from the list now?
                    }
                }
            }
        }
        if (!switched)
        {
            uint32_t nextSegmentBefore =
                ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
            DashVideoDownloaderExtractor::processSegmentDownload();
            uint32_t nextSegmentAfter =
                ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
            if (nextSegmentAfter > nextSegmentBefore)
            {
                // this is called to avoid cache getting full with old segments, that can happen at least if the
                // extractor change is triggered many times before actually switching to it
                for (FixedArray<DashAdaptationSetExtractorMR*, 32>::Iterator nextVideoBaseAdaptationSet =
                         mNextVideoBaseAdaptationSets.begin();
                     nextVideoBaseAdaptationSet != mNextVideoBaseAdaptationSets.end(); ++nextVideoBaseAdaptationSet)
                {
                    (*nextVideoBaseAdaptationSet)->updateProgressDuringSwitch(nextSegmentAfter);
                }
            }
        }
    }
    else
    {
        DashVideoDownloaderExtractor::processSegmentDownload();
    }
}

void_t DashVideoDownloaderExtractorMultiRes::doSwitch(DashAdaptationSetExtractorMR* aNextVideoBaseAdaptationSet)
{


    PastAdaptationSet past;
    past.sources = mVideoBaseAdaptationSet->getCurrentVideoStreams().at(0)->getVideoSources(past.validFrom);
    if (mPastVideoSources.getSize() == mPastVideoSources.getCapacity() - 1)
    {
        mPastVideoSources.removeAt(0);
    }
    mPastVideoSources.add(past);

    uint32_t nextSegment = ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
    OMAF_LOG_V("Switching base adaptation set to %d, at segment %d", aNextVideoBaseAdaptationSet->getId(), nextSegment);
    aNextVideoBaseAdaptationSet->switchToThis(nextSegment);
    ((DashAdaptationSetExtractorMR*) mVideoBaseAdaptationSet)->switchedToAnother();
    mVideoBaseAdaptationSet = aNextVideoBaseAdaptationSet;
    mStreamUpdateNeeded = updateVideoStreams();
    mStreamChangeMonitor.update();  // trigger also renderer thread to update streams

    // do the processing immediately to ensure video stream gets created/updated before any video data is tried to read
    DashVideoDownloaderExtractor::processSegmentDownload();
}

DashAdaptationSetExtractorMR* DashVideoDownloaderExtractorMultiRes::hasSegmentAvailable(uint32_t aSegmentId,
                                                                                        size_t aNrNextSetsToCheck)
{
    OMAF_ASSERT(aNrNextSetsToCheck <= mNextVideoBaseAdaptationSets.getSize(), "Can't check more than is stored");
    for (size_t i = 0; i < aNrNextSetsToCheck; i++)
    {
        DashAdaptationSetExtractorMR* nextVideoBaseAdaptationSet = mNextVideoBaseAdaptationSets.at(i);
        // OMAF_LOG_V("Switch try %d", (*nextVideoBaseAdaptationSet)->getId());
        if (nextVideoBaseAdaptationSet->segmentAvailable(
                aSegmentId,
                mNextVideoBaseAdaptationSets.back()))  // we pass the currently downloading adaptation set in as it is
                                                       // not worth checking segments that are being downloaded as they
                                                       // will come eventually
        {
            return nextVideoBaseAdaptationSet;
        }
    }
    return OMAF_NULL;
}


void_t DashVideoDownloaderExtractorMultiRes::setQualityLevels(uint8_t aFgQuality,
                                                              uint8_t aMarginQuality,
                                                              uint8_t aBgQuality,
                                                              uint8_t aNrQualityLevels)
{
    if (mNextVideoBaseAdaptationSets.isEmpty())
    {
        mVideoBaseAdaptationSet->setQualityLevels(aFgQuality, aMarginQuality, aBgQuality, aNrQualityLevels);
    }
    else
    {
        mNextVideoBaseAdaptationSets.back()->setQualityLevels(aFgQuality, aMarginQuality, aBgQuality, aNrQualityLevels);
    }
}
#ifdef OMAF_ABR_LOGGING
void_t DashVideoDownloaderExtractorMultiRes::printViewportTiles(VASTileSelection& viewport) const
{
    // >> TEST code for analysing VD performance
    char tilelog[1000];
    strcpy(tilelog, "Viewport: ");
    // just a single tile pointing to the extractor, so need to loop through the extractor
    DashAdaptationSetExtractorMR* extractor = (DashAdaptationSetExtractorMR*) viewport.at(0)->getAdaptationSet();

    // a lower bitrate.
    // Further, the polar tiles distort the statistics, at least if the statistics is only based on tile counts, not on
    // coverage. Perhaps they should be analyzed separately?
    FixedArray<uint32_t, 32> fgIds = extractor->getTileIds(TileRole::FOREGROUND);

    for (uint32_t id : fgIds)
    {
        char idStr[10];
        sprintf(idStr, "%d ", id);
        strcat(tilelog, idStr);
    }
    OMAF_LOG_ABR(tilelog);

    strcpy(tilelog, "PolarView: ");
    FixedArray<uint32_t, 32> fgpIds = extractor->getTileIds(TileRole::FOREGROUND_POLE);

    for (uint32_t id : fgpIds)
    {
        char idStr[10];
        sprintf(idStr, "%d ", id);
        strcat(tilelog, idStr);
    }

    OMAF_LOG_ABR(tilelog);
    // << TEST code for analysing VD performance
}
#else
void_t DashVideoDownloaderExtractorMultiRes::printViewportTiles(VASTileSelection& viewport) const
{
}
#endif
OMAF_NS_END
