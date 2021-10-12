
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
class DashAdaptationSetViewportDep : public DashAdaptationSet
{
public:
    virtual ~DashAdaptationSetViewportDep();

public:  // new
    virtual uint32_t estimateSegmentIdForSwitch(uint32_t aNextSegmentToProcess, uint32_t aNextSegmentNotDownloaded);
    virtual bool_t readyToSwitch(DashRepresentation* aRepresentation, uint32_t aNextNeededSegment);

protected:
    DashAdaptationSetViewportDep(DashAdaptationSetObserver& observer);
};
OMAF_NS_END
