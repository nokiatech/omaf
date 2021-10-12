
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
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "Foundation/NVRLogger.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetExtractorMR)

DashAdaptationSetExtractorMR::DashAdaptationSetExtractorMR(DashAdaptationSetObserver& observer)
    : DashAdaptationSetExtractor(observer)
    , mIsInUse(false)
{
}

DashAdaptationSetExtractorMR::~DashAdaptationSetExtractorMR()
{
}

DashRepresentation* DashAdaptationSetExtractorMR::createRepresentation(DashComponents aDashComponents,
                                                                       uint32_t aInitializationSegmentId,
                                                                       uint32_t aBandwidth)
{
    DashRepresentationExtractor* newrepresentation =
        (DashRepresentationExtractor*) DashAdaptationSetExtractor::createRepresentation(
            aDashComponents, aInitializationSegmentId, aBandwidth);

    if (newrepresentation != OMAF_NULL)
    {
        // if extractor track has a coverage definition, assign it to representation level.
        newrepresentation->setCoveredViewport(mCoveredViewport);
        // ownership was passed
        mCoveredViewport = OMAF_NULL;
        // if we have coverage in the extractor, the dl manager should ignore coverages in tile adaptation sets

        // dependencyId based extractors also are expected to have quality descriptor
        uint8_t qualityIndex = 0;
        bool_t qualityLevelsGlobal = false;
        parseVideoQuality(aDashComponents, qualityIndex, qualityLevelsGlobal, newrepresentation);
        // extractor representations are not really ordered by quality, as they contain dependencies to both high and
        // low quality tiles
    }

    return newrepresentation;
}


// in extractor tracks, viewport can be stored in representations too
const FixedArray<VASTileViewport*, 32> DashAdaptationSetExtractorMR::getCoveredViewports() const
{
    OMAF_ASSERT(mCoveredViewport == OMAF_NULL, "Wrong variant of getCoveredViewport used!");
    FixedArray<VASTileViewport*, 32> viewports;
    for (Representations::ConstIterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        viewports.add(((DashRepresentationExtractor*) (*it))->getCoveredViewport());
    }
    return viewports;
}

uint32_t DashAdaptationSetExtractorMR::getLastSegmentId() const
{
    if (mCurrentRepresentation)
    {
        uint32_t lastId = mCurrentRepresentation->getLastSegmentId();
        for (SupportingAdaptationSets::ConstIterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            uint32_t id = (*it)->getLastSegmentId();
            if (id > 0 && id < lastId)
            {
                lastId = id;
            }
        }
        return lastId;
    }
    else
    {
        return INVALID_SEGMENT_INDEX;
    }
}

bool_t DashAdaptationSetExtractorMR::parseVideoQuality(DashComponents& aNextComponents,
                                                       uint8_t& aQualityLevel,
                                                       bool_t& aGlobal,
                                                       DashRepresentation* aLatestRepresentation)
{
    Error::Enum result = DashOmafAttributes::getOMAFQualityRanking(
        aNextComponents.representation->GetAdditionalSubNodes(),
        *((DashRepresentationExtractor*) aLatestRepresentation)->getCoveredViewport(), aQualityLevel, aGlobal);

    if (result == Error::OK_NO_CHANGE)
    {
        result = DashOmafAttributes::getOMAFQualityRanking(
            aNextComponents.adaptationSet->GetAdditionalSubNodes(),
            *((DashRepresentationExtractor*) aLatestRepresentation)->getCoveredViewport(), aQualityLevel, aGlobal);
        if (result == Error::OK || result == Error::OK)
        {
            return true;
        }
    }
    return false;
}

Error::Enum DashAdaptationSetExtractorMR::startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                                   uint32_t aNextToBeProcessedSegmentId,
                                                                   uint32_t aBufferingTimeMs)
{
    uint32_t targetSegmentId = aTargetDownloadSegmentId;
    Error::Enum result = DashAdaptationSetExtractor::startDownloadFromSegment(
        targetSegmentId, aNextToBeProcessedSegmentId, aBufferingTimeMs);

    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        if (!(*it)->isActive())
        {
            uint32_t supportingTargetSegmentId = aTargetDownloadSegmentId;
            uint32_t lastAvailableSegment = (*it)->getLastSegmentIdByQ();
            if (lastAvailableSegment > aNextToBeProcessedSegmentId)
            {
                // It is dangerous to leave gaps in the segments queue.
                // The aNextToBeProcessedSegmentId should be used when restarting the adaptation set / representation,
                // ie. after switch to another one was initiated but was cancelled since user turned the head back

                // Either we optimize for latency and let the player to utilize earlier downloaded segment(s) even
                // though there is a risk that downloading new segments from that onwards rather than from
                // aTargetDownloadSegmentId is waste of bandwidth (but that is must if we are switching back before
                // previous switch completed, i.e. restarting this representation), or we just keep the
                // aTargetDownloadSegmentId and remove all older than that
                OMAF_LOG_V("Some segments for %s exist, newest segment %d start from it +1",
                           (*it)->getRepresentationId(), lastAvailableSegment);

                supportingTargetSegmentId = lastAvailableSegment + 1;
            }

            OMAF_LOG_V("start downloading supporting set %d for extractor set %d", (*it)->getId(), mAdaptationSetId);
            (*it)->startDownloadFromSegment(supportingTargetSegmentId, aNextToBeProcessedSegmentId, aBufferingTimeMs);
        }
    }
    aTargetDownloadSegmentId = targetSegmentId;
    return result;
}

Error::Enum DashAdaptationSetExtractorMR::stopDownloadAsync(bool_t aReset,
                                                            DashAdaptationSetExtractorMR* aNewAdaptationSet)
{
    OMAF_LOG_V("stopDownloadAsync for %d", mAdaptationSetId);
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if (aNewAdaptationSet == OMAF_NULL || !aNewAdaptationSet->hasSupportingSet((*it)->getId()))
            {
                OMAF_LOG_V("Stop supporting set %d", (*it)->getId());
                (*it)->stopDownloadAsync(false, aReset);  // in multi-resolution case we don't want to abort, as that
                                                          // can lead to segment sync issues
            }
        }
    }
    return DashAdaptationSet::stopDownloadAsync(false, aReset);
}

bool_t DashAdaptationSetExtractorMR::hasSupportingSet(uint32_t aId)
{
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if ((*it)->getId() == aId)
            {
                return true;
            }
        }
    }
    return false;
}

bool_t DashAdaptationSetExtractorMR::processSegmentDownload()
{
    mCurrentRepresentation->processSegmentDownload();

    if (mSupportingSets.getSize() > 0)
    {
        // this is an OMAF extractor track
        if (getIsInitialized())
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if ((*it)->isActive())
                {
                    (*it)->processSegmentDownload();
                }
                else if (isActive())  // must not re-start a stopped supporting set
                {
                    OMAF_LOG_V("start downloading supporting set %d for extractor set %d", (*it)->getId(),
                               mAdaptationSetId);
                    if (mTargetNextSegmentId > 0)
                    {
                        (*it)->startDownloadFromSegment(mTargetNextSegmentId, mNextSegmentToBeConcatenated,
                                                        mBufferingTimeMs);
                    }
                    else
                    {
                        (*it)->startDownload(mDownloadStartTime, mBufferingTimeMs);
                    }
                }
            }
        }
    }

    return false;
}

void_t DashAdaptationSetExtractorMR::concatenateIfReady()
{
    if (mIsInUse)
    {
        DashAdaptationSetExtractor::concatenateIfReady();
    }
}

void_t DashAdaptationSetExtractorMR::switchToThis(uint32_t aSegmentId)
{
    mIsInUse = true;
    mNextSegmentToBeConcatenated = aSegmentId;
    mCurrentRepresentation->cleanUpOldSegments(aSegmentId);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->cleanUpOldSegments(aSegmentId);
    }
    // there could be mNextSegmentToBeConcatenated-1 in downloading, or not even yet downloaded, so cleanup doesn't
    // ensure the next id with 100%, hence the concatenation need to still check it
}

// should be called when we want to discontinue concatenating segments from this set
void_t DashAdaptationSetExtractorMR::switchingToAnother()
{
    mCurrentRepresentation->switchedToAnother();
    mIsInUse = false;
}

void_t DashAdaptationSetExtractorMR::switchedToAnother()
{
    // need to clear downloaded content, otherwise some common (used) partial set may have last segment in parser and
    // block downloading new segments
    OMAF_LOG_V("switchedToAnother");
    mCurrentRepresentation->releaseSegmentsUntil(mNextSegmentToBeConcatenated);
    mCurrentRepresentation->clearVideoSources();
    mIsInUse = false;
}

bool_t DashAdaptationSetExtractorMR::isEndOfStream()
{
    if (mCurrentRepresentation->isEndOfStream())  // && mCurrentRepresentation->isDone())
    {
        return true;
    }
    return false;
}

bool_t DashAdaptationSetExtractorMR::readyToSwitch(uint32_t aNextNeededSegment)
{
    if (aNextNeededSegment < mTargetNextSegmentId)
    {
        // under preparation, and target was aNextNeededSegment+1; some cleanup of old segments could be useful but
        // when? Now it is done only after swithing to this.
        OMAF_LOG_V("%s Not targeting to this segment %d target %d", getCurrentRepresentationId().getData(),
                   aNextNeededSegment, mTargetNextSegmentId);
        return false;
    }
    if (getLastSegmentId() >= aNextNeededSegment)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if ((*it)->getLastSegmentId() < aNextNeededSegment)
            {
                OMAF_LOG_V("not ready to switch to %d due to missing %s", aNextNeededSegment,
                           (*it)->getCurrentRepresentationId().getData());
                return false;
            }
        }
        // ok, extractor and every supporting set has at least the next needed segment; check if they have enough cache
        if (DashAdaptationSetViewportDep::readyToSwitch(mCurrentRepresentation, aNextNeededSegment))
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if (!(*it)->readyToSwitch((*it)->getCurrentRepresentation(), aNextNeededSegment))
                {
                    OMAF_LOG_V("not ready to switch at %d from %s", aNextNeededSegment,
                               (*it)->getCurrentRepresentationId().getData());
                    return false;
                }
            }
            return true;
        }
        else
        {
            OMAF_LOG_V("not ready to switch at %d from extractor %s", aNextNeededSegment,
                       getCurrentRepresentationId().getData());
        }
    }
    else
    {
        OMAF_LOG_V("not ready to switch at %d from extractor %s (segment unavailable)", aNextNeededSegment,
                   getCurrentRepresentationId().getData());
    }
    return false;
}

void_t DashAdaptationSetExtractorMR::updateProgressDuringSwitch(uint32_t aNextSegmentToBeProcessed)
{
    // this is called to avoid cache getting full with old segments, that can happen at least if the extractor change is
    // triggered many times before actually switching to it
    mCurrentRepresentation->cleanUpOldSegments(aNextSegmentToBeProcessed);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->cleanUpOldSegments(aNextSegmentToBeProcessed);
    }
}

bool_t DashAdaptationSetExtractorMR::segmentAvailable(uint32_t aSegmentId)
{
    if (mCurrentRepresentation->getLastSegmentId(true) < aSegmentId)
    {
        OMAF_LOG_V("Extractor doesn't have the segment %d", aSegmentId);
        return false;
    }
    // checks adaptation sets that are not common with the new one, to see if they are running out of segments
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            // Do not include ongoing downloads
            if ((*it)->getLastSegmentId(false) < aSegmentId)
            {
                OMAF_LOG_V("Tile %s doesn't have the segment %d", (*it)->getCurrentRepresentationId().getData(),
                           aSegmentId);
                return false;
            }
        }
    }
    OMAF_LOG_V("All in %s have segment %d", getCurrentRepresentationId().getData(), aSegmentId);
    return true;
}

bool_t DashAdaptationSetExtractorMR::segmentAvailable(uint32_t aSegmentId,
                                                      DashAdaptationSetExtractorMR* aNewAdaptationSet)
{
    if (mCurrentRepresentation->getLastSegmentId(true) < aSegmentId)
    {
        OMAF_LOG_V("Extractor doesn't have the segment %d", aSegmentId);
        return false;
    }
    // checks adaptation sets that are not common with the new one, to see if they are running out of segments
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if (!aNewAdaptationSet->hasSupportingSet((*it)->getId()))
            {
                // Include also possible incomplete download.
                if ((*it)->getLastSegmentId(true) < aSegmentId)
                {
                    OMAF_LOG_V("Tile %s (stopped) doesn't have the segment %d",
                               (*it)->getCurrentRepresentationId().getData(), aSegmentId);
                    return false;
                }
            }
        }
    }
    OMAF_LOG_V("All in %s have segment %d", getCurrentRepresentationId().getData(), aSegmentId);
    return true;
}

TileAdaptationSets DashAdaptationSetExtractorMR::getSupportingSetsForRole(TileRole::Enum aRole)
{
    TileAdaptationSets sets;
    for (DashAdaptationSetTile* set : mSupportingSets)
    {
        if (set->getRole() == aRole)
        {
            sets.add(set);
        }
    }
    return sets;
}

bool_t DashAdaptationSetExtractorMR::isAtSegmentBoundary(uint32_t& aSegment, uint32_t& aNewestSegmentInParser)
{
    if (mCurrentRepresentation)
    {
        return ((DashRepresentationExtractor*) mCurrentRepresentation)
            ->isAtSegmentBoundary(aSegment, aNewestSegmentInParser);
    }
    return true;  // if no representation, cannot be "undone"
}

bool_t DashAdaptationSetExtractorMR::canConcatenate()
{
    DashSegment* segment = mCurrentRepresentation->peekSegment();
    if (segment != OMAF_NULL)
    {
        uint32_t segmentId = segment->getSegmentId();
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            size_t segmentSize = 0;
            if ((*it)->getCurrentRepresentation()->isDownloading() || (*it)->hasSegment(segmentId, segmentSize))
            {
                // OK
            }
            else
            {
                // not downloading, and does not have the needed segment => cannot concatenate any more
                OMAF_LOG_V("Cannot complete the current extractor since missing segment from %d (%s)", (*it)->getId(),
                           (*it)->getCurrentRepresentationId().getData());
                return false;
            }
        }
        return true;
    }
    return false;
}

bool_t DashAdaptationSetExtractorMR::calculateBitrates(uint8_t aNrQualityLevels)
{
    if (mSupportingSets.isEmpty())
    {
        return false;
    }
    // tiles
    // as it loops through the quality levels and assumes the getRepresentationForQuality maps the level in bg tiles
    // always to bg quality

    // as the extractor defines the tile fg/bg selection, we just take sum of bitrates in the supporting sets without
    // any fg/bg leveling, but if some supporting set has more levels, we run the sum for all levels. The levels in this
    // case are for ABR only
    for (uint8_t quality = aNrQualityLevels; quality > 0; quality--)
    {
        uint32_t bitrate = getRepresentationForQuality(1, aNrQualityLevels)->getBitrate();
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            bitrate += (*it)->getRepresentationForQuality(quality, aNrQualityLevels)->getBitrate();
        }
        // scale up by a factor that include estimated viewport switching overhead
        bitrate *= 1.2f;
        mBitrates.add(bitrate);
    }
    return true;
}

void_t DashAdaptationSetExtractorMR::prepareQualityLevels(const TileAdaptationSets& aFgTiles,
                                                          const TileAdaptationSets& aMarginTiles,
                                                          uint8_t aFgQuality,
                                                          uint8_t aBgQuality,
                                                          uint8_t aMarginQuality,
                                                          uint8_t aNrQualityLevels)
{
    // should be called before starting to download a new tileset; run-time setQualityLevels for ABR is used from
    // DashAdaptationSetExtractor

    OMAF_LOG_V("prepareQualityLevels fg %d bg %d margin %d", aFgQuality, aBgQuality, aMarginQuality);
    for (DashAdaptationSetSubPicture* tile : aFgTiles)
    {
        ((DashAdaptationSetTile*) tile)->switchMarginRole(false);
    }
    for (DashAdaptationSetSubPicture* tile : aMarginTiles)
    {
        ((DashAdaptationSetTile*) tile)->switchMarginRole(true);
    }
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        // there is no next, so use the current one
        TileRole::Enum role = (*it)->getRole();
        if (role == TileRole::FOREGROUND)
        {
            OMAF_LOG_D("Prepare foreground quality of %s to %d", (*it)->getCurrentRepresentationId().getData(),
                       aFgQuality);
            (*it)->prepareQuality(aFgQuality, aNrQualityLevels);
        }
        else if (role == TileRole::FOREGROUND_MARGIN)
        {
            OMAF_LOG_D("Prepare margin quality of %s to %d", (*it)->getCurrentRepresentationId().getData(),
                       aMarginQuality);
            (*it)->prepareQuality(aMarginQuality, aNrQualityLevels);
        }
        else
        {
            OMAF_LOG_D("Prepare background quality of %s to %d", (*it)->getCurrentRepresentationId().getData(),
                       aBgQuality);
            (*it)->prepareQuality(aBgQuality, aNrQualityLevels);
        }
    }
}

size_t DashAdaptationSetExtractorMR::getNrForegroundTiles()
{
    size_t counter = 0;
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        if ((*it)->getRole() == TileRole::FOREGROUND)
        {
            counter++;
        }
    }
    return counter;
}

OMAF_NS_END
