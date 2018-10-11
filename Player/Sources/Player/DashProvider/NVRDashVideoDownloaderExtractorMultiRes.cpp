
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
#include "DashProvider/NVRDashVideoDownloaderExtractorMultiRes.h"
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
OMAF_LOG_ZONE(DashVideoDownloaderExtractorMultiRes)

DashVideoDownloaderExtractorMultiRes::DashVideoDownloaderExtractorMultiRes()
    : DashVideoDownloaderExtractor()
    , mNextVideoBaseAdaptationSet(OMAF_NULL)
{
}

DashVideoDownloaderExtractorMultiRes::~DashVideoDownloaderExtractorMultiRes()
{
    release();
}

void_t DashVideoDownloaderExtractorMultiRes::release()
{
    mNextVideoBaseAdaptationSet = OMAF_NULL;
    mAlternateVideoBaseAdaptationSets.clear();

    DashVideoDownloaderExtractor::release();
}

Error::Enum DashVideoDownloaderExtractorMultiRes::completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo)
{
    Error::Enum result;

    SupportingAdaptationSetIds supportingSets;

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
    sourceid_t sourceIdsPerAdaptationSet = (sourceid_t)(supportingSets.at(0).partialAdSetIds.getSize() * 2);// the number of partial adaptation sets gives the limit for the number of sources, *2 for stereo; mono cubemap typically requires 24 (6*4)
    // Check if there is an extractor set with supporting partial sets, and if so, link them. 
    // This is not (full) dupe of what is done above with supportingSets, as here we have the adaptation set objects created and really link the objects, not just the ids
    for (uint32_t index = 0; index < mAdaptationSets.getSize(); index++)
    {
        uint8_t nrQualityLevels = 0;
        DashAdaptationSet* dashAdaptationSet = mAdaptationSets.at(index);
        if (dashAdaptationSet->getType() == AdaptationSetType::EXTRACTOR && dashAdaptationSet->getAdaptationSetContent().matches(MediaContent::Type::HEVC))
        {
            mAlternateVideoBaseAdaptationSets.add((DashAdaptationSetExtractorMR*)dashAdaptationSet);
            mVideoBaseAdaptationSet = dashAdaptationSet;
            // check if it has coverage definitions
            const FixedArray<VASTileViewport*, 32> viewports = ((DashAdaptationSetExtractorMR*)dashAdaptationSet)->getCoveredViewports();
            if (!viewports.isEmpty())
            {
                for (FixedArray<VASTileViewport*, 32>::ConstIterator it = viewports.begin(); it != viewports.end(); ++it)
                {
                    mVideoPartialTiles.add((DashAdaptationSetExtractor*)dashAdaptationSet, **it);
                }
            }
            
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
        if (nrQualityLevels > 0)
        {
            mBitrateController.setNrQualityLevels(nrQualityLevels);
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

    mTilePicker = OMAF_NEW_HEAP(VASTileSetPicker);

    if (!mVideoPartialTiles.isEmpty())
    {
        // server has defined the tilesets per viewing orientation => 1 tileset at a time => overlap is often intentional and makes no harm
        mVideoPartialTiles.allSetsAdded(false);
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
        TileAdaptationSets(),
        mTilePicker);

    return Error::OK;
}

Error::Enum DashVideoDownloaderExtractorMultiRes::startDownload(time_t aStartTime, uint32_t aExpectedPingTimeMs)
{
    mExpectedPingTimeMs = aExpectedPingTimeMs;
    if (mVideoBaseAdaptationSet != OMAF_NULL && mVideoBaseAdaptationSet->getLastSegmentId() == 0)
    {
        bool_t selectionUpdated = false;
        VASTileSelection droppedTiles;
        VASTileSelection additionalTiles;
        VASTileSelection tiles = mTilePicker->getLatestTiles(selectionUpdated, droppedTiles, additionalTiles);
        if (tiles.getSize() == 1)
        {
            mVideoBaseAdaptationSet = tiles.front()->getAdaptationSet();
            if (!mAlternateVideoBaseAdaptationSets.isEmpty())
            {
                ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->switchToThis(1);
            }
        }
        return DashVideoDownloader::startDownload(aStartTime, aExpectedPingTimeMs);
    }
    else if (mNextVideoBaseAdaptationSet != OMAF_NULL)
    {
        return mNextVideoBaseAdaptationSet->startDownload(aStartTime, aExpectedPingTimeMs);
    }
    else
    {
        return DashVideoDownloader::startDownload(aStartTime, aExpectedPingTimeMs);
    }
}

bool_t DashVideoDownloaderExtractorMultiRes::isEndOfStream() const
{
    if (((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->isEndOfStream())
    {
        return true;
    }
    return false;
}


Error::Enum DashVideoDownloaderExtractorMultiRes::stopDownload()
{
    Error::Enum result = DashVideoDownloaderExtractor::stopDownload();

    if (mNextVideoBaseAdaptationSet != OMAF_NULL)
    {
        mNextVideoBaseAdaptationSet->stopDownload();
    }
    return result;
}

void_t DashVideoDownloaderExtractorMultiRes::clearDownloadedContent()
{
    DashVideoDownloaderExtractor::clearDownloadedContent();
    if (mNextVideoBaseAdaptationSet != OMAF_NULL)
    {
        mNextVideoBaseAdaptationSet->clearDownloadedContent();
    }
}

bool_t DashVideoDownloaderExtractorMultiRes::isSeekable()
{
    // not seekable for now, until we get get extractor switch incorporated with seek
    return false;
}

void_t DashVideoDownloaderExtractorMultiRes::checkVASVideoStreams(uint64_t currentPTS)
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
            if (!additionalTiles.isEmpty())
            {
                OMAF_LOG_V("Adding adaptation set %d", additionalTiles.at(0)->getAdaptationSet()->getId());
            }
            if (!droppedTiles.isEmpty())
            {
                OMAF_LOG_V("Dropping %d", droppedTiles.at(0)->getAdaptationSet()->getId());
            }
            // select the best base adaptation set
            if (!additionalTiles.isEmpty())
            {
                if (mNextVideoBaseAdaptationSet != OMAF_NULL)
                {
                    OMAF_LOG_V("Stop the next one under preparation (%d), as new next is set", mNextVideoBaseAdaptationSet->getId());
                    mNextVideoBaseAdaptationSet->stopDownloadAsync(false, (DashAdaptationSetExtractorMR*)(additionalTiles.at(0)->getAdaptationSet()));
                }

                uint64_t targetPtsUs = OMAF_UINT64_MAX;
                uint32_t nextSegmentIndex = ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
                if (nextSegmentIndex == 1)  // TODO live cases?
                {
                    // not even started yet, switch immediately
                    mVideoBaseAdaptationSet->stopDownloadAsync(false);
                    ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->switchingToAnother();
                    ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->switchedToAnother();
                    mVideoBaseAdaptationSet = (DashAdaptationSetExtractorMR*)(additionalTiles.at(0)->getAdaptationSet());
                    mVideoBaseAdaptationSet->startDownload(targetPtsUs, nextSegmentIndex, VideoStreamMode::BASE, mExpectedPingTimeMs);
                    ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->switchToThis(nextSegmentIndex);
                }
                else
                {
                    uint32_t lastDownloadedSegmentIndex = mVideoBaseAdaptationSet->getLastSegmentId();
                    if (lastDownloadedSegmentIndex >= nextSegmentIndex)
                    {
                        // normal case: there are segments waiting to be concatenated already for the current adaptation set too, use them first (TODO latency optimization may change this)
                        nextSegmentIndex = lastDownloadedSegmentIndex + 1;
                    }
                    // Else some tiles etc may be missing the next segment index and they are now stopped, so they won't get downloaded at this point. Hence the starting segment for the new adaptation set should be the next segment to be processed


                    mNextVideoBaseAdaptationSet = (DashAdaptationSetExtractorMR*)(additionalTiles.at(0)->getAdaptationSet());
                    if (mVideoBaseAdaptationSet == mNextVideoBaseAdaptationSet)
                    {
                        // switching back to the currently active one before the previous switch away from it took place; no need to change, but need to restart the download in it. 
                        mNextVideoBaseAdaptationSet = OMAF_NULL;
                        // restart
                        OMAF_LOG_V("Restart the old adaptation set %d", mVideoBaseAdaptationSet->getId());
                        mVideoBaseAdaptationSet->startDownload(targetPtsUs, nextSegmentIndex, VideoStreamMode::BASE, mExpectedPingTimeMs);
                    }
                    else
                    {
                        OMAF_LOG_V("Start switching to extractor adaptation set %d", mNextVideoBaseAdaptationSet->getId());
                        mNextVideoBaseAdaptationSet->startDownload(targetPtsUs, nextSegmentIndex, VideoStreamMode::BASE, mExpectedPingTimeMs);
                        if (!droppedTiles.isEmpty())
                        {
                            ((DashAdaptationSetExtractorMR*)droppedTiles.at(0)->getAdaptationSet())->stopDownloadAsync(false, mNextVideoBaseAdaptationSet);
                        }
                    }
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


bool_t DashVideoDownloaderExtractorMultiRes::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // in multi-resolution case, we should not signal EOS when switching extractors, since we reuse video decoder
    return false;
}

void_t DashVideoDownloaderExtractorMultiRes::processSegmentDownload()
{
    DashVideoDownloaderExtractor::processSegmentDownload();

    if (mNextVideoBaseAdaptationSet != OMAF_NULL)
    {
        // use the specific version to avoid tiles to get extracted in parallel, as that can cause kind of race condition with the main adaptation set with common tiles
        mNextVideoBaseAdaptationSet->processSegmentDownload();

        if (((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->isDone())
        {
            // TODO this works with segment templates, but how about other DASH modes?
            uint32_t nextSegment = ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
            OMAF_LOG_V("Switching base adaptation set to %d, at segment %d", mNextVideoBaseAdaptationSet->getId(), nextSegment);
            mNextVideoBaseAdaptationSet->switchToThis(nextSegment);
            ((DashAdaptationSetExtractorMR*)mVideoBaseAdaptationSet)->switchedToAnother();
            mVideoBaseAdaptationSet = mNextVideoBaseAdaptationSet;
            mNextVideoBaseAdaptationSet = OMAF_NULL;
            mStreamUpdateNeeded = updateVideoStreams();
            mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
        }
    }
}

OMAF_NS_END
