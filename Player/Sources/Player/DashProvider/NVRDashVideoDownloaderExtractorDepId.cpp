
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
#include "DashProvider/NVRDashVideoDownloaderExtractorDepId.h"
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
OMAF_LOG_ZONE(DashVideoDownloaderExtractorDepId)

DashVideoDownloaderExtractorDepId::DashVideoDownloaderExtractorDepId()
    : DashVideoDownloaderExtractor()
{
}

DashVideoDownloaderExtractorDepId::~DashVideoDownloaderExtractorDepId()
{
}


Error::Enum DashVideoDownloaderExtractorDepId::completeInitialization(DashComponents& aDashComponents, BasicSourceInfo& aSourceInfo)
{
    Error::Enum result;

    SupportingAdaptationSetIds supportingSets;
    RepresentationDependencies dependingRepresentations;

    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        if (DashAdaptationSetExtractor::isExtractor(aDashComponents))
        {
            dependingRepresentations = DashAdaptationSetExtractor::hasDependencies(aDashComponents);
            //TODO just single extractor supported
        }
    }

    // Create adaptation sets based on information parsed from MPD
    uint32_t initializationSegmentId = 0;
    for (uint32_t index = 0; index < aDashComponents.period->GetAdaptationSets().size(); index++)
    {
        dash::mpd::IAdaptationSet* adaptationSet = aDashComponents.period->GetAdaptationSets().at(index);
        aDashComponents.adaptationSet = adaptationSet;
        DashAdaptationSet* dashAdaptationSet = OMAF_NULL;
        if (!dependingRepresentations.isEmpty())
        {
            // TODO dependency case with multiple extractors
            AdaptationSetBundleIds joint;
            supportingSets.add(joint);
            if (DashAdaptationSetTile::isTile(aDashComponents, dependingRepresentations))
            {
                // a tile that an extractor depends on
                uint32_t adSetId = 0;
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetTile)(*this);
                result = ((DashAdaptationSetTile*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId, adSetId);
                if (adSetId > 0)
                {
                    // Also dependency-based linking is handled later on with supportingSets. This must be the first branch here.
                    supportingSets.at(0).partialAdSetIds.add(adSetId);
                }
            }
            else
            {
                // extractor with dependencies
                dashAdaptationSet = OMAF_NEW_HEAP(DashAdaptationSetExtractorDepId)(*this);
                result = ((DashAdaptationSetExtractorDepId*)dashAdaptationSet)->initialize(aDashComponents, initializationSegmentId);
                supportingSets.at(0).mainAdSetId = dashAdaptationSet->getId();
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
            if (!dependingRepresentations.isEmpty())
            {
                // check if it has coverage definitions
                const FixedArray<VASTileViewport*, 32> viewports = ((DashAdaptationSetExtractorDepId*)dashAdaptationSet)->getCoveredViewports();
                if (!viewports.isEmpty())
                {
                    for (FixedArray<VASTileViewport*, 32>::ConstIterator it = viewports.begin(); it != viewports.end(); ++it)
                    {
                        mVideoPartialTiles.add((DashAdaptationSetExtractorDepId*)dashAdaptationSet, **it);
                    }
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


void_t DashVideoDownloaderExtractorDepId::checkVASVideoStreams(uint64_t currentPTS)
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
                OMAF_LOG_V("Tile selection updated, select representations accordingly");
                // dependencyId based extractor, so extractor contains the coverage info
                uint32_t nextProcessedSegment = ((DashAdaptationSetExtractor*)mVideoBaseAdaptationSet)->getNextProcessedSegmentId();
                if (((DashAdaptationSetExtractorDepId*)mVideoBaseAdaptationSet)->selectRepresentation(&additionalTiles.front()->getCoveredViewport(), nextProcessedSegment))
                {
                    mStreamUpdateNeeded = updateVideoStreams();
                    mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
                }
            }
        }
        else if (mStreamUpdateNeeded)
        {
            mStreamUpdateNeeded = updateVideoStreams();
            mVideoStreamsChanged = true;    //trigger also renderer thread to update streams
        }
    }
}

bool_t DashVideoDownloaderExtractorDepId::isReadyToSignalEoS(MP4MediaStream& aStream) const
{
    // in dependency case, we should not signal EOS when switching extractor representations, since we reuse video decoder
    return false;
}


OMAF_NS_END
