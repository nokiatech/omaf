
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
#include "DashProvider/NVRDashAdaptationSetExtractorMR.h"
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "Foundation/NVRLogger.h"


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

    DashRepresentation* DashAdaptationSetExtractorMR::createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth)
    {
        DashRepresentationExtractor* newrepresentation = (DashRepresentationExtractor*)DashAdaptationSetExtractor::createRepresentation(aDashComponents, aInitializationSegmentId, aBandwidth);
        
        if (newrepresentation != OMAF_NULL)
        {
            // if extractor track has a coverage definition, assign it to representation level. 
            newrepresentation->setCoveredViewport(mCoveredViewport);
            // ownership was passed
            mCoveredViewport = OMAF_NULL;
            // if we have coverage in the extractor, the dl manager should ignore coverages in tile adaptation sets

            // dependencyId based extractors also are expected to have quality descriptor
            uint8_t qualityIndex = 0;
            bool_t qualityLevelsGlobal = false;//TODO is there any use for this?
            parseVideoQuality(aDashComponents, qualityIndex, qualityLevelsGlobal, newrepresentation);
            // extractor representations are not really ordered by quality, as they contain dependencies to both high and low quality tiles
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
            viewports.add(((DashRepresentationExtractor*)(*it))->getCoveredViewport());
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

    bool_t DashAdaptationSetExtractorMR::parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityLevel, bool_t& aGlobal, DashRepresentation* aLatestRepresentation)
    {
        Error::Enum result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(), *((DashRepresentationExtractor*)aLatestRepresentation)->getCoveredViewport(), aQualityLevel, aGlobal);

        if (result == Error::OK_NO_CHANGE)
        {
            result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.adaptationSet->GetAdditionalSubNodes(), *((DashRepresentationExtractor*)aLatestRepresentation)->getCoveredViewport(), aQualityLevel, aGlobal);
            if (result == Error::OK || result == Error::OK)
            {
                return true;
            }
        }
        return false;
    }

    Error::Enum DashAdaptationSetExtractorMR::startDownloadFromSegment(uint32_t& aTargetDownloadSegmentId, uint32_t aNextToBeProcessedSegmentId, uint32_t aExpectedPingTimeMs)
    {
        uint32_t targetSegmentId = aTargetDownloadSegmentId;
        Error::Enum result = DashAdaptationSetExtractor::startDownloadFromSegment(targetSegmentId, aNextToBeProcessedSegmentId, aExpectedPingTimeMs);
        startedDownloadFromSegment(aTargetDownloadSegmentId);

        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            if (!(*it)->isActive())
            {
                uint32_t supportingTargetSegmentId = aTargetDownloadSegmentId;
                OMAF_LOG_V("start downloading supporting set %d for extractor set %d", (*it)->getId(), mAdaptationSetId);
                (*it)->startDownloadFromSegment(supportingTargetSegmentId, aNextToBeProcessedSegmentId, aExpectedPingTimeMs);
            }
        }
        aTargetDownloadSegmentId = targetSegmentId;
        return result;
    }

    Error::Enum DashAdaptationSetExtractorMR::stopDownloadAsync(bool_t aReset, DashAdaptationSetExtractorMR* aNewAdaptationSet)
    {
        OMAF_LOG_V("stopDownloadAsync for %d", mAdaptationSetId);
        if (mSupportingSets.getSize() > 0)
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if (aNewAdaptationSet == OMAF_NULL || !aNewAdaptationSet->hasSupportingSet((*it)->getId()))
                {
                    OMAF_LOG_V("Stop supporting set %d", (*it)->getId());
                    (*it)->stopDownloadAsync(false, aReset);    // in multi-resolution case we don't want to abort, as that can lead to segment sync issues
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
                    else if (isActive())// must not re-start a stopped supporting set
                    {
                        OMAF_LOG_V("start downloading supporting set %d for extractor set %d", (*it)->getId(), mAdaptationSetId);
                        if (mTargetNextSegmentId > 0)
                        {
                            (*it)->startDownloadFromSegment(mTargetNextSegmentId, mNextSegmentToBeConcatenated, mExpectedPingTimeMs);
                        }
                        else
                        {
                            (*it)->startDownload(mDownloadStartTime, mExpectedPingTimeMs);
                        }
                    }
                }

                if (mIsInUse)
                {
                    if (mCurrentRepresentation->readyForSegment(mNextSegmentToBeConcatenated))
                    {
                        concatenateAndParseSegments();
                    }
                    else
                    {
                        //OMAF_LOG_V("skip concatenate %d", mNextSegmentToBeConcatenated);
                    }
                }
                else
                {
                    // do not consume segments yet
                    return false;
                }
            }
        }

        // TODO ABR or other reasons can change streams?
        return false;
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
        // there could be mNextSegmentToBeConcatenated-1 in downloading, or not even yet downloaded, so cleanup doesn't ensure the next id with 100%, hence the concatenation need to still check it
    }

    // should be called when we want to discontinue concatenating segments from this set
    void_t DashAdaptationSetExtractorMR::switchingToAnother()
    {
        mCurrentRepresentation->switchedToAnother();
        mIsInUse = false;
    }

    void_t DashAdaptationSetExtractorMR::switchedToAnother()
    {
        // need to clear downloaded content, otherwise some common (used) partial set may have last segment in parser and block downloading new segments
        // TODO could clear used content only?
        OMAF_LOG_V("switchedToAnother");
        mCurrentRepresentation->clearDownloadedContent();
        mIsInUse = false;
    }

    bool_t DashAdaptationSetExtractorMR::isEndOfStream()
    {
        if (mCurrentRepresentation->isEndOfStream())// && mCurrentRepresentation->isDone())
        {
            return true;
        }
        return false;
    }

    bool_t DashAdaptationSetExtractorMR::readyToSwitch(uint32_t aNextNeededSegment)
    {
        if (aNextNeededSegment < mTargetNextSegmentId)
        {
            //TODO this is to avoid cases where some set has segments until aNextNeededSegment-1 from previous time it was under preparation, and target was aNextNeededSegment+1; some cleanup of old segments could be useful but when? Now it is done only after swithing to this.
            return false;
        }
        if (getLastSegmentId() >= aNextNeededSegment)
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if ((*it)->getLastSegmentId() < aNextNeededSegment)
                {
                    OMAF_LOG_V("not ready to switch to %d due to %s", aNextNeededSegment, (*it)->getCurrentRepresentationId().getData());
                    return false;
                }
            }
            switchSegmentDownloaded(aNextNeededSegment);

            if (DashAdaptationSetViewportDep::readyToSwitch(mCurrentRepresentation, aNextNeededSegment))
            {
                for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
                {
                    if (!(*it)->readyToSwitch((*it)->getCurrentRepresentation(), aNextNeededSegment))
                    {
                        OMAF_LOG_V("not ready to switch to %d due to %s", aNextNeededSegment, (*it)->getCurrentRepresentationId().getData());
                        return false;
                    }
                }
                return true;
            }
        }
        OMAF_LOG_V("not ready to switch to %d due to extractor %s", aNextNeededSegment, getCurrentRepresentationId().getData());
        return false;
    }

    void_t DashAdaptationSetExtractorMR::updateProgressDuringSwitch(uint32_t aNextSegmentToBeProcessed)
    {
        // this is called to avoid cache getting full with old segments, that can happen at least if the extractor change is triggered many times before actually switching to it
        mCurrentRepresentation->cleanUpOldSegments(aNextSegmentToBeProcessed);
        for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
        {
            (*it)->cleanUpOldSegments(aNextSegmentToBeProcessed);
        }
    }

    bool_t DashAdaptationSetExtractorMR::segmentAvailable(uint32_t aSegmentId, DashAdaptationSetExtractorMR* aNewAdaptationSet)
    {
        if (mCurrentRepresentation->getLastSegmentId(true) < aSegmentId)
        {
            return false;
        }
        // checks adaptation sets that are not common with the new one, to see if they are running out of segments
        if (mSupportingSets.getSize() > 0)
        {
            for (SupportingAdaptationSets::Iterator it = mSupportingSets.begin(); it != mSupportingSets.end(); ++it)
            {
                if (!aNewAdaptationSet->hasSupportingSet((*it)->getId()))
                {
                    if ((*it)->getLastSegmentId(true) < aSegmentId) // include possible incomplete download
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    bool_t DashAdaptationSetExtractorMR::isDone(uint32_t& aSegment)
    {
        if (mCurrentRepresentation)
        {
            return ((DashRepresentationExtractor*)mCurrentRepresentation)->isDone(aSegment);
        }
        return true;    // if no representation, cannot be "undone"
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
                    OMAF_LOG_V("Cannot complete the current extractor since missing segment from %d (%s)", (*it)->getId(), (*it)->getCurrentRepresentationId().getData());
                    return false;
                }
            }
            return true;
        }
        return false;
    }

OMAF_NS_END
