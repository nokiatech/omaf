
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
#include "DashProvider/NVRDashAdaptationSetExtractorDepId.h"
#include "VideoDecoder/NVRVideoDecoderManager.h"
#include "DashProvider/NVRMPDAttributes.h"
#include "DashProvider/NVRMPDExtension.h"
#include "Foundation/NVRLogger.h"
#include "Media/NVRMediaType.h"
#include "Foundation/NVRTime.h"
#include "DashProvider/NVRDashRepresentationExtractor.h"
#include "DashProvider/NVRDashRepresentationTile.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(DashAdaptationSetExtractorDepId)

    DashAdaptationSetExtractorDepId::DashAdaptationSetExtractorDepId(DashAdaptationSetObserver& observer)
    : DashAdaptationSetExtractor(observer)
    {

    }

    DashAdaptationSetExtractorDepId::~DashAdaptationSetExtractorDepId()
    {
    }

    
    AdaptationSetType::Enum DashAdaptationSetExtractorDepId::getType() const
    {
        return AdaptationSetType::EXTRACTOR;
    }

    Error::Enum DashAdaptationSetExtractorDepId::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
    {
        DashOmafAttributes::getOMAFRepresentationDependencies(aDashComponents, mDependingRepresentationIds);
        return doInitialize(aDashComponents, aInitializationSegmentId);
    }

    Error::Enum DashAdaptationSetExtractorDepId::doInitialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
    {
        //TODO anything special here? At least select the mCurrentRepresentation
        return DashAdaptationSet::doInitialize(aDashComponents, aInitializationSegmentId);
    }

    DashRepresentation* DashAdaptationSetExtractorDepId::createRepresentation(DashComponents aDashComponents, uint32_t aInitializationSegmentId, uint32_t aBandwidth)
    {
        DashRepresentationExtractor* newrepresentation = (DashRepresentationExtractor*)DashAdaptationSetExtractor::createRepresentation(aDashComponents, aInitializationSegmentId, aBandwidth);

        if (newrepresentation != OMAF_NULL)
        {
            // if extractor track has a coverage definition, assign it to representation level. This is valid at least with @dependencyId case, TODO if also other cases
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
    const FixedArray<VASTileViewport*, 32> DashAdaptationSetExtractorDepId::getCoveredViewports() const
    {
        OMAF_ASSERT(mCoveredViewport == OMAF_NULL, "Wrong variant of getCoveredViewport used!");
        FixedArray<VASTileViewport*, 32> viewports;
        if (mContent.matches(MediaContent::Type::VIDEO_EXTRACTOR))
        {
            for (Representations::ConstIterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
            {
                viewports.add(((DashRepresentationExtractor*)(*it))->getCoveredViewport());
            }
        }
        return viewports;
    }


    bool_t DashAdaptationSetExtractorDepId::processSegmentDownload()
    {
        // operations for mCurrentRepresentation done in the extractor base class - returns always false as no representation switch takes place there
        DashAdaptationSetExtractor::processSegmentDownload();

        if (mNextRepresentation != OMAF_NULL)
        {
            //technically the new stream/representation/decoder will get activated HERE!
            //(stream activate gets called when init segment is processed)
            mNextRepresentation->processSegmentDownload();
        }

        // For ABR, check if there's a switch pending and if current has any more packets left for video and if not, switch
        if (mNextRepresentation != OMAF_NULL && mCurrentRepresentation != mNextRepresentation)
        {
            bool_t doTheSwitch = false;
//            if (mContent.matches(MediaContent::Type::VIDEO_EXTRACTOR))
            {
                if (!mNextRepresentation->isBuffering())
                {
                    DashSegment* segment = mNextRepresentation->peekSegment();
                    if (segment != OMAF_NULL)
                    {
                        if (segment->getSegmentId() == mNextSegmentToBeConcatenated)
                        {
                            doTheSwitch = true;
                        }
                    }
                }
            }
            if (doTheSwitch)
            {
                doSwitchRepresentation();
                return true;
            }
        }
        return false;
    }

    const RepresentationDependencies& DashAdaptationSetExtractorDepId::getDependingRepresentations() const
    {
        return mDependingRepresentationIds;
    }

    bool_t DashAdaptationSetExtractorDepId::selectRepresentation(const VASTileViewport* aTile, uint32_t aNextNeededSegment)
    {
        OMAF_ASSERT(mContent.matches(MediaContent::Type::VIDEO_EXTRACTOR), "Called selectRepresentation for other than extractor adaptation set!");
        for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
        {
            //TODO verify type casting? Should we do some adaptationset level class-variation or something?
            if (((DashRepresentationExtractor*)(*it))->getCoveredViewport() == aTile)
            {
                if (*it == mCurrentRepresentation)
                {
                    return false;
                }
                mNextRepresentation = *it;
                OMAF_LOG_V("selectRepresentation for extractor, found: %s", mNextRepresentation->getId());
                // find the dependencies for this one
                for (RepresentationDependencies::Iterator dRepr = mDependingRepresentationIds.begin(); dRepr != mDependingRepresentationIds.end(); ++dRepr)
                {
                    if ((*dRepr).ownRepresentationId == mNextRepresentation->getId())
                    {
                        // now we have the dependency list for the mNextRepresentation. Switch the representations of all the adaptation sets.
                        for (SupportingAdaptationSets::Iterator supp = mSupportingSets.begin(); supp != mSupportingSets.end(); ++supp)
                        {
                            (*supp)->selectRepresentation((*dRepr).dependendableRepresentations, aNextNeededSegment);
                        }
                        return prepareForSwitch(aNextNeededSegment, false);
                    }
                }
            }
        }
        return false;
    }

    void_t DashAdaptationSetExtractorDepId::doSwitchRepresentation()
    {
        //deactivate old representations decoder..
        mCurrentRepresentation->clearDownloadedContent();
#ifdef OMAF_PLATFORM_ANDROID
        OMAF_LOG_V("%d Switch done from %d to %d", Time::getClockTimeMs(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getBitrate());
#else
        OMAF_LOG_V("%lld Switch done from %s (%d) to %s (%d)", Time::getClockTimeMs(), mCurrentRepresentation->getId(), mCurrentRepresentation->getBitrate(), mNextRepresentation->getId(), mNextRepresentation->getBitrate());
#endif
        mCurrentRepresentation = mNextRepresentation;
        mNextRepresentation = OMAF_NULL;
    }

    bool_t DashAdaptationSetExtractorDepId::parseVideoQuality(DashComponents& aNextComponents, uint8_t& aQualityLevel, bool_t& aGlobal, DashRepresentation* aLatestRepresentation)
    {
        Error::Enum result = DashOmafAttributes::getOMAFQualityRanking(aNextComponents.representation->GetAdditionalSubNodes(), *((DashRepresentationExtractor*)aLatestRepresentation)->getCoveredViewport(), aQualityLevel, aGlobal);

        if (result == Error::OK || result == Error::OK_NO_CHANGE)
        {
            return true;
        }
        return false;
    }


OMAF_NS_END
