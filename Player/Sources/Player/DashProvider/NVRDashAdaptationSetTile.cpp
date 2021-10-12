
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
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashRepresentationTile.h"
#include "DashProvider/NVRDashSpeedFactorMonitor.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "Media/NVRMediaType.h"
#include "Metadata/NVRCubemapDefinitions.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetTile)

DashAdaptationSetTile::DashAdaptationSetTile(DashAdaptationSetObserver& observer)
    : DashAdaptationSetSubPicture(observer)
    , mNrQualityLevels(0)
    , mRole(TileRole::FOREGROUND)
                                   // decision
    , mTileFlag(0)
{
}

DashAdaptationSetTile::~DashAdaptationSetTile()
{
}

bool_t DashAdaptationSetTile::isTile(dash::mpd::IAdaptationSet* aAdaptationSet,
                                     SupportingAdaptationSetIds& aSupportingIds)
{
    if (DashOmafAttributes::getOMAFCoverageInfo(aAdaptationSet)
        && DashAdaptationSetTile::isPartialAdaptationSet(aAdaptationSet, aSupportingIds))
    {
        return true;
    }
    return false;
}

bool_t DashAdaptationSetTile::isPartialAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet,
                                     SupportingAdaptationSetIds& aSupportingIds)
{
    // actually should also verify base adaptation set mimetype, but not available here
    for (SupportingAdaptationSetIds::Iterator it = aSupportingIds.begin(); it != aSupportingIds.end(); ++it)
    {
        if (!(*it).partialAdSetIds.isEmpty() && (*it).partialAdSetIds.contains(aAdaptationSet->GetId()))
        {
            return true;
        }
    }
    return false;
}

bool_t DashAdaptationSetTile::isTile(dash::mpd::IAdaptationSet* aAdaptationSet,
                                     RepresentationDependencies& aDependableRepresentations)
{
    const std::vector<dash::mpd::IRepresentation*>& repr = aAdaptationSet->GetRepresentation();
    for (std::vector<dash::mpd::IRepresentation*>::const_iterator it = repr.begin(); it != repr.end(); ++it)
    {
        for (RepresentationDependencies::Iterator jt = aDependableRepresentations.begin();
             jt != aDependableRepresentations.end(); ++jt)
        {
            if ((*jt).dependendableRepresentations.contains((*it)->GetId().c_str()))
            {
                // one of the representations of this adaptation set is a dependable, so this is a supporting partial
                // adaptation set
                return true;
            }
        }
    }
    return false;
}

AdaptationSetType::Enum DashAdaptationSetTile::getType() const
{
    return AdaptationSetType::TILE;
}

// used in cases where adaptation sets are linked using Preselections
Error::Enum DashAdaptationSetTile::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
{
    mContent.addType(MediaContent::Type::VIDEO_TILE);

    return doInitialize(aDashComponents, aInitializationSegmentId);
}

// used in cases where representations are linked using dependencyId
Error::Enum DashAdaptationSetTile::initialize(DashComponents aDashComponents,
                                              uint32_t& aInitializationSegmentId,
                                              uint32_t& aAdaptationSetId)
{
    mContent.addType(MediaContent::Type::VIDEO_TILE);

    Error::Enum result = doInitialize(aDashComponents, aInitializationSegmentId);
    if (!DashAdaptationSet::externalAdSetIdExists() && mAdaptationSetId == 0)
    {
        // create a local id
        mAdaptationSetId = DashAdaptationSet::getNextAdSetId();
    }
    aAdaptationSetId = mAdaptationSetId;

    return result;
}

Error::Enum DashAdaptationSetTile::doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
{
    Error::Enum result = DashAdaptationSet::doInitialize(aDashComponents, aInitializationSegmentId);

    if (mContent.matches(MediaContent::Type::VIDEO_TILE) && !mRepresentationsByQuality.isEmpty())
    {
        // start with the highest quality
        mCurrentRepresentation = getRepresentationForQuality(1, mNrQualityLevels);
    }
    return result;
}

DashRepresentation* DashAdaptationSetTile::createRepresentation(DashComponents aDashComponents,
                                                                uint32_t aInitializationSegmentId,
                                                                uint32_t aBandwidth)
{
    DashRepresentationTile* newrepresentation = OMAF_NEW(mMemoryAllocator, DashRepresentationTile);
    if (newrepresentation->initialize(aDashComponents, aBandwidth, mContent, aInitializationSegmentId, this, this) !=
        Error::OK)
    {
        OMAF_LOG_W("Non-supported representation skipped");
        OMAF_DELETE(mMemoryAllocator, newrepresentation);
        return OMAF_NULL;
    }

    if (mCoveredViewport == OMAF_NULL)
    {
        // tile is always video
        parseVideoViewport(aDashComponents);
    }

    addRepresentationByQuality(aDashComponents, newrepresentation);

    // set cache mode: auto
    // we are using more conservative cache strategy with extractor tiles than subpictures, since buffer underflow with
    // extractor tiles is more visible to user than with subpictures (no baselayer as backup)
    newrepresentation->setCacheFillMode(true);

    // do not increment aInitializationSegmentId! Tiles don't have their own init segment
    return newrepresentation;
}

void_t DashAdaptationSetTile::addRepresentationByQuality(DashComponents aDashComponents,
                                                         DashRepresentationTile* aRepresentation)
{
    uint8_t qualityIndex = 0;
    bool_t qualityLevelsGlobal = false;
    if (parseVideoQuality(aDashComponents, qualityIndex, qualityLevelsGlobal, aRepresentation))
    {
        aRepresentation->assignQualityLevel(qualityIndex);
        // insert reference to the representation in increasing quality order
        size_t j = 0;
        for (j = 0; j < mRepresentationsByQuality.getSize(); j++)
        {
            if (mRepresentationsByQuality[j]->getQualityLevel() > qualityIndex)
            {
                mRepresentationsByQuality.add(aRepresentation, j);
                OMAF_LOG_ABR("BitrateLevels(Tile/Bitrate/Quality) %d %d %d", getId(), aRepresentation->getBitrate(),
                             qualityIndex);
                break;
            }
        }
        if (j == mRepresentationsByQuality.getSize())
        {
            mRepresentationsByQuality.add(aRepresentation);
            OMAF_LOG_ABR("BitrateLevels(Tile/Bitrate/Quality) %d %d %d", getId(), aRepresentation->getBitrate(),
                         qualityIndex);
        }
    }
}

Error::Enum DashAdaptationSetTile::stopDownload()
{
    for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        OMAF_LOG_V("stopDownload %s", (*it)->getId());
        (*it)->stopDownload();
    }
    return Error::OK;
}

void_t DashAdaptationSetTile::clearDownloadedContent()
{
    DashAdaptationSet::clearDownloadedContent();
    // clear the segments waiting in RepresentationExtractor
    for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        ((DashRepresentationTile*) (*it))->clearDownloadedContent();
    }
}

bool_t DashAdaptationSetTile::processSegmentDownload()
{
    for (DashRepresentation* representation : mRepresentations)
    {
        representation->processSegmentDownload();
    }
    return false;  // no streams changed in this variant, ever
}

bool_t DashAdaptationSetTile::trySwitchingRepresentation(uint32_t aNextSegmentId)
{
    if (mNextRepresentation != OMAF_NULL && mCurrentRepresentation != mNextRepresentation)
    {
        // For ABR, check if there's a switch pending and switch ASAP (on next segment boundary),
        // and synchronously with other sets, rather than waiting until all frames have been processed, since the merged
        // picture should match the viewport ASAP
        mNextRepresentation->cleanUpOldSegments(aNextSegmentId);
        if (readyToSwitch(mNextRepresentation, aNextSegmentId))
        {
            OMAF_LOG_V(
                "trySwitchingRepresentation: next representation has enough data starting from segment %d, switch",
                aNextSegmentId);
            doSwitchRepresentation();
            return true;
        }
        else
        {
            // not safe to switch yet, but check if current one has data left to show
            DashSegment* crSegment = mCurrentRepresentation->peekSegment();
            if (crSegment != OMAF_NULL && crSegment->getSegmentId() == aNextSegmentId)
            {
                // can use the current one still
                OMAF_LOG_V("trySwitchingRepresentation: Keep using the current representation %s",
                           mCurrentRepresentation->getId());
                return false;
            }
            else
            {
                OMAF_LOG_V(
                    "trySwitchingRepresentation: The old repr has no useful data left, has to switch and hope that the "
                    "buffer of the new repr gets filled while playing");
                doSwitchRepresentation();
                return true;
            }
        }
    }
    return false;
}

DashRepresentation* DashAdaptationSetTile::getRepresentationForQuality(uint8_t aQualityLevel, uint8_t aNrLevels)
{
    // note! the aQualityLevel is internal enumeration of the levels. It doesn't necessarily have a direct mapping to
    // quality levels in representation, other than the order i.e. quality level 1 can map to quality ranking value 10
    // in a representation, if 10 is the best of all representations within this adaptation set

    if (mRepresentationsByQuality.isEmpty())
    {
        return mCurrentRepresentation;
    }
    else if (mNrQualityLevels == aNrLevels)
    {
        return mRepresentationsByQuality.at(aQualityLevel -
                                            1);  // quality levels run from 1 to N, the indices from 0 to N-1
    }
    else
    {
        // less steps in this adaptation set than expected, do some mapping
        uint8_t level = max(uint8_t(1), (uint8_t) round(((float32_t) mNrQualityLevels / aNrLevels) * aQualityLevel));
        return mRepresentationsByQuality.at(level - 1);  // quality levels run from 1 to N, the indices from 0 to N-1
    }
}

bool_t DashAdaptationSetTile::prepareForSwitch(uint32_t aNextProcessedSegment, bool_t aAggressiveSwitch)
{
    // initial estimate is the last downloaded segment + 1
    uint32_t nextNotLoadedSegment = getLastSegmentId() + 1;
    if (nextNotLoadedSegment == 1)
    {
        // getLastSegmentId == 0, which can occur e.g. after pause + resume
        OMAF_LOG_V("No segments loaded, start from %d", aNextProcessedSegment);
        nextNotLoadedSegment = aNextProcessedSegment;
        aAggressiveSwitch = false;
    }
    uint32_t estimatedSegment = nextNotLoadedSegment;
    if (aAggressiveSwitch)
    {
        estimatedSegment = estimateSegmentIdForSwitch(aNextProcessedSegment, nextNotLoadedSegment);
        OMAF_LOG_V("prepareForSwitch estimated segment id for %s is %d", mNextRepresentation->getId(),
                   estimatedSegment);
    }
    // else lazy switch e.g. from fg to bg, use the last downloaded segment + 1.

    // OMAF_LOG_V("%s cleanup", getCurrentRepresentationId().getData());
    mNextRepresentation->cleanUpOldSegments(aNextProcessedSegment);
    mNextRepresentation->startDownloadFromSegment(estimatedSegment, aNextProcessedSegment);

    if (aAggressiveSwitch && readyToSwitch(mNextRepresentation, aNextProcessedSegment))
    {
        OMAF_LOG_V("Using already downloaded segment(s) for %s", mNextRepresentation->getId());
        doSwitchRepresentation();
    }

    return true;
}

void_t DashAdaptationSetTile::cleanUpOldSegments(uint32_t aSegmentId)
{
    for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        (*it)->cleanUpOldSegments(aSegmentId);
    }
}

void_t DashAdaptationSetTile::setBufferingTime(uint32_t aBufferingTimeMs)
{
    for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        (*it)->setBufferingTime(aBufferingTimeMs);
    }
}

uint8_t DashAdaptationSetTile::getNrQualityLevels() const
{
    return mNrQualityLevels;
}

bool_t DashAdaptationSetTile::selectQuality(uint8_t aQualityLevel,
                                            uint8_t aNrQualityLevels,
                                            uint32_t aNextProcessedSegment,
                                            TileRole::Enum aRole)
{
    DashRepresentation* nextRepresentation = getRepresentationForQuality(aQualityLevel, aNrQualityLevels);
    if (mNextRepresentation == OMAF_NULL && mCurrentRepresentation == nextRepresentation)
    {
        OMAF_LOG_D("already using the requested representation");
        return false;
    }
    bool_t aggressiveSwitch = true;
    if (aRole == mRole || aRole == TileRole::BACKGROUND || aRole == TileRole::BACKGROUND_POLE)
    {
        // ABR switch (fg->fg or bg->bg) or viewport switch (fg->bg) => use lazy switching, i.e. start from last
        // downloaded + 1
        aggressiveSwitch = false;
    }
    mRole = aRole;
    OMAF_LOG_V("selectQuality new level %d for %s", aQualityLevel, getCurrentRepresentationId().getData());
    if (isABRSwitchOngoing())
    {
        // (eg quick look at left and then turn back => will now wait until left switch is completed, and may never
        // switch back if no new changes?)
        OMAF_LOG_D("already switching to %s - but is it the right one?", mNextRepresentation->getId());
    }

    if (nextRepresentation == mNextRepresentation)
    {
        // Already switching to this quality
        OMAF_LOG_D("already switching to the requested representation");
        return false;
    }
    else if (nextRepresentation == mCurrentRepresentation)
    {
        OMAF_LOG_D("Switching back to %s", mCurrentRepresentation->getId());
        if (mNextRepresentation != OMAF_NULL)
        {
            OMAF_LOG_V("NextRepresentation %s was downloading, stop it", mNextRepresentation->getId());
            mNextRepresentation->stopDownload();
            mNextRepresentation = OMAF_NULL;
        }
        if (mCurrentRepresentation->isDownloading())
        {
            // don't stop, hence no need to restart
            return false;
        }
        else
        {
            uint32_t lastSegment = 0;
            if (aggressiveSwitch)
            {
                OMAF_LOG_V("Restart %s aggressively", getCurrentRepresentationId().getData());
                lastSegment = mCurrentRepresentation->getLastSegmentId();
            }
            else
            {
                OMAF_LOG_V("Restart %s lazy", getCurrentRepresentationId().getData());
                lastSegment = getLastSegmentId();  // this should be safe, no need to have the segments in sequence per
                                                   // representation, since we utilize segments from other reps
            }
            if (lastSegment == 0)
            {
                // can happen e.g. after pause + resume if tile switch takes place before anything downloaded (current
                // was active before pause but switching and now switching back when resuming)
                lastSegment = aNextProcessedSegment;
            }
            else
            {
                lastSegment++;  // don't load it again
            }
            mCurrentRepresentation->startDownloadFromSegment(lastSegment, aNextProcessedSegment);
            return true;
        }
    }
    // else a completely new representation, i.e. nextRepresentation != mCurrentRepresentation && nextRepresentation !=
    // mNextRepresentation
    mCurrentRepresentation->stopDownloadAsync(false, false);  // don't abort to avoid segment mismatches
    if (mNextRepresentation != OMAF_NULL)
    {
        OMAF_LOG_V("NextRepresentation %s was downloading, stop it", mNextRepresentation->getId());
        mNextRepresentation->stopDownload();
    }
    mNextRepresentation = nextRepresentation;

    if (aRole == TileRole::BACKGROUND || aRole == TileRole::BACKGROUND_POLE)
    {
        ((DashRepresentationTile*) nextRepresentation)->setLatencyReq(LatencyRequirement::MEDIUM_LATENCY);
    }
    else
    {
        ((DashRepresentationTile*) nextRepresentation)->setLatencyReq(LatencyRequirement::LOW_LATENCY);
    }

    return prepareForSwitch(aNextProcessedSegment, aggressiveSwitch);
}

void_t DashAdaptationSetTile::prepareQuality(uint8_t aQualityLevel, uint8_t aNrQualityLevels)
{
    mCurrentRepresentation = getRepresentationForQuality(aQualityLevel, aNrQualityLevels);
}

bool_t DashAdaptationSetTile::isBuffering()
{
    if (mCurrentRepresentation->isBuffering())
    {
        OMAF_LOG_D("Still doing initial buffering");
        return true;
    }
    return false;
}

bool_t DashAdaptationSetTile::isBuffering(uint32_t aNextSegmentToBeConcatenated)
{
    if (!mCurrentRepresentation->isDone())
    {
        if (mCurrentRepresentation->isBuffering() && getLastSegmentId() < aNextSegmentToBeConcatenated)
        {
            return true;
        }
        // else some other representation has newer segments
    }
    return false;
}

// This is relevant only with server-based tile selection, i.e. where fg vs bg tiles are part of the extractor
// configuration
void_t DashAdaptationSetTile::setRole(TileRole::Enum aRole)
{
    OMAF_LOG_V("setRole %d for %d", aRole, getId());
    mRole = aRole;
    for (Representations::Iterator repr = mRepresentations.begin(); repr != mRepresentations.end(); ++repr)
    {
        if (aRole == TileRole::BACKGROUND || aRole == TileRole::BACKGROUND_POLE)
        {
            // set segment merging for download on, if supported. 4 segments in typical short VD scenarios reduce the
            // http traffic, without adding too much bandwidth waste
            ((DashRepresentationTile*) *repr)->setLatencyReq(LatencyRequirement::MEDIUM_LATENCY);
        }
        else
        {
            ((DashRepresentationTile*) *repr)->setLatencyReq(LatencyRequirement::LOW_LATENCY);
        }
    }
}

void_t DashAdaptationSetTile::switchMarginRole(bool_t aToMargin)
{
    if (aToMargin)
    {
        if (mRole == TileRole::FOREGROUND)
        {
            mRole = TileRole::FOREGROUND_MARGIN;
        }
    }
    else
    {
        if (mRole == TileRole::FOREGROUND_MARGIN)
        {
            mRole = TileRole::FOREGROUND;
        }
    }
}

TileRole::Enum DashAdaptationSetTile::getRole() const
{
    return mRole;
}

void_t DashAdaptationSetTile::addTileFlag(uint64_t TileFlag)
{
    mTileFlag = TileFlag;
}

uint64_t DashAdaptationSetTile::getTileFlag()
{
    return mTileFlag;
}

bool_t DashAdaptationSetTile::selectRepresentation(DependableRepresentations& aDependencies,
                                                   uint32_t aNextProcessedSegment)
{
    OMAF_ASSERT(mContent.matches(MediaContent::Type::VIDEO_TILE),
                "Called selectRepresentation for other than tile adaptation set!");
    for (Representations::Iterator repr = mRepresentations.begin(); repr != mRepresentations.end(); ++repr)
    {
        if (aDependencies.contains((*repr)->getId()))
        {
            if (mNextRepresentation != OMAF_NULL)
            {
                OMAF_LOG_V("NextRepresentation %s was downloading, stop it", mNextRepresentation->getId());
                mNextRepresentation->stopDownload();
                mNextRepresentation = OMAF_NULL;
            }
            if (mCurrentRepresentation != *repr)
            {
                OMAF_LOG_V("selectRepresentation, found: %s", (*repr)->getId());
                mNextRepresentation = *repr;
                mCurrentRepresentation->stopDownloadAsync(
                    false, false);  // don't abort since that can cause sync issues with segments
                return prepareForSwitch(aNextProcessedSegment,
                                        false);  // lazy switch to background must never be used, as extractor needs
                                                 // exactly the representation it depends on
            }

            return false;
        }
    }
    return false;
}

void_t DashAdaptationSetTile::handleSpeedFactor(float32_t aSpeedFactor)
{
    if (mRole == TileRole::FOREGROUND)  // only the center foreground tiles counted as they dominate the download sizes
    {
        DashSpeedFactorMonitor::addSpeedFactor(aSpeedFactor);
    }
}

bool_t DashAdaptationSetTile::parseVideoProperties(DashComponents& aNextComponents)
{
    if (mRole == TileRole::NON_TILE_PARTIAL_ADAPTATION_SET)
    {
        aNextComponents.basicSourceInfo.sourceType = SourceType::RAW;
        return true;
    }

    SourceType::Enum projection = DashOmafAttributes::getOMAFVideoProjection(aNextComponents);
    if (projection != SourceType::INVALID)
    {
        mSourceType = projection;
        if (mSourceType == SourceType::CUBEMAP)
        {
            aNextComponents.basicSourceInfo.sourceType = SourceType::CUBEMAP;
            Error::Enum rwpk = DashOmafAttributes::hasOMAFVideoRWPK(aNextComponents);
            if (rwpk == Error::OK)
            {
                // There is rwpk property, we should get face info from fileformat rwpk box.
                // In our current design, this is however not that relevant piece of info, since we need that info for
                // the extractor track (we are in tile track here), so we check the existence of rwpk anyway
            }
            else if (rwpk == Error::OK_NO_CHANGE)
            {
                // Default OMAF face order is "LFRDBU", with 2nd row faces rotated right
                aNextComponents.basicSourceInfo.faceOrder =
                    DEFAULT_OMAF_FACE_ORDER;  // a coded representation of LFRDBU
                aNextComponents.basicSourceInfo.faceOrientation =
                    DEFAULT_OMAF_FACE_ORIENTATION;  // a coded representation of 000111. A merge of enum of individual
                                                    // faces / face sections, that can be many
            }
            else
            {
                OMAF_LOG_W("Unsupported RWPK");
                return false;
            }
        }
        else
        {
            // equirect projection. Set the info like for cubemap
            aNextComponents.basicSourceInfo.sourceType = SourceType::EQUIRECTANGULAR_PANORAMA;
        }
    }
    else
    {
        // ignore the track / adaptation set
        return false;
    }
    parseVideoChannel(aNextComponents);

    return true;
}

void_t DashAdaptationSetTile::parseVideoViewport(DashComponents& aNextComponents)
{
    DashAttributes::Coverage coverage;

    Error::Enum result = DashOmafAttributes::getOMAFVideoViewport(aNextComponents, coverage, mSourceType);
    if (result == Error::OK_NO_CHANGE)
    {
        mContent.addType(MediaContent::Type::VIDEO_BASE);
    }
    else if (result == Error::OK)
    {
        if (mSourceType == SourceType::EQUIRECTANGULAR_PANORAMA)
        {
            mSourceType = SourceType::EQUIRECTANGULAR_TILES;
            aNextComponents.basicSourceInfo.sourceType = SourceType::EQUIRECTANGULAR_TILES;

            if (!mContent.matches(MediaContent::Type::VIDEO_TILE))
            {
                // It is not OMAF partial set, but it is a tile, so it must be enhancement tile, wrapped into OMAF
                mContent.addType(MediaContent::Type::VIDEO_ENHANCEMENT);
            }
            if (mCoveredViewport == OMAF_NULL)
            {
                mCoveredViewport = OMAF_NEW(mMemoryAllocator, VASTileViewport)();
            }
            mCoveredViewport->set(coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange,
                                  coverage.elevationRange, VASLongitudeDirection::COUNTER_CLOCKWISE);
        }
        else if (mSourceType == SourceType::CUBEMAP)
        {
            mSourceType = SourceType::CUBEMAP_TILES;
            aNextComponents.basicSourceInfo.sourceType = SourceType::CUBEMAP_TILES;
            if (mCoveredViewport == OMAF_NULL)
            {
                mCoveredViewport = OMAF_NEW(mMemoryAllocator, VASTileViewportCube)();
            }
            mCoveredViewport->set(coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange,
                                  coverage.elevationRange, VASLongitudeDirection::COUNTER_CLOCKWISE);
            OMAF_LOG_V("Created cubemap tile at (%f, %f), range (%f, %f)", coverage.azimuthCenter,
                       coverage.elevationCenter, coverage.azimuthRange, coverage.elevationRange);
        }
    }
    else
    {
        return;
    }
}

bool_t DashAdaptationSetTile::parseVideoQuality(DashComponents& aNextComponents,
                                                uint8_t& aQualityLevel,
                                                bool_t& aGlobal,
                                                DashRepresentation* aLatestRepresentation)
{
    Error::Enum result = Error::OK;
    if (mSourceType == SourceType::EQUIRECTANGULAR_PANORAMA || mSourceType == SourceType::EQUIRECTANGULAR_180)
    {
        result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.adaptationSet->GetAdditionalSubNodes(),
                                                           *mCoveredViewport, aQualityLevel, aGlobal);
    }
    else if (mSourceType == SourceType::EQUIRECTANGULAR_TILES)
    {
        // tiles, check from representation
        result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(),
                                                           *mCoveredViewport, aQualityLevel, aGlobal);
    }
    else if (mSourceType == SourceType::CUBEMAP_TILES)
    {
        VASTileViewport tmpViewport;
        tmpViewport.set(0, 0, 360.f, 180.f, VASLongitudeDirection::COUNTER_CLOCKWISE);
        result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(),
                                                           tmpViewport, aQualityLevel, aGlobal);
    }

    if (aQualityLevel > 0)
    {
        mNrQualityLevels++;
    }
    if (result == Error::OK || result == Error::OK_NO_CHANGE)
    {
        return true;
    }
    return false;
}

void_t DashAdaptationSetTile::doSwitchRepresentation()
{
    OMAF_LOG_V("%d Switch done from %s (%d bps) to %s (%d bps)", (uint32_t)(Time::getClockTimeMs()),
               mCurrentRepresentation->getId(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getId(),
               mNextRepresentation->getBitrate());
    mCurrentRepresentation->switchedToAnother();
    mCurrentRepresentation = mNextRepresentation;
    mNextRepresentation = OMAF_NULL;
}

// Since OMAF requires that representations within an adaptation set have the same resolution, we should pick the
// highest quality one from cache.
bool_t DashAdaptationSetTile::hasSegment(uint32_t aSegmentId, size_t& aSegmentSize)
{
    uint32_t oldestAll = OMAF_UINT32_MAX;
    for (Representations::ConstIterator it = mRepresentationsByQuality.begin(); it != mRepresentationsByQuality.end();
         ++it)
    {
        uint32_t oldest = 0;
        if ((*it)->hasSegment(aSegmentId, oldest, aSegmentSize))
        {
            return true;
        }
        // for logging only
        if (oldest > 0 && oldest < oldestAll)
        {
            oldestAll = oldest;
        }
    }
    if (oldestAll == OMAF_UINT32_MAX)
    {
        OMAF_LOG_V("segment %d for %s (or sisters) not yet available for concatenation, no segments", aSegmentId,
                   getCurrentRepresentationId().getData());
    }
    else
    {
        OMAF_LOG_V("segment %d for %s (or sisters) not yet available for concatenation, oldest %d", aSegmentId,
                   getCurrentRepresentationId().getData(), oldestAll);
    }
    return false;
}

DashSegment* DashAdaptationSetTile::getSegment(uint32_t aSegmentId, uint8_t& aFromQualityLevel)
{
    DashSegment* segment = OMAF_NULL;
    for (Representations::ConstIterator it = mRepresentationsByQuality.begin(); it != mRepresentationsByQuality.end();
         ++it)
    {
        size_t segmentSize = 0;
        uint32_t oldest = 0;
        if ((*it)->hasSegment(aSegmentId, oldest, segmentSize))
        {
            if (segment == OMAF_NULL)
            {
                OMAF_LOG_V("getSegment %d from %s", aSegmentId, (*it)->getId());
                segment = (*it)->getSegment();
                aFromQualityLevel = (*it)->getQualityLevel();
            }
            else
            {
                (*it)->cleanUpOldSegments(aSegmentId);
            }
        }
    }
    if (segment == OMAF_NULL)
    {
        OMAF_LOG_V("segment %d for %s (or sisters) not yet available for concatenation", aSegmentId,
                   getCurrentRepresentationId().getData());
    }
    return segment;
}

uint32_t DashAdaptationSetTile::getLastSegmentId(bool_t aIncludeSegmentInDownloading) const
{
    uint32_t latest = 0;
    for (Representations::ConstIterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        uint32_t current = (*it)->getLastSegmentId(aIncludeSegmentInDownloading);
        if (current > latest)
        {
            latest = current;
        }
    }
    return latest;
}

uint32_t DashAdaptationSetTile::getLastSegmentIdByQ(bool_t aIncludeSegmentInDownloading) const
{
    uint32_t latest = 0;
    for (Representations::ConstIterator it = mRepresentationsByQuality.begin();
         it != mRepresentationsByQuality.end() && (*it)->getQualityLevel() <= mCurrentRepresentation->getQualityLevel();
         ++it)
    {
        uint32_t current = (*it)->getLastSegmentId();
        if (current > latest)
        {
            latest = current;
        }
    }
    return latest;
}


OMAF_NS_END
