
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
#include "DashProvider/NVRDashAdaptationSetOverlayMeta.h"
#include "DashProvider/NVRDashRepresentationOverlay.h"
#include "Foundation/NVRLogger.h"


OMAF_NS_BEGIN
OMAF_LOG_ZONE(DashAdaptationSetOverlayMeta)

DashAdaptationSetOverlayMeta::DashAdaptationSetOverlayMeta(DashAdaptationSetObserver& observer)
    : DashAdaptationSet(observer)
{
}

DashAdaptationSetOverlayMeta::~DashAdaptationSetOverlayMeta()
{
}

bool_t DashAdaptationSetOverlayMeta::isOverlayMetadata(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    for (size_t codecIndex = 0; codecIndex < aAdaptationSet->GetCodecs().size(); codecIndex++)
    {
        if (aAdaptationSet->GetCodecs().at(codecIndex).find("dyol") != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool_t DashAdaptationSetOverlayMeta::isOverlayRvpoMetadata(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    for (size_t codecIndex = 0; codecIndex < aAdaptationSet->GetCodecs().size(); codecIndex++)
    {
        if (aAdaptationSet->GetCodecs().at(codecIndex).find("invo") !=
            std::string::npos)
        {
            return true;
        }
    }
    return false;
}

Error::Enum DashAdaptationSetOverlayMeta::initialize(DashComponents aDashComponents,
                                                     uint32_t& aInitializationSegmentId,
                                                     MediaContent::Type aType)
{
    mContent.addType(aType);

    return doInitialize(aDashComponents, aInitializationSegmentId);
}

DashRepresentation* DashAdaptationSetOverlayMeta::createRepresentation(DashComponents aDashComponents,
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

AdaptationSetType::Enum DashAdaptationSetOverlayMeta::getType() const
{
    return AdaptationSetType::METADATA;
}


void_t DashAdaptationSetOverlayMeta::onNewStreamsCreated(MediaContent& aContent)
{
    mObserver.onNewStreamsCreated(aContent);
}

OMAF_NS_END
