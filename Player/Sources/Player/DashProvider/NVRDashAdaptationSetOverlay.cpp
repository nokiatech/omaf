
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
#include "DashProvider/NVRDashAdaptationSetOverlay.h"
#include "DashProvider/NVRDashRepresentationOverlay.h"
#include "Foundation/NVRLogger.h"

OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetOverlay)

DashAdaptationSetOverlay::DashAdaptationSetOverlay(DashAdaptationSetObserver& observer)
    : DashAdaptationSet(observer)
    , mAssociatedSet(OMAF_NULL)
{
}

DashAdaptationSetOverlay::~DashAdaptationSetOverlay()
{
}

bool_t DashAdaptationSetOverlay::isOverlay(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    return DashOmafAttributes::getOMAFOverlay(aAdaptationSet);
}

bool_t DashAdaptationSetOverlay::hasAssociation(dash::mpd::IAdaptationSet* aAdaptationSet,
                                                FixedArray<uint32_t, 64>& aBackgroundVideos)
{
    return DashOmafAttributes::getOMAFOverlayAssociations(aAdaptationSet, aBackgroundVideos);
}

Error::Enum DashAdaptationSetOverlay::initialize(DashComponents aDashComponents, uint32_t& aInitializationSegmentId)
{
    mContent.addType(MediaContent::Type::VIDEO_OVERLAY);

    return doInitialize(aDashComponents, aInitializationSegmentId);
}

bool_t DashAdaptationSetOverlay::parseVideoProperties(DashComponents& aNextComponents)
{
    if (!DashAdaptationSet::parseVideoProperties(aNextComponents))
    {
        return false;
    }

    // parse overlay-specific properties
    FixedArray<uint32_t, 64> backgroundVideos;
    return DashOmafAttributes::getOMAFOverlayAssociations(aNextComponents.adaptationSet, backgroundVideos);
}

AdaptationSetType::Enum DashAdaptationSetOverlay::getType() const
{
    return AdaptationSetType::OVERLAY;
}

bool_t DashAdaptationSetOverlay::createVideoSources(sourceid_t& firstSourceId)
{
    // create a source for each representation - will be actually created later, this is just storing basic parameters
    for (Representations::Iterator it = mRepresentations.begin(); it != mRepresentations.end(); ++it)
    {
        (*it)->createVideoSource(firstSourceId, mSourceType, mVideoChannel);
    }
    return true;
}

DashRepresentation* DashAdaptationSetOverlay::createRepresentation(DashComponents aDashComponents,
                                                                   uint32_t aInitializationSegmentId,
                                                                   uint32_t aBandwidth)
{
    DashRepresentationOverlay* newrepresentation = OMAF_NEW(mMemoryAllocator, DashRepresentationOverlay);
    if (newrepresentation->initialize(aDashComponents, aBandwidth, mContent, aInitializationSegmentId, this, this) !=
        Error::OK)
    {
        OMAF_LOG_W("Non-supported representation skipped");
        OMAF_DELETE(mMemoryAllocator, newrepresentation);
        return OMAF_NULL;
    }
    aInitializationSegmentId++;

    // set cache mode: auto
    newrepresentation->setCacheFillMode(true);

    return newrepresentation;
}

void_t DashAdaptationSetOverlay::onNewStreamsCreated(MediaContent& aContent)
{
    mObserver.onNewStreamsCreated(aContent);
}

void_t DashAdaptationSetOverlay::setAssociatedSet(DashAdaptationSet* aAdaptationSet)
{
    mAssociatedSet = aAdaptationSet;
}

DashAdaptationSet* DashAdaptationSetOverlay::getAssociatedSet() const
{
    return mAssociatedSet;
}

OMAF_NS_END
