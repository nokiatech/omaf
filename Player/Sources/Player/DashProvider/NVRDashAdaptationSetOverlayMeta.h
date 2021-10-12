
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
#pragma once

#include "DashProvider/NVRDashAdaptationSet.h"


OMAF_NS_BEGIN

// this class is not meant to be instantiated, it is just a base class for viewport dependent adaptation sets
class DashAdaptationSetOverlayMeta : public DashAdaptationSet
{
public:
    DashAdaptationSetOverlayMeta(DashAdaptationSetObserver& observer);
    virtual ~DashAdaptationSetOverlayMeta();

    virtual Error::Enum initialize(DashComponents aDashComponents,
                                   uint32_t& aInitializationSegmentId,
                                   MediaContent::Type aType);
    virtual AdaptationSetType::Enum getType() const;

    virtual void_t onNewStreamsCreated(MediaContent& aContent);

protected:
    virtual DashRepresentation* createRepresentation(DashComponents aDashComponents,
                                                     uint32_t aInitializationSegmentId,
                                                     uint32_t aBandwidth);

public:  // new
    static bool_t isOverlayMetadata(dash::mpd::IAdaptationSet* aAdaptationSet);
    static bool_t isOverlayRvpoMetadata(dash::mpd::IAdaptationSet* aAdaptationSet);
};
OMAF_NS_END
