
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
#include "DashProvider/NVRDashVideoDownloaderExtractor.h"
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
#include "VAS/NVRVASTileSetPicker.h"


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

    // first delete extractors, as they may have linked segments from supporting sets
    for (size_t i = 0; i < mAdaptationSets.getSize(); )
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
    //Make sure these are NULL
    mVideoBaseAdaptationSet = OMAF_NULL;
    OMAF_DELETE_HEAP(mTilePicker);
    mTilePicker = OMAF_NULL;
}

Error::Enum DashVideoDownloaderExtractor::completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo)
{
    Error::Enum result;

    SupportingAdaptationSetIds supportingSets;

    // it was already checked that this variant has preselection-based extractor(s)
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        if (DashAdaptationSetExtractor::isExtractor(aDashComponents))
        {
            supportingSets.add(DashAdaptationSetExtractor::hasPreselection(aDashComponents, DashAdaptationSetExtractor::parseId(aDashComponents)));
        }
    }

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (!supportingSets.isEmpty())
        {
            if (DashAdaptationSetExtractor::isExtractor(aDashComponents))
            {
                // extractor with Preselections-descriptor
                if (DashAdaptationSetExtractor::hasMultiResolution(aDashComponents))
                {
                    dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractorMR)(*this);
                    result = ((DashAdaptationSetExtractorMR*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
                }
                else
                {
                    dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractor)(*this);
                    result = ((DashAdaptationSetExtractor*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
                }
            }
            else
            {
                // a tile that an extractor depends on through Preselections-descriptor
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result = ((DashAdaptationSetTile*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
            }
        }
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

    sourceid_t sourceIdBase = 0;
    sourceid_t sourceIdsPerAdaptationSet = (sourceid_t)supportingSets.at(0).partialAdSetIds.getSize() * 2;// the number of partial adaptation sets gives the limit for the number of sources, *2 for stereo; mono cubemap typically requires 24 (6*4)
    // Check if there is an extractor set with supporting partial sets, and if so, link them. 
    // This is not (full) dupe of what is done above with supportingSets, as here we have the adaptation set objects created and really link the objects, not just the ids
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        uint8_t nrQualityLevels = 0;
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getType() == AdaptationSetType::EXTRACTOR && dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HEVC))
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
                            if (otherAdaptationSet != dashAdaptationSet && otherAdaptationSet->getType() == AdaptationSetType::TILE && (supportingSets[k].partialAdSetIds.at(l) == otherAdaptationSet->getId()))
                            {
                                ((DashAdaptationSetExtractor*)dashAdaptationSet)->addSupportingSet((DashAdaptationSetTile*)otherAdaptationSet);
                            }
                        }
                    }
                    break;
                }
            }
        }
        else if (dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::VIDEO_TILE))
        {
            // a tile in OMAF VD scheme. In case this is not a depencencyId-based system (where extractor represents tiles), add to tile selection
            // but do not start/stop adaptation set from this level, but let the extractor handle it
            if (dashAdaptationSet->getCoveredViewport() != OMAF_NULL)
            {
                mVideoPartialTiles.add((DashAdaptationSetTile*)dashAdaptationSet, *dashAdaptationSet->getCoveredViewport());
                uint8_t qualities = ((DashAdaptationSetTile*)dashAdaptationSet)->getNrQualityLevels();
                // find the max # of levels and store it to bitrate controller
                if (qualities > nrQualityLevels)
                {
                    nrQualityLevels = qualities;
                }
            }
        }
        if (nrQualityLevels > 0)
        {
            mBitrateController.setNrQualityLevels(nrQualityLevels);
        }
    }

    //OMAF_ASSERT(mVideoBaseAdaptationSet != OMAF_NULL, "No extractor set for video base adaptation set");
    if (!mVideoBaseAdaptationSet)
    {
        OMAF_LOG_E("Stream needs to have video");
        return Error::NOT_SUPPORTED;
    }

    mTilePicker = OMAF_NEW_HEAP(VASTilePicker);

    if (!mVideoPartialTiles.isEmpty())
    {
        mVideoPartialTiles.allSetsAdded(true);
        // we start with all tiles in high quality => dependency to adaptation set - assuming it picks the highest quality at first
        mTilePicker->setAllSelected(mVideoPartialTiles);
        mMediaInformation.videoType = VideoType::VIEW_ADAPTIVE;
    }

    if (aSourceInfo.sourceDirection != SourceDirection::Enum::MONO || mVideoBaseAdaptationSet->getVideoChannel() == StereoRole::FRAME_PACKED)
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
        OMAF_NULL,
        mVideoPartialTiles.getAdaptationSets(),
        mTilePicker);

    return Error::OK;
}


const MP4VideoStreams& DashVideoDownloaderExtractor::selectSources(float64_t longitude, float64_t latitude, float64_t roll, float64_t width, float64_t height, streamid_t& aBaseLayerStreamId, bool_t& aViewportSet)
{
    if (!mTileSetupDone)
    {
        if (!mVideoPartialTiles.isEmpty())
        {
            // need to initialize the picker from renderer thread too (now we know the viewport width & height)
            mTilePicker->setupTileRendering(mVideoPartialTiles, width, height, mBaseLayerDecoderPixelsinSec);
            mTileSetupDone = true;
        }
    }

    aViewportSet = false;
    if (!mVideoPartialTiles.isEmpty())
    {
        if (mTilePicker->setRenderedViewPort(longitude, latitude, roll, width, height))
        {
            if (!mTilePicker->pickTiles(mVideoPartialTiles, mBaseLayerDecoderPixelsinSec).isEmpty())
            {
                aViewportSet = true;
            }
        }
    }
    if ((mRenderingVideoStreams.isEmpty() || mReselectSources || mVideoStreamsChanged.compareAndSet(false, true)) && mVideoBaseAdaptationSet != OMAF_NULL)
    {
        mRenderingVideoStreams.clear();
        mReselectSources = false;
        mVideoStreamsChanged = false;   // reset in case some other condition triggered too
        size_t count = 1;
        mRenderingVideoStreams.add(mVideoBaseAdaptationSet->getCurrentVideoStreams());
        if (mRenderingVideoStreams.getSize() < count)
        {
            //OMAF_LOG_V("Missing base layer streams for rendering: expected %zd now %zd", count, mRenderingVideoStreams.getSize());
            mReselectSources = true;
        }
    }

    if (!mRenderingVideoStreams.isEmpty())
    {
        aBaseLayerStreamId = mRenderingVideoStreams.at(0)->getStreamId();
    }
    return mRenderingVideoStreams;
}

void_t DashVideoDownloaderExtractor::updateStreams(uint64_t aCurrentPlayTimeUs)
{
    checkVASVideoStreams(aCurrentPlayTimeUs);
    DashVideoDownloader::updateStreams(aCurrentPlayTimeUs);
}

void_t DashVideoDownloaderExtractor::checkVASVideoStreams(uint64_t currentPTS)
{
    if (!mVideoPartialTiles.isEmpty())
    {
        // OMAF
        bool_t selectionUpdated = false;
        VASTileSelection droppedTiles;
        VASTileSelection additionalTiles;
        VASTileSelection tiles = mTilePicker->getLatestTiles(selectionUpdated, droppedTiles, additionalTiles);

        if (selectionUpdated)
        {
            uint32_t nextProcessedSegment = ((DashAdaptationSetExtractor*)mVideoBaseAdaptationSet)->getNextProcessedSegmentId();

            // single resolution - multiple qualities, supporting sets identified with Preselections, so each of them contains coverage info

            // first switch dropped tiles to lower quality
            uint8_t nrLevels = 0;
            uint8_t level = 0;
            if (mBitrateController.getQualityLevelBackground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = droppedTiles.begin(); it != droppedTiles.end(); ++it)
                {
                    ((DashAdaptationSetTile*)(*it)->getAdaptationSet())->selectQuality(level, nrLevels, nextProcessedSegment);
                }
            }
            // then switch the selected new tiles to higher quality
            if (mBitrateController.getQualityLevelForeground(level, nrLevels))
            {
                for (VASTileSelection::Iterator it = additionalTiles.begin(); it != additionalTiles.end(); ++it)
                {
                    ((DashAdaptationSetTile*)(*it)->getAdaptationSet())->selectQuality(level, nrLevels, nextProcessedSegment);
                }
            }
        }
        else if (mStreamUpdateNeeded)
        {
            mStreamUpdateNeeded = updateVideoStreams();
            mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
        }

        return;
    }
}





OMAF_NS_END
