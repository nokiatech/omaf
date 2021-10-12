
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
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashAdaptationSetOverlay.h"
#include "DashProvider/NVRDashAdaptationSetOverlayMeta.h"
#include "DashProvider/NVRDashAdaptationSetSubPicture.h"
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashBitrateController.h"
#include "DashProvider/NVRDashLog.h"
#include "DashProvider/NVRDashSpeedFactorMonitor.h"
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
OMAF_LOG_ZONE(DashVideoDownloaderExtractor)

DashVideoDownloaderExtractor::DashVideoDownloaderExtractor()
    : DashVideoDownloader()
    , mVideoPartialTiles()
{
}

DashVideoDownloaderExtractor::~DashVideoDownloaderExtractor()
{
}

void_t DashVideoDownloaderExtractor::release()
{
    stopDownload();

    if (mVideoBaseAdaptationSet != OMAF_NULL)
    {
        VideoDecoderManager::getInstance()->releaseSharedStreamID(
            ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)->getVideoStreamId());
    }

    // first delete extractors, as they may have linked segments from supporting sets
    for (size_t i = 0; i < mAdaptationSets.getSize();)
    {
        if (mAdaptationSets.at(i)->getType() == AdaptationSetType::EXTRACTOR)
        {
            DashAdaptationSet* set = mAdaptationSets.at(i);
            mAdaptationSets.removeAt(i);
            OMAF_DELETE_HEAP(set);
        }
        else
        {
            i++;
        }
    }
    // then delete the rest
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

Error::Enum DashVideoDownloaderExtractor::completeInitialization(DashComponents& aDashComponents,
                                                                 SourceDirection::Enum aUriSourceDirection,
                                                                 sourceid_t aSourceIdBase)
{
    Error::Enum result;

    mBitrateController = OMAF_NEW_HEAP(DashBitrateContollerExtractor);

    SupportingAdaptationSetIds supportingSets;

    // it was already checked that this variant has preselection-based extractor(s)
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().getSize(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        if (DashAdaptationSetExtractor::isExtractor(adaptationSet))
        {
            supportingSets.add(DashAdaptationSetExtractor::hasPreselection(
                adaptationSet, DashAdaptationSetExtractor::parseId(adaptationSet)));
        }
    }

    if (!supportingSets.isEmpty())
    {
        // Create adaptation sets based on information parsed from MPD
        uint32_t initializationSegmentId = 0;
        for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().getSize(); index++)
        {
            dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
            aDashComponents.adaptationSet = adaptationSet;
            DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
            if (DashAdaptationSetExtractor::isExtractor(adaptationSet))
            {
                // extractor with Preselections-descriptor
                if (DashAdaptationSetExtractor::hasMultiResolution(adaptationSet))
                {
                    dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractorMR)(*this);
                    result = ((DashAdaptationSetExtractorMR*) dashAdaptationSet)
                                 ->initialize(aDashComponents, initializationSegmentId);
                }
                else
                {
                    dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractor)(*this);
                    result = ((DashAdaptationSetExtractor*) dashAdaptationSet)
                                 ->initialize(aDashComponents, initializationSegmentId);
                }
            }
            else if (DashAdaptationSetTile::isTile(adaptationSet, supportingSets))
            {
                // a tile that an extractor depends on through Preselections-descriptor
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result =
                    ((DashAdaptationSetTile*) dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
            }
            else if (DashAdaptationSetOverlay::isOverlay(adaptationSet))
            {
                OMAF_LOG_V("Creating Overlay adaptation set %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlay)(*this);
                result = ((DashAdaptationSetOverlay*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId);
                mOvlyAdaptationSets.add(dashAdaptationSet);
            }
            else if (DashAdaptationSetOverlayMeta::isOverlayMetadata(adaptationSet))
            {
                OMAF_LOG_V("Creating DYOL adaptationset %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
                result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_DYOL);
                mOvlyMetadataAdaptationSets.add(dashAdaptationSet);
            }
            else if (DashAdaptationSetOverlayMeta::isOverlayRvpoMetadata(adaptationSet))
            {
                OMAF_LOG_V("Creating Rvpo adaptationset (reusing overlay meta class) %d", aDashComponents.adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetOverlayMeta)(*this);
                result = ((DashAdaptationSetOverlayMeta*) dashAdaptationSet)
                             ->initialize(aDashComponents, initializationSegmentId, MediaContent::Type::METADATA_INVO);
                mOvlyRvpoMetadataAdaptationSets.add(dashAdaptationSet);
            }
            else if (DashAdaptationSetTile::isPartialAdaptationSet(adaptationSet, supportingSets))
            {
                OMAF_LOG_V("A partial adaptation set which is actually not a tile %d", adaptationSet->GetId());
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                ((DashAdaptationSetTile*) dashAdaptationSet)->setRole(TileRole::NON_TILE_PARTIAL_ADAPTATION_SET);
                result =
                    ((DashAdaptationSetTile*) dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);

            }
            else
            {
                aDashComponents.nonVideoAdaptationSets.add(adaptationSet);
                continue;
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
    }

    if (mAdaptationSets.isEmpty())
    {
        // No adaptations sets found, return failure
        return Error::NOT_SUPPORTED;
    }

    mVideoBaseAdaptationSet = OMAF_NULL;

    sourceid_t sourceIdBase = aSourceIdBase;
    sourceid_t sourceIdsPerAdaptationSet = (sourceid_t) supportingSets.at(0).partialAdSetIds.getSize() * 2;
    // the number of partial adaptation sets gives the limit for the number of sources, *2 for stereo; mono
    // cubemap typically requires 24 (6*4)
    // Check if there is an extractor set with supporting partial sets, and if so, link them.
    // This is not (full) dupe of what is done above with supportingSets, as here we have the adaptation set objects
    // created and really link the objects, not just the ids
    uint8_t nrQualityLevels = 0;
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getType() == AdaptationSetType::EXTRACTOR &&
            dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HEVC))
        {
            mVideoBaseAdaptationSet = dashAdaptationSet;

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
                            }
                        }
                    }
                    break;
                }
            }
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_TILE))
        {
            // a tile in OMAF VD scheme. In case this is not a depencencyId-based system (where extractor represents
            // tiles), add to tile selection but do not start/stop adaptation set from this level, but let the extractor
            // handle it
            if (dashAdaptationSet->getCoveredViewport() != OMAF_NULL)
            {
                mVideoPartialTiles.add(OMAF_NEW_HEAP(VASTileContainer)((DashAdaptationSetTile*) dashAdaptationSet,
                                                                       *dashAdaptationSet->getCoveredViewport()));
                uint8_t qualities = ((DashAdaptationSetTile*) dashAdaptationSet)->getNrQualityLevels();
                // find the max # of levels and store it to bitrate controller
                if (qualities > nrQualityLevels)
                {
                    nrQualityLevels = qualities;
                }
            }
        }
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

    // OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No extractor set for video base adaptation set");
    if (!mVideoBaseAdaptationSet)
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }
    if (mVideoPartialTiles.isEmpty())
    {
        OMAF_LOG_E("Stream needs to have video tiles");
        return Error::NOT_SUPPORTED;
    }
    if (nrQualityLevels > 0)
    {
        mBitrateController->setNrQualityLevels(nrQualityLevels);
    }


    mTilePicker = OMAF_NEW_HEAP(VASTilePicker);

    if (!mVideoPartialTiles.isEmpty())
    {
        mVideoPartialTiles.allSetsAdded(true);
        // we start with all tiles in high quality => dependency to adaptation set - assuming it picks the highest
        // quality at first
        mTilePicker->setAllSelected(mVideoPartialTiles);
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

    estimateBitrates(nrQualityLevels);

    mBitrateController->initialize(this);

    // Video stream id plays a special role with extractors, it must be shared between the representations.
    // Typically RWMQ extractors have only one representation, but as creating shared ID should not cause any harm
    // let's use it for this case too in case there will be multiple representations.
    streamid_t videoStreamId = VideoDecoderManager::getInstance()->generateUniqueStreamID(true);
    ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)->setVideoStreamId(videoStreamId);

    return Error::OK;
}

void_t DashVideoDownloaderExtractor::estimateBitrates(uint8_t aNrQualityLevels)
{
    // loop through viewports in steps
    float32_t vpHeight = 112.f;
    float32_t vpWidth = 112.f;
    FixedArray<ViewportRate, 200> vpBitrates;
    for (float64_t latitude = 50.f; latitude >= -50.f; latitude -= 50.f)
    {
        VASTileRows rows;
        mVideoPartialTiles.getRows(rows, StereoRole::MONO, latitude + vpHeight / 2, latitude - vpHeight / 2);
        if (rows.isEmpty())
        {
            OMAF_LOG_D("no tile rows found");
            continue;
        }
        VASTiles tilesInRow = rows.at(0)->getTiles();
        for (size_t i = 0; i < tilesInRow.getSize(); i++)
        {
            float64_t left, right;
            tilesInRow.at(tilesInRow.getSize() - i - 1)->getCoveredViewport().getLeftRight(left, right);

            // estimate bitrates for viewport starting at

            // a tile boundary
            estimateBitrateForVP(latitude + vpHeight / 2, latitude - vpHeight / 2, left, left - vpWidth,
                                 aNrQualityLevels, vpBitrates);

            // a position close to the right tile boundary but starting inside this tile => can get more tiles included
            // in the viewport
            left = right + 5.f;
            estimateBitrateForVP(latitude + vpHeight / 2, latitude - vpHeight / 2, left, left - vpWidth,
                                 aNrQualityLevels, vpBitrates);
        }
    }
    if (vpBitrates.isEmpty())
    {
        return;
    }
    // pick the set with a) max high q bitrate (worst case), or b) median high q bitrate, or c) 75th percentile bitrate
    ViewportRate rate;
    // worst case
    // rate = vpBitrates.at(0);
    // median
    rate = vpBitrates.at(vpBitrates.getSize() / 2);
    // 75th percentile
    // rate = vpBitrates.at(vpBitrates.getSize() / 4);

    ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)
        ->calculateBitrates(mTilePicker->estimateTiles(mVideoPartialTiles, rate.top, rate.top - vpHeight, rate.left,
                                                       rate.left - vpWidth),
                            aNrQualityLevels);
}

void_t DashVideoDownloaderExtractor::estimateBitrateForVP(float64_t top,
                                                          float64_t bottom,
                                                          float64_t left,
                                                          float64_t right,
                                                          uint8_t aNrQualityLevels,
                                                          FixedArray<ViewportRate, 200>& aBitrates)
{
    ViewportRate rate;
    rate.top = top;
    rate.left = left;
    rate.bitrate = ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)
                       ->calculateBitrate(mTilePicker->estimateTiles(mVideoPartialTiles, top, bottom, left, right),
                                          aNrQualityLevels, 1, aNrQualityLevels);
    bool_t added = false;
    for (size_t i = 0; i < aBitrates.getSize(); i++)
    {
        if (rate.bitrate > aBitrates.at(i).bitrate)
        {
            aBitrates.add(rate, i);
            added = true;
            break;
        }
    }
    if (!added)
    {
        aBitrates.add(rate);
    }
    OMAF_LOG_ABR("VP (top,left) = (%f,%f) bitrate %d", top, left, rate.bitrate);
}


const MP4VideoStreams& DashVideoDownloaderExtractor::selectSources(float64_t longitude,
                                                                   float64_t latitude,
                                                                   float64_t roll,
                                                                   float64_t width,
                                                                   float64_t height,
                                                                   bool_t& aViewportSet)
{
    if (!mTileSetupDone)
    {
        if (!mVideoPartialTiles.isEmpty())
        {
            // need to initialize the picker from renderer thread too (now we know the viewport width & height)
            mTilePicker->setupTileRendering(mVideoPartialTiles, width, height, mBaseLayerDecoderPixelsinSec,
                                            VASLongitudeDirection::COUNTER_CLOCKWISE);
            mTileSetupDone = true;
        }
    }

    aViewportSet = false;
    if (!mVideoPartialTiles.isEmpty())
    {
        if (mTilePicker->setRenderedViewPort(longitude, latitude, roll, width, height))
        {
            VASTileSelection& viewport = mTilePicker->pickTiles(mVideoPartialTiles, mBaseLayerDecoderPixelsinSec);
            if (!viewport.isEmpty())
            {
                printViewportTiles(viewport);
                aViewportSet = true;
            }
        }
    }
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

    return mRenderingVideoStreams;
}

Error::Enum DashVideoDownloaderExtractor::startDownload(time_t aStartTime, uint32_t aBufferingTimeMs)
{
    initializeTiles();
    Error::Enum result = DashVideoDownloader::startDownload(aStartTime, aBufferingTimeMs);
    DashSpeedFactorMonitor::start(
        mTilePicker->getNrVisibleTiles(mVideoPartialTiles, 112.f, 112.f, VASLongitudeDirection::COUNTER_CLOCKWISE));
    return result;
}

Error::Enum DashVideoDownloaderExtractor::startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs)
{
    initializeTiles();
    Error::Enum result = DashVideoDownloader::startDownloadFromTimestamp(aTargetTimeMs, aBufferingTimeMs);
    DashSpeedFactorMonitor::start(
        mTilePicker->getNrVisibleTiles(mVideoPartialTiles, 112.f, 112.f, VASLongitudeDirection::COUNTER_CLOCKWISE));
    return result;
}

void_t DashVideoDownloaderExtractor::updateStreams(uint64_t aCurrentPlayTimeUs)
{
    checkVASVideoStreams(aCurrentPlayTimeUs);
    DashVideoDownloader::updateStreams(aCurrentPlayTimeUs);
}
void_t DashVideoDownloaderExtractor::processSegments()
{
    ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)->concatenateIfReady();
}

void_t DashVideoDownloaderExtractor::checkVASVideoStreams(uint64_t currentPTS)
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
        VASTileSelection toBackground;
        VASTileSelection toViewport;
        VASTileSelection viewportTiles = mTilePicker->getLatestTiles(selectionUpdated, toViewport, toBackground);

        if (selectionUpdated)
        {
            uint32_t nextProcessedSegment =
                ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();

            // single resolution - multiple qualities, supporting sets identified with Preselections, so each of them
            // contains coverage info


            // first switch the selected new tiles to higher quality
            uint8_t nrLevels = 0;
            uint8_t level = 0;
            if (mBitrateController->getQualityLevelForeground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = toViewport.begin(); it != toViewport.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::FOREGROUND);
                }
            }

            // then find out possible margin and set margin tiles to margin quality, updating also aToBackground
            size_t nrMarginTiles = 0;
            if (mBitrateController->getQualityLevelMargin(viewportTiles.getSize(), nrMarginTiles, level))
            {
                VASTileSelection marginTiles = mTilePicker->getLatestMargins(nrMarginTiles, toBackground);

                for (VASTileSelection::Iterator it = marginTiles.begin(); it != marginTiles.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::FOREGROUND_MARGIN);
                }
            }
            // and finally drop tiles to background quality
            if (mBitrateController->getQualityLevelBackground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = toBackground.begin(); it != toBackground.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::BACKGROUND);
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

void_t DashVideoDownloaderExtractor::initializeTiles()
{
    if (!mVideoPartialTiles.isEmpty())
    {
        bool_t selectionUpdated = false;
        VASTileSelection toViewport;
        VASTileSelection toBackground;
        VASTileSelection viewportTiles = mTilePicker->getLatestTiles(selectionUpdated, toViewport, toBackground);

        if (selectionUpdated)
        {
            uint32_t nextProcessedSegment =
                ((DashAdaptationSetExtractor*) mVideoBaseAdaptationSet)->getNextProcessedSegmentId();

            // single resolution - multiple qualities, supporting sets identified with Preselections, so each of them
            // contains coverage info

            // first switch the selected new tiles to higher quality
            uint8_t nrLevels = 0;
            uint8_t level = 0;
            if (mBitrateController->getQualityLevelForeground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = toViewport.begin(); it != toViewport.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::FOREGROUND);
                }
            }

            // then find out possible margin and set margin tiles to margin quality, updating also aToBackground
            size_t nrMarginTiles = 0;
            if (mBitrateController->getQualityLevelMargin(viewportTiles.getSize(), nrMarginTiles, level))
            {
                VASTileSelection marginTiles = mTilePicker->getLatestMargins(nrMarginTiles, toBackground);

                for (VASTileSelection::Iterator it = marginTiles.begin(); it != marginTiles.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::FOREGROUND_MARGIN);
                }
            }
            // and finally drop tiles to background quality
            if (mBitrateController->getQualityLevelBackground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = toBackground.begin(); it != toBackground.end(); ++it)
                {
                    ((DashAdaptationSetTile*) (*it)->getAdaptationSet())
                        ->selectQuality(level, nrLevels, nextProcessedSegment, TileRole::BACKGROUND);
                }
            }
        }
        return;
    }
}
#ifdef OMAF_ABR_LOGGING
void_t DashVideoDownloaderExtractor::printViewportTiles(VASTileSelection& viewport) const
{
    // >> TEST code for analysing VD performance
    // for test log only, so can use std::string
    char tilelog[1000];
    strcpy(tilelog, "Viewport: ");
    float64_t area = 0.f;
    for (VASTileContainer* tile : viewport)
    {
        char id[10];
        sprintf(id, "%d ", tile->getAdaptationSet()->getId());
        strcat(tilelog, id);
        area += tile->getIntersectionArea();
    }
    OMAF_LOG_ABR(tilelog);
    char tileArealog[1000];
    strcpy(tileArealog, "ViewportShare: ");
    for (VASTileContainer* tile : viewport)
    {
        char tileAreaStr[12];
        float64_t tileArea = 100 * tile->getIntersectionArea() / area;
        sprintf(tileAreaStr, "%d:%.2f ", tile->getAdaptationSet()->getId(), tileArea);
        strcat(tileArealog, tileAreaStr);
    }
    OMAF_LOG_ABR(tileArealog);

    // << TEST code for analysing VD performance
}
#else
void_t DashVideoDownloaderExtractor::printViewportTiles(VASTileSelection& viewport) const
{
}
#endif

OMAF_NS_END
