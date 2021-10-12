
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
#include "DashProvider/NVRDashAdaptationSetExtractor.h"
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "DashProvider/NVRDashRepresentationTile.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRTime.h"
#include "Media/NVRMediaType.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetExtractor)

const float32_t BITRATE_SAFETY_MARGIN = 1.15f;

DashAdaptationSetExtractor::DashAdaptationSetExtractor(DashAdaptationSetObserver& observer)
    : DashAdaptationSetTile(observer)
    , mNextSegmentToBeConcatenated(1)
    , mNrSegmentsProcessed(0)
    , mTargetNextSegmentId(1)
    , mHighestSegmentIdtoDownload(1)
    , mCurrFlagsTileDownload(0)
    , mMaskTileDownload(0)
{
}

DashAdaptationSetExtractor::~DashAdaptationSetExtractor()
{
}

bool_t DashAdaptationSetExtractor::isExtractor(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    for (size_t codecIndex = 0; codecIndex < aAdaptationSet->GetCodecs().size(); codecIndex++)
    {
        const std::string& codec = aAdaptationSet->GetCodecs().at(codecIndex);
        if (codec.find("hvc2") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

uint32_t DashAdaptationSetExtractor::parseId(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    return aAdaptationSet->GetId();
}

AdaptationSetBundleIds DashAdaptationSetExtractor::hasPreselection(dash::mpd::IAdaptationSet* aAdaptationSet,
                                                                   uint32_t aAdaptationSetId)
{
    AdaptationSetBundleIds supportingSetIds;
    // In OMAF, Preselection links adaptation sets. Not relevant with the representations
    DashOmafAttributes::getOMAFPreselection(aAdaptationSet, aAdaptationSetId, supportingSetIds);
    return supportingSetIds;
}

uint32_t DashAdaptationSetExtractor::hasDependencies(dash::mpd::IAdaptationSet* aAdaptationSet,
                                            RepresentationDependencies& dependingRepresentationIds)
{
    // check for dependencies too. We add all representation ids that all representations of this set depends on into
    // the same list, but grouped
    DashOmafAttributes::getOMAFRepresentationDependencies(aAdaptationSet, dependingRepresentationIds);
    return dependingRepresentationIds.getSize();
}

bool_t DashAdaptationSetExtractor::hasMultiResolution(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    bool_t multiResolution;
    dash::mpd::IRepresentation* representation = aAdaptationSet->GetRepresentation().at(0);
    Error::Enum result =
        DashOmafAttributes::getOMAFQualityRankingType(representation->GetAdditionalSubNodes(), multiResolution);
    if (result == Error::ITEM_NOT_FOUND)
    {
        result =
            DashOmafAttributes::getOMAFQualityRankingType(aAdaptationSet->GetAdditionalSubNodes(), multiResolution);
    }

    if (result != Error::OK)
    {
        return false;
    }
    return multiResolution;
}


AdaptationSetType::Enum DashAdaptationSetExtractor::getType() const
{
    return AdaptationSetType::EXTRACTOR;
}

Error::Enum DashAdaptationSetExtractor::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
{
    // In OMAF, Preselection links adaptation sets. Not relevant with the representations
    Error::Enum result = doInitialize(aDashComponents, aInitializationSegmentId);
    DashOmafAttributes::getOMAFPreselection(aDashComponents.adaptationSet, mAdaptationSetId, mSupportingSetIds);
    return result;
}

Error::Enum DashAdaptationSetExtractor::doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
{
    return DashAdaptationSet::doInitialize(aDashComponents, aInitializationSegmentId);
}

DashRepresentation* DashAdaptationSetExtractor::createRepresentation(DashComponents aDashComponents,
                                                                     uint32_t aInitializationSegmentId,
                                                                     uint32_t aBandwidth)
{
    DashRepresentationExtractor* newrepresentation = OMAF_NEW(mMemoryAllocator, DashRepresentationExtractor);
    if (newrepresentation->initialize(aDashComponents, aBandwidth, mContent, aInitializationSegmentId, this, this) !=
        Error::OK)
    {
        OMAF_LOG_W("Non-supported representation skipped");
        OMAF_DELETE(mMemoryAllocator, newrepresentation);
        return OMAF_NULL;
    }
    aInitializationSegmentId++;

    if (mCoveredViewport == OMAF_NULL)
    {
        // extractor is always video
        parseVideoViewport(aDashComponents);
    }

    // set cache mode: auto
    newrepresentation->setCacheFillMode(true);
    // set segment merging on if supported; extractor track is not causing big overhead (if any, depending on the mode)
    // even if we download 10 at a time
    newrepresentation->setLatencyReq(LatencyRequirement::NOT_LATENCY_CRITICAL);

    return newrepresentation;
}

uint32_t DashAdaptationSetExtractor::getCurrentBandwidth() const
{
    if (mCurrentRepresentation != OMAF_NULL)
    {
        uint32_t bitrate = mCurrentRepresentation->getBitrate();
        if (mContent.matches(MediaContent::Type::VIDEO_EXTRACTOR) && !mSupportingSets.isEmpty())
        {
            for (SupportingAdaptationSets::ConstIterator it = mSupportingSets.begin(); it != mSupportingSets.end();
                 ++it)
            {
                bitrate += (*it)->getCurrentBandwidth();
            }
        }
        return bitrate;
    }
    else
    {
        return 0;
    }
}

Error::Enum DashAdaptationSetExtractor::startDownload(time_t startTime)
{
    mTargetNextSegmentId = mNextSegmentToBeConcatenated;
    Error::Enum result = DashAdaptationSet::startDownload(startTime);

    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        if (!(*it)->isActive())
        {
            OMAF_LOG_V("start downloading supporting set %d for extractor set %d", (*it)->getId(), mAdaptationSetId);
            (*it)->startDownloadFromSegment(mTargetNextSegmentId, mNextSegmentToBeConcatenated, mBufferingTimeMs);
        }
    }
    return result;
}

Error::Enum DashAdaptationSetExtractor::startDownload(time_t startTime, uint32_t aBufferingTimeMs)
{
    OMAF_LOG_V("startDownload with expected round-trip delay %d", aBufferingTimeMs);
    mBufferingTimeMs = aBufferingTimeMs;
    mCurrentRepresentation->setBufferingTime(aBufferingTimeMs);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->setBufferingTime(mBufferingTimeMs);
    }
    mTargetNextSegmentId = mNextSegmentToBeConcatenated;
    return DashAdaptationSet::startDownload(startTime, aBufferingTimeMs);
}

Error::Enum DashAdaptationSetExtractor::startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId,
                                                                 uint32_t aNextToBeProcessedSegmentId,
                                                                 uint32_t aBufferingTimeMs)
{
    mBufferingTimeMs = aBufferingTimeMs;
    mCurrentRepresentation->setBufferingTime(aBufferingTimeMs);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->setBufferingTime(mBufferingTimeMs);
    }
    mTargetNextSegmentId = aTargetDownloadSegmentId;
    if (aNextToBeProcessedSegmentId != INVALID_SEGMENT_INDEX)
    {
        mNextSegmentToBeConcatenated = aNextToBeProcessedSegmentId;
    }
    return DashAdaptationSet::startDownloadFromSegment(aTargetDownloadSegmentId, aNextToBeProcessedSegmentId,
                                                       aBufferingTimeMs);
}

Error::Enum DashAdaptationSetExtractor::startDownloadFromTimestamp(uint32_t& aTargetTimeMs, uint32_t aBufferingTimeMs)
{
    OMAF_LOG_V("startDownload with expected round-trip delay %d", aBufferingTimeMs);
    mBufferingTimeMs = aBufferingTimeMs;
    mCurrentRepresentation->setBufferingTime(aBufferingTimeMs);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->setBufferingTime(mBufferingTimeMs);
    }
    Error::Enum result = DashAdaptationSet::startDownloadFromTimestamp(aTargetTimeMs, aBufferingTimeMs);
    // take the first segment to download as the target for concatenation
    mTargetNextSegmentId = mNextSegmentToBeConcatenated = mCurrentRepresentation->getFirstSegmentId();
    return result;
}

Error::Enum DashAdaptationSetExtractor::stopDownload()
{
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            (*it)->stopDownload();

            // to avoid gaps in segment sequence after pause + resume (could have cached one representation over the
            // pause, but then resumes a different one => can create tricky issues), it is better to clean up old
            // segments and also reset download counter
            (*it)->cleanUpOldSegments(INVALID_SEGMENT_INDEX);
        }
    }
    return DashAdaptationSet::stopDownload();
}

Error::Enum DashAdaptationSetExtractor::stopDownloadAsync(bool_t aAbort, bool_t aReset)
{
    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            (*it)->stopDownloadAsync(aAbort, aReset);
        }
    }
    return DashAdaptationSet::stopDownloadAsync(aAbort, aReset);
}

void_t DashAdaptationSetExtractor::clearDownloadedContent()
{
    DashAdaptationSet::clearDownloadedContent();
    // clear the segments waiting in RepresentationExtractor
    for (DashRepresentation* extractor : mRepresentations)
    {
        ((DashRepresentationExtractor*) extractor)->clearDownloadedContent();
    }

    if (mSupportingSets.getSize() > 0)
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            ((DashAdaptationSetTile*) (*it))->clearDownloadedContent();
        }
    }
}

bool_t DashAdaptationSetExtractor::isBuffering()
{
    if (!mCurrentRepresentation->readyForSegment(mNextSegmentToBeConcatenated))
    {
        // we don't need the next segment yet
        return false;
    }
    bool_t isBuffering = mCurrentRepresentation->isBuffering();
    if (!isBuffering && !mSupportingSets.isEmpty())
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if (mNrSegmentsProcessed == 0)
            {
                if ((*it)->isBuffering())
                {
                    OMAF_LOG_V("%s is doing prebuffering", (*it)->getCurrentRepresentation()->getId());
                    return true;
                }
            }
            else if ((*it)->isBuffering(mNextSegmentToBeConcatenated))
            {
                OMAF_LOG_V("%s is buffering, next segment to be concat %d", (*it)->getCurrentRepresentation()->getId(),
                           mNextSegmentToBeConcatenated);
                return true;
            }
        }
    }
    else
    {
        OMAF_LOG_V("%s is buffering", getCurrentRepresentationId().getData());
    }
    return isBuffering;
}

bool_t DashAdaptationSetExtractor::isEndOfStream()
{
    bool_t isEndOfStream = mCurrentRepresentation->isEndOfStream();
    if (!isEndOfStream && !mSupportingSets.isEmpty())
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if ((*it)->isEndOfStream())
            {
                OMAF_LOG_V("%s is end of stream", (*it)->getCurrentRepresentation()->getId());
                return true;
            }
        }
    }
    return isEndOfStream;
}

bool_t DashAdaptationSetExtractor::isError()
{
    bool_t isError = mCurrentRepresentation->isError();
    if (!isError && !mSupportingSets.isEmpty())
    {
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if ((*it)->isError())
            {
                OMAF_LOG_V("%s is error", (*it)->getCurrentRepresentation()->getId());
                return true;
            }
        }
    }
    return isError;
}

void_t DashAdaptationSetExtractor::setQualityLevels(uint8_t aFgQuality,
                                                    uint8_t aMarginQuality,
                                                    uint8_t aBgQuality,
                                                    uint8_t aNrQualityLevels)
{
    OMAF_LOG_V("setQualityLevels fg %d bg %d", aFgQuality, aBgQuality);

    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        // first check if there is a next representation, as current may be obsolete and we shouldn't compare against it
        DashRepresentation* current = (*it)->getNextRepresentation();
        if (current == OMAF_NULL)
        {
            // there is no next, so use the current one
            current = (*it)->getCurrentRepresentation();
        }
        TileRole::Enum role = (*it)->getRole();
        if (role == TileRole::FOREGROUND || role == TileRole::FOREGROUND_POLE)
        {
           // currently supported
            if (current != (*it)->getRepresentationForQuality(aFgQuality, aNrQualityLevels))
            {
                OMAF_LOG_D("%d\tSet foreground quality of %s to %d", Time::getClockTimeMs(), current->getId(),
                           aFgQuality);
                (*it)->selectQuality(aFgQuality, aNrQualityLevels, mNextSegmentToBeConcatenated, role);
            }
            // else the level is ok
        }
        else if (role == TileRole::FOREGROUND_MARGIN)
        {
            if (current != (*it)->getRepresentationForQuality(aMarginQuality, aNrQualityLevels))
            {
                OMAF_LOG_D("%d\tSet margin quality of %s to %d", Time::getClockTimeMs(), current->getId(), aFgQuality);
                (*it)->selectQuality(aFgQuality, aNrQualityLevels, mNextSegmentToBeConcatenated, role);
            }
            // else the level is ok
        }
        else
        {
            if (current != (*it)->getRepresentationForQuality(aBgQuality, aNrQualityLevels))
            {
                OMAF_LOG_D("Set background quality to %d", current->getId(), aBgQuality);
                (*it)->selectQuality(aBgQuality, aNrQualityLevels, mNextSegmentToBeConcatenated, role);
            }
            // else the level is ok
        }
    }
}

void_t DashAdaptationSetExtractor::setBufferingTime(uint32_t aBufferingTimeMs)
{
    mBufferingTimeMs = aBufferingTimeMs;
    DashAdaptationSet::setBufferingTime(aBufferingTimeMs);
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        (*it)->setBufferingTime(aBufferingTimeMs);
    }
}

Error::Enum DashAdaptationSetExtractor::seekToMs(uint64_t& aSeekToTargetMs, uint64_t& aSeekToResultMs)
{
    uint32_t seekSegmentIndex;
    Error::Enum result = mCurrentRepresentation->seekToMs(aSeekToTargetMs, aSeekToResultMs, seekSegmentIndex);
    mNextSegmentToBeConcatenated = mTargetNextSegmentId = seekSegmentIndex;

    // other representations should get the indices right when doing quality / ABR switch
    if (mSupportingSets.getSize() > 0)
    {
        // this is an OMAF extractor track
        uint64_t resultMs;
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            (*it)->seekToMs(aSeekToTargetMs, resultMs);
        }
    }
    return result;
}

bool_t DashAdaptationSetExtractor::processSegmentDownload()
{
    mCurrentRepresentation->processSegmentDownload();

    if (mSupportingSets.getSize() > 0)
    {
        // this is an OMAF extractor track
        uint32_t nextSegmentId = peekNextSegmentId();
        if (nextSegmentId == INVALID_SEGMENT_INDEX)
        {
            return false;
        }
        if (getIsInitialized() &&
            mCurrentRepresentation
                ->isDownloading())  // must not restart if stopped due to viewpoint switch, just keep concatenating
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if ((*it)->isActive())
                {
                    if ((*it)->getLastSegmentId(false) <= mHighestSegmentIdtoDownload)
                    {
                        (*it)->processSegmentDownload();
                        if ((*it)->getLastSegmentId(false) == mHighestSegmentIdtoDownload)
                        {
                            mCurrFlagsTileDownload |= (*it)->getTileFlag();
                        }
                    }
                    else
                    {
                        mCurrFlagsTileDownload |= (*it)->getTileFlag();
                    }
                    if (mCurrFlagsTileDownload == mMaskTileDownload)
                    {
                        mHighestSegmentIdtoDownload++;
                        OMAF_LOG_V("New Segment to download %d ", mHighestSegmentIdtoDownload);
                        mCurrFlagsTileDownload = 0;
                    }
                }
                else
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
                        (*it)->startDownload(mDownloadStartTime);
                    }
                }
            }
        }
    }
    // this class supports only extractors with single representation
    return false;
}

void_t DashAdaptationSetExtractor::concatenateIfReady()
{
    uint32_t nextSegmentId = peekNextSegmentId();
    if (nextSegmentId == INVALID_SEGMENT_INDEX)
    {
        return;
    }

    if (mCurrentRepresentation->readyForSegment(nextSegmentId))
    {
        concatenateAndParseSegments();
    }
    else
    {
        // OMAF_LOG_V("skip concatenate %d", nextSegmentId);
    }
}

bool_t DashAdaptationSetExtractor::isABRSwitchOngoing() const
{
    for (SupportingAdaptationSets::ConstIterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        if ((*it)->isABRSwitchOngoing())
        {
            return true;
        }
    }
    return false;
}

void_t DashAdaptationSetExtractor::handleSpeedFactor(float32_t aSpeedFactor)
{
    // don't store it
}


void_t DashAdaptationSetExtractor::parseVideoViewport(DashComponents& aNextComponents)
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
            if (mCoveredViewport == OMAF_NULL)
            {
                mCoveredViewport = OMAF_NEW(mMemoryAllocator, VASTileViewport)();
            }
            mCoveredViewport->set(coverage.azimuthCenter, coverage.elevationCenter, coverage.azimuthRange,
                                  coverage.elevationRange, VASLongitudeDirection::COUNTER_CLOCKWISE);
        }
        else if (mSourceType == SourceType::CUBEMAP)
        {
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

const AdaptationSetBundleIds& DashAdaptationSetExtractor::getSupportingSets() const
{
    return mSupportingSetIds;
}

void_t DashAdaptationSetExtractor::addSupportingSet(DashAdaptationSetTile* aSupportingSet)
{
    OMAF_ASSERT(!mSupportingSets.contains(aSupportingSet), "Adding the same supporting set again");
    // each tile is assigned a unique bit. The bits are then masked to see if all tiles have reached a certain segment
    // index.
    uint64_t flag = 1ULL << mSupportingSets.getSize();
    aSupportingSet->addTileFlag(flag);
    mMaskTileDownload |= flag;
    OMAF_LOG_I("addSupportingSet %d for %d %d %x %x:%x", aSupportingSet->getId(), mAdaptationSetId, flag, flag,
               mMaskTileDownload, mMaskTileDownload);

    mSupportingSets.add(aSupportingSet);
    // OMAF_LOG_V("addSupportingSet %d for %d", aSupportingSet->getId(), mAdaptationSetId);
}

uint32_t DashAdaptationSetExtractor::getNextProcessedSegmentId() const
{
    return mNextSegmentToBeConcatenated;
}

size_t DashAdaptationSetExtractor::getNrForegroundTiles()
{
    OMAF_ASSERT(false, "Not supported in this class");
    return 0;
}

void_t DashAdaptationSetExtractor::setVideoStreamId(streamid_t aStreamId)
{
    // Video stream id plays a special role with extractors: in multi-res all extractors must share it within a
    // viewpoint. But it must change between viewpoints.
    OMAF_ASSERT(!mRepresentations.isEmpty(), "setVideoStreamId must be called only after init");
    for (DashRepresentation* representation : mRepresentations)
    {
        representation->setVideoStreamId(aStreamId);
    }
}

streamid_t DashAdaptationSetExtractor::getVideoStreamId() const
{
    return mCurrentRepresentation->getVideoStreamId();
}

void_t DashAdaptationSetExtractor::concatenateAndParseSegments()
{
    // check if we have segments to concatenate
    DashSegment* segment = mCurrentRepresentation->peekSegment();
    if (segment != OMAF_NULL)
    {
        if (segment->getSegmentId() < mNextSegmentToBeConcatenated)
        {
            OMAF_LOG_V("Too old segment!");
            mCurrentRepresentation->cleanUpOldSegments(mNextSegmentToBeConcatenated);
            segment = mCurrentRepresentation->peekSegment();
            if (segment == OMAF_NULL)
            {
                return;
            }
        }
        OMAF_LOG_V("concatenateAndParseSegments %d, repr %s", segment->getSegmentId(), mCurrentRepresentation->getId());
        size_t totalSize = segment->getDataBuffer()->getSize();
        uint32_t segmentId = segment->getSegmentId();
        bool_t available = true;
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            (*it)->trySwitchingRepresentation(segmentId);

            size_t segmentSize = 0;
            if ((*it)->hasSegment(segmentId, segmentSize))
            {
                totalSize += segmentSize;
            }
            else
            {
                available = false;
            }
        }
        if (available)
        {
            // concatenate and feed to the mp4 parser. Note: the ownership of the segment is given to the parser who
            // frees the memory
            DashSegment* concatenatedSegment = OMAF_NEW_HEAP(DashSegment)(
                totalSize, segment->getSegmentId(), segment->getInitSegmentId(), false, OMAF_NULL);
            segment = mCurrentRepresentation->getSegment();
            uint8_t* ptr = concatenatedSegment->getDataBuffer()->getDataPtr();
            size_t index = 0;
            memcpy(ptr, segment->getDataBuffer()->getDataPtr(), segment->getDataBuffer()->getSize());
            index += segment->getDataBuffer()->getSize();
            concatenatedSegment->addSrcSegment(segment);
#if OMAF_ABR_LOGGING
            // >> TEST code for analysing VD performance
            char segmentLog[1000];
            sprintf(segmentLog, "Tile/Quality - ");
            // << TEST code for analysing VD performance
#endif
            OMAF_LOG_V("Concatenate segments %d", segmentId);
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                uint8_t qualityLevel = 0;
                segment = (*it)->getSegment(segmentId, qualityLevel);
                if (segment == OMAF_NULL)
                {
                    OMAF_LOG_W("Segment %d missing from %s although just checked", segmentId,
                               (*it)->getCurrentRepresentationId().getData());
                    OMAF_DELETE_HEAP(concatenatedSegment);
                    OMAF_ASSERT(false, "NULL segment found even after checking");
                    break;
                }
                // id was already checked above
                memcpy(ptr + index, segment->getDataBuffer()->getDataPtr(), segment->getDataBuffer()->getSize());
                index += segment->getDataBuffer()->getSize();
                concatenatedSegment->addSrcSegment(segment);
#if OMAF_ABR_LOGGING
                // >> TEST code for analysing VD performance

                {
                    char level[12];
                    sprintf(level, "%d/%d ", (*it)->getId(), qualityLevel);
                    strcat(segmentLog, level);
                }
                // << TEST code for analysing VD performance
#endif
            }
            concatenatedSegment->getDataBuffer()->setSize(index);
            mNextSegmentToBeConcatenated = segmentId + 1;
            concatenatedSegment->setTimestampBase(segment->getTimestampBase());
            OMAF_LOG_V("concatenated %zd bytes", index);
            mCurrentRepresentation->parseConcatenatedMediaSegment(concatenatedSegment);
            mNrSegmentsProcessed++;
#if OMAF_ABR_LOGGING
            // >> TEST code for analysing VD performance
            // This way of finding out the first timestamp of the segment relies on the criteria (in
            // MP4VRParser::readyForSegment) where we concatenate a new segment only when the previous one is almost
            // empty.
            uint64_t pts = 0;
            mCurrentRepresentation->getCurrentVideoStreams().at(0)->framesUntilNextSyncFrame(pts);
            char segmentLogFull[1100];
            sprintf(segmentLogFull, "CONCATENATE at %lld %s", pts, segmentLog);
            OMAF_LOG_ABR(segmentLogFull);
            // << TEST code for analysing VD performance
#endif
        }
    }
}

bool_t DashAdaptationSetExtractor::calculateBitrates(const VASTileSelection& aForegroundTiles, uint8_t aNrQualityLevels)
{
    if (mSupportingSets.isEmpty())
    {
        return false;
    }
    // Calculate the bitrates in increasing order
    // 1) take N foreground tiles in background quality + rest background tiles and sum the bitrates
    // 2) take N foreground tiles in background + step quality + rest background tiles and sum the bitrates
    // 3) loop until foreground-step == 1, which represents the highest quality
    // could try also increasing background quality until it is foreground -1?
    // in preselection cases, select new tile reps (quality levels)

    for (uint8_t qualityFg = aNrQualityLevels, qualityBg = aNrQualityLevels; qualityFg > 0; qualityFg--)
    {
        mBitrates.add(calculateBitrate(aForegroundTiles, aNrQualityLevels, qualityFg, qualityBg));
        OMAF_LOG_V("Added bitrate: %d", mBitrates.at(mBitrates.getSize() - 1));
    }
    return true;
}

uint32_t DashAdaptationSetExtractor::calculateBitrate(const VASTileSelection& aForegroundTiles,
                                                      uint8_t aNrQualityLevels,
                                                      uint8_t aQualityFg,
                                                      uint8_t aQualityBg)
{
    uint32_t bitrate = getRepresentationForQuality(1, aNrQualityLevels)->getBitrate();
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        bool_t inFg = false;
        for (VASTileSelection::ConstIterator fg = aForegroundTiles.begin(); fg != aForegroundTiles.end(); ++fg)
        {
            if ((*fg)->getAdaptationSet() == (*it))
            {
                bitrate += (*it)->getRepresentationForQuality(aQualityFg, aNrQualityLevels)->getBitrate();
                inFg = true;
                break;
            }
        }
        if (!inFg)
        {
            bitrate += (*it)->getRepresentationForQuality(aQualityBg, aNrQualityLevels)->getBitrate();
        }
    }
    // scale up by a factor that include estimated viewport switching overhead
    return (uint32_t)(bitrate * BITRATE_SAFETY_MARGIN);
}


// need to have a dedicated variant since extractor can be ahead of tiles in downloading
uint32_t DashAdaptationSetExtractor::getTimestampMsOfLastDownloadedFrame()
{
    uint32_t lastSegmentId = 0;
#if 0
        getReadPositionUs(lastSegmentId);
#else
    if (mNextRepresentation)
    {
        lastSegmentId = mNextRepresentation->getLastSegmentId(false);
    }
    uint32_t id = mCurrentRepresentation->getLastSegmentId(false);

    if (id > lastSegmentId)
    {
        lastSegmentId = id;
    }
    for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
    {
        uint32_t tileId = (*it)->getLastSegmentId(true);
        if (tileId < lastSegmentId)
        {
            lastSegmentId = tileId;
        }
    }
#endif
    uint64_t durationMs = 0;
    mCurrentRepresentation->isSegmentDurationFixed(durationMs);

    OMAF_LOG_V("getTimestampMsOfLastDownloadedFrame, last segment %d segment duration %d", lastSegmentId, durationMs);
    // segmentId is 1-based, here we use the count
    return (uint32_t)((lastSegmentId - 1) * durationMs);
}

void_t DashAdaptationSetExtractor::onTargetSegmentLocated(uint32_t aSegmentId, uint32_t aSegmentTimeMs)
{
    mTargetNextSegmentId = mNextSegmentToBeConcatenated = aSegmentId;
}

void_t DashAdaptationSetExtractor::onNewStreamsCreated(MediaContent& aContent)
{
    mObserver.onNewStreamsCreated(aContent);
}

OMAF_NS_END
