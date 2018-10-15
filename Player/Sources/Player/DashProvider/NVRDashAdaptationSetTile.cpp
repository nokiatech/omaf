
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
#include "DashProvider/NVRDashAdaptationSetTile.h"
#include "DashProvider/NVRDashRepresentationTile.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMediaType.h"
#include "Foundation/NVRTime.h"
#include "Metadata/NVRCubemapDefinitions.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAdaptationSetTile)

    DashAdaptationSetTile::DashAdaptationSetTile(DashAdaptationSetObserver& observer)
    : DashAdaptationSetSubPicture(observer)
    , mNrQualityLevels(0)
    {

    }

    DashAdaptationSetTile::~DashAdaptationSetTile()
    {
    }

    bool_t DashAdaptationSetTile::isTile(DashComponents aDashComponents, AdaptationSetBundleIds& aSupportingIds)
    {
        if (!aSupportingIds.partialAdSetIds.isEmpty() && aSupportingIds.partialAdSetIds.contains(aDashComponents.adaptationSet->GetId()))
        {
            return true;
        }
        return false;
    }
    bool_t DashAdaptationSetTile::isTile(DashComponents aDashComponents, RepresentationDependencies& aDependableRepresentations)
    {
        const std::vector<dash::mpd::IRepresentation*>& repr = aDashComponents.adaptationSet->GetRepresentation();
        for (std::vector<dash::mpd::IRepresentation*>::const_iterator it = repr.begin(); it != repr.end(); ++it)
        {
            for (RepresentationDependencies::Iterator jt = aDependableRepresentations.begin(); jt != aDependableRepresentations.end(); ++jt)
            {
                if ((*jt).dependendableRepresentations.contains((*it)->GetId().c_str()))
                {
                    // one of the representations of this adaptation set is a dependable, so this is a supporting partial adaptation set
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
    Error::Enum DashAdaptationSetTile::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId, uint32_t& aAdaptationSetId)
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

    DashRepresentation* DashAdaptationSetTile::createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth)
    {
        DashRepresentationTile* newrepresentation = OMAF_NEW(mMemoryAllocator, DashRepresentationTile);
        if (newrepresentation->initialize(aDashComponents, aBandwidth, mContent, aInitializationSegmentId, this) != Error::OK)
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
        // we are using more conservative cache strategy with extractor tiles than subpictures, since buffer underflow with extractor tiles is more visible to user than with subpictures (no baselayer as backup)
        newrepresentation->setCacheFillMode(true);

        // do not increment aInitializationSegmentId! Tiles don't have their own init segment
        return newrepresentation;
    }

    void_t DashAdaptationSetTile::addRepresentationByQuality(DashComponents aDashComponents, DashRepresentationTile* aRepresentation)
    {
        uint8_t qualityIndex = 0;
        bool_t qualityLevelsGlobal = false;//TODO is there any use for this?
        if (parseVideoQuality(aDashComponents, qualityIndex, qualityLevelsGlobal, aRepresentation))
        {
            aRepresentation->assignQualityLevel(qualityIndex);
            // insert reference to the representation in quality order
            size_t j = 0;
            for (j = 0; j < mRepresentationsByQuality.getSize(); j++)
            {
                if (mRepresentationsByQuality[j]->getQualityLevel() > qualityIndex)
                {
                    mRepresentationsByQuality.add(aRepresentation, j);
                    break;
                }
            }
            if (j == mRepresentationsByQuality.getSize())
            {
                mRepresentationsByQuality.add(aRepresentation);
            }
        }
    }

    // This is used only with OMAF partial adaptation sets
    bool_t DashAdaptationSetTile::processSegmentDownloadTile(uint32_t aNextSegmentId, bool_t aCanSwitchRepresentations)
    {
        mCurrentRepresentation->processSegmentDownload();

        if (aCanSwitchRepresentations && mNextRepresentation != OMAF_NULL && mCurrentRepresentation != mNextRepresentation)
        {
            //technically the new stream/representation/decoder will get activated HERE!
            //(stream activate gets called when init segment is processed)
            mNextRepresentation->processSegmentDownload();

            // For ABR, check if there's a switch pending and switch ASAP (on next segment boundary), 
            // and synchronously with other sets, rather than waiting until all frames have been processed, since the merged picture should match the viewport ASAP
            mNextRepresentation->cleanUpOldSegments(aNextSegmentId);
            if (readyToSwitch(aNextSegmentId))
            {
                OMAF_LOG_V("processSegmentDownloadTile: next representation has enough data starting from segment %d, switch", peekNextSegmentId());
                doSwitchRepresentation();
                return true;
            }
            else
            {
                // check if current one has data to show
                DashSegment* crSegment = mCurrentRepresentation->peekSegment();
                if (crSegment != OMAF_NULL && crSegment->getSegmentId() == aNextSegmentId)
                {
                    // can use the current one still
                    OMAF_LOG_V("processSegmentDownloadTile: Keep using the current representation %s", mCurrentRepresentation->getId());
                    return false;
                }
                else
                {
                    OMAF_LOG_V("processSegmentDownloadTile: The old repr has no useful data left, has to switch and hope that the buffer of the new repr gets filled while playing");
                    doSwitchRepresentation();
                    return true;
                }
            }
        }
        return false;
    }

    DashRepresentation* DashAdaptationSetTile::getRepresentationForQuality(uint8_t aQualityLevel, uint8_t aNrLevels)
    {
        // note! the aQualityLevel is internal enumeration of the levels. It doesn't necessarily have a direct mapping to quality levels in representation, other than the order
        // i.e. quality level 1 can map to quality ranking value 10 in a representation, if 10 is the best of all representations within this adaptation set

        if (mRepresentationsByQuality.isEmpty())
        {
            return mCurrentRepresentation;
        }
        else if (mNrQualityLevels == aNrLevels)
        {
            return mRepresentationsByQuality.at(aQualityLevel - 1);   // quality levels run from 1 to N, the indices from 0 to N-1
        }
        else
        {
            // less steps in this adaptation set than expected, do some mapping
            uint8_t level = max(uint8_t(1), (uint8_t)round(((float32_t)mNrQualityLevels / aNrLevels) * aQualityLevel));
            return mRepresentationsByQuality.at(level - 1);   // quality levels run from 1 to N, the indices from 0 to N-1
        }
    }

    bool_t DashAdaptationSetTile::prepareForSwitch(uint32_t aNextNeededSegment, bool_t aGoToBackground)
    {
        uint32_t adjustedSegment = aNextNeededSegment;
        uint32_t downloadSkip = 0;
        if (mCurrentRepresentation->getLastSegmentId() == 0)
        {
            // haven't downloaded any segment yet
            adjustedSegment = 1;//TODO live cases??
        }
        else if (aGoToBackground)
        {
            // lazy switch from fg to bg
            adjustedSegment = min(adjustedSegment, mCurrentRepresentation->getLastSegmentId() + 1);
            //OMAF_LOG_D("%lld prepareForSwitch bg: to %s at %d (was %d)", Time::getClockTimeMs(), mNextRepresentation->getId(), adjustedSegment, aNextNeededSegment);
        }
        else
        {
            // check the latest segment mNextRepresentation already has
            if (mNextRepresentation->getLastSegmentId() < adjustedSegment)
            {
                // there are no useful segments cached

                // add download latency
                //TODO getDownloadSpeedFactor should give a global factor, not just for the current representation, as that may have been measured for a few segments only in VD
                downloadSkip = (uint32_t)((float32_t)1 / getDownloadSpeedFactor() + 1);
                adjustedSegment += downloadSkip; //round up as download always takes more than 0 ms

                                                 // ensure there are no gaps when switching from current to next
                adjustedSegment = min(adjustedSegment, mCurrentRepresentation->getLastSegmentId() + 1);
                OMAF_LOG_V("prepareForSwitch adjusted segment id %d", adjustedSegment);
            }
            //OMAF_LOG_D("%lld prepareForSwitch fg: to %s at %d (was %d, download delay: %d segments)", Time::getClockTimeMs(), mNextRepresentation->getId(), adjustedSegment, aNextNeededSegment, downloadSkip);
        }

        mNextRepresentation->cleanUpOldSegments(adjustedSegment);
        mNextRepresentation->startDownloadABR(adjustedSegment);
        if (readyToSwitch(adjustedSegment))
        {
            OMAF_LOG_V("Using already downloaded segment(s) for %s", mNextRepresentation->getId());
            doSwitchRepresentation();
        }

        return true;
    }

    bool_t DashAdaptationSetTile::readyToSwitch(uint32_t aNextNeededSegment)
    {
        DashSegment* segment = mNextRepresentation->peekSegment();
        if (segment == OMAF_NULL || segment->getSegmentId() > aNextNeededSegment)
        {
            return false;
        }
        uint64_t bufferedDataMs = 0;
        uint64_t thresholdMs = 500;
        uint64_t segmentDurationMs = 0;
        if (mNextRepresentation->isSegmentDurationFixed(segmentDurationMs))
        {
            bufferedDataMs = mNextRepresentation->getNrSegments() * segmentDurationMs;
            //TODO getDownloadSpeedFactor should give a global factor, not just for the current representation, as that may have been measured for a few segments only in VD
            thresholdMs = max((uint64_t)(segmentDurationMs / getDownloadSpeedFactor()), (uint64_t)400); // e.g. duration = 1sec, speed factor = 4 (250 msec per segment) => threshold = max(250, 400)
            OMAF_LOG_V("readyToSwitch: buffered %lld, threshold %lld, factor %f", bufferedDataMs, thresholdMs, getDownloadSpeedFactor());
        }
        else
        {
            // in timeline mode the durations are not fixed, so needs some other logic here
            //bufferedDataMs = mNextRepresentation->getNrSegments() * XX
        }

        if (bufferedDataMs > thresholdMs)
        {
            //OMAF_LOG_D("%lld readyToSwitch to %s at %d", Time::getClockTimeMs(), mNextRepresentation->getId(), aNextNeededSegment);
            return true;
        }
        return false;
    }

    void_t DashAdaptationSetTile::setBufferingTime(uint32_t aExpectedPingTimeMs)
    {
        for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
        {
            (*it)->setBufferingTime(aExpectedPingTimeMs);
        }
    }

    uint8_t DashAdaptationSetTile::getNrQualityLevels() const
    {
        return mNrQualityLevels;
    }

    bool_t DashAdaptationSetTile::selectQuality(uint8_t aQualityLevel, uint8_t aNrQualityLevels, uint32_t aNextNeededSegment)
    {
        OMAF_LOG_V("selectQuality new level %d segment %d", aQualityLevel, aNextNeededSegment);
        if (isABRSwitchOngoing())
        {
            //TODO the viewport direction should still be obeyed, perhaps the switching is ok, but it may be obsolete too? (eg quick look at left and then turn back => will now wait until left switch is completed, and may never switch back if no new changes?)
            OMAF_LOG_D("already switching to %s - but is it the right one?", mNextRepresentation->getId());
        }

        DashRepresentation* nextRepresentation = getRepresentationForQuality(aQualityLevel, aNrQualityLevels);
        if (nextRepresentation == mNextRepresentation)
        {
            // Already switching to this quality
            OMAF_LOG_D("already switching to the requested representation");
            return true;
        }
        else if (nextRepresentation == mCurrentRepresentation)
        {
            OMAF_LOG_D("Switching back to", mCurrentRepresentation->getId());
            //TODO anything special? We have stopped mCurrent
            // but if mNext has better Q than mCurr, and it has segments, could we switch to it temporarily? adjustedSegment would then need to be taken from mNext
        }
        mCurrentRepresentation->stopDownloadAsync(false);
        if (mNextRepresentation != OMAF_NULL)
        {
            OMAF_LOG_V("NextRepresentation %s was downloading, stop it", mNextRepresentation->getId());
            mNextRepresentation->stopDownload();
        }
        mNextRepresentation = nextRepresentation;

        return prepareForSwitch(aNextNeededSegment, (aQualityLevel >= aNrQualityLevels - 1));
    }

    bool_t DashAdaptationSetTile::selectRepresentation(DependableRepresentations& aDependencies, uint32_t aNextNeededSegment)
    {
        OMAF_ASSERT(mContent.matches(MediaContent::Type::VIDEO_TILE), "Called selectRepresentation for other than tile adaptation set!");
        for (Representations::Iterator repr = mRepresentations.begin(); repr != mRepresentations.end(); ++repr)
        {
            if (aDependencies.contains((*repr)->getId()))
            {
                if (mCurrentRepresentation != *repr)
                {
                    OMAF_LOG_V("selectRepresentation, found: %s", (*repr)->getId());
                    mNextRepresentation = *repr;
                    mCurrentRepresentation->stopDownloadAsync(false);
                    return prepareForSwitch(aNextNeededSegment, false);//TODO can we know if we go to background in this case? By quality rank in representation
                }
                return false;
            }
        }
        return false;
    }

    bool_t DashAdaptationSetTile::parseVideoProperties(DashComponents& aNextComponents)
    {
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
                    // In our current design, this is however not that relevant piece of info, since we need that info for the extractor track (we are in tile track here), so we check the existence of rwpk anyway
                }
                else if (rwpk == Error::OK_NO_CHANGE)
                {
                    // Default OMAF face order is "LFRDBU", with 2nd row faces rotated right
                    aNextComponents.basicSourceInfo.faceOrder = DEFAULT_OMAF_FACE_ORDER;       // a coded representation of LFRDBU
                    aNextComponents.basicSourceInfo.faceOrientation = DEFAULT_OMAF_FACE_ORIENTATION;  // a coded representation of 000111. A merge of enum of individual faces / face sections, that can be many
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
            //ignore the track / adaptation set
            return false;
        }
        // stereo info - TODO may require some OMAF-specific parts
        parseVideoChannel(aNextComponents);

        //TODO parse and handle Viewpoint element - at least before any multi-camera scenarios

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
                mCoveredViewport->set(coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange, coverage.elevationRange, VASLongitudeDirection::COUNTER_CLOCKWISE);
            }
            else if (mSourceType == SourceType::CUBEMAP)
            {
                mSourceType = SourceType::CUBEMAP_TILES;
                aNextComponents.basicSourceInfo.sourceType = SourceType::CUBEMAP_TILES;
                if (mCoveredViewport == OMAF_NULL)
                {
                    mCoveredViewport = OMAF_NEW(mMemoryAllocator, VASTileViewportCube)();
                }
                mCoveredViewport->set(coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange, coverage.elevationRange, VASLongitudeDirection::COUNTER_CLOCKWISE);
                OMAF_LOG_V("Created cubemap tile at (%f, %f), range (%f, %f)", coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange, coverage.elevationRange);
            }
        }
        else
        {
            return;
        }
    }

    bool_t DashAdaptationSetTile::parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityLevel, bool_t& aGlobal, DashRepresentation* aLatestRepresentation)
    {
            Error::Enum result = Error::OK;
            if (mSourceType == SourceType::EQUIRECTANGULAR_PANORAMA || mSourceType == SourceType::EQUIRECTANGULAR_180)
            {
                result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.adaptationSet->GetAdditionalSubNodes(), *mCoveredViewport, aQualityLevel, aGlobal);
            }
            else if (mSourceType == SourceType::EQUIRECTANGULAR_TILES)
            {
                // tiles, check from representation
                result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(), *mCoveredViewport, aQualityLevel, aGlobal);
            }
            else if (mSourceType == SourceType::CUBEMAP_TILES)
            {
                //TODO support quality with cubemaps - probably has impact to equirect quality parsing too
                VASTileViewport tmpViewport;
                tmpViewport.set(0, 0, 360.f, 180.f, VASLongitudeDirection::COUNTER_CLOCKWISE);
                result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(), tmpViewport, aQualityLevel, aGlobal);
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
#ifdef OMAF_PLATFORM_ANDROID
        OMAF_LOG_V("%d Switch done from %d to %d", Time::getClockTimeMs(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getBitrate());
#else
        OMAF_LOG_V("%lld Switch done from %s (%d) to %s (%d)", Time::getClockTimeMs(), mCurrentRepresentation->getId(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getId(), mNextRepresentation->getBitrate());
#endif
        mCurrentRepresentation = mNextRepresentation;
        mNextRepresentation = OMAF_NULL;
    }

OMAF_NS_END
