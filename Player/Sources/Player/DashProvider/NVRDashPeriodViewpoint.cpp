
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
#include "NVRDashPeriodViewpoint.h"


OMAF_NS_BEGIN

DashPeriodViewpoint::DashPeriodViewpoint(dash::mpd::IPeriod& aPeriod)
    : mPeriod(aPeriod)
    , mViewpoint()
{
}

DashPeriodViewpoint::DashPeriodViewpoint(dash::mpd::IPeriod& aPeriod, MpdViewpointInfo& aViewpoint)
    : mPeriod(aPeriod)
    , mViewpoint(aViewpoint)
{
}


DashPeriodViewpoint::~DashPeriodViewpoint()
{
}

void_t DashPeriodViewpoint::addAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    mAdaptationSets.add(aAdaptationSet);
}

bool_t DashPeriodViewpoint::removeAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet)
{
    return mAdaptationSets.remove(aAdaptationSet);
}

uint32_t DashPeriodViewpoint::getViewpointId() const
{
    return atoi(mViewpoint.id);
}

const MpdViewpointInfo& DashPeriodViewpoint::getViewpoint() const
{
    return mViewpoint;
}


const std::vector<dash::mpd::IBaseUrl*>& DashPeriodViewpoint::GetBaseURLs() const
{
    return mPeriod.GetBaseURLs();
}
dash::mpd::ISegmentBase* DashPeriodViewpoint::GetSegmentBase() const
{
    return mPeriod.GetSegmentBase();
}
dash::mpd::ISegmentList* DashPeriodViewpoint::GetSegmentList() const
{
    return mPeriod.GetSegmentList();
}
dash::mpd::ISegmentTemplate* DashPeriodViewpoint::GetSegmentTemplate() const
{
    return mPeriod.GetSegmentTemplate();
}
const MPDAdaptationSets& DashPeriodViewpoint::GetAdaptationSets() const
{
    return mAdaptationSets;
}
const std::vector<dash::mpd::ISubset*>& DashPeriodViewpoint::GetSubsets() const
{
    return mPeriod.GetSubsets();
}
const std::string& DashPeriodViewpoint::GetXlinkHref() const
{
    return mPeriod.GetXlinkHref();
}
const std::string& DashPeriodViewpoint::GetXlinkActuate() const
{
    return mPeriod.GetXlinkActuate();
}
const std::string& DashPeriodViewpoint::GetId() const
{
    return mPeriod.GetId();
}
const std::string& DashPeriodViewpoint::GetStart() const
{
    return mPeriod.GetStart();
}
const std::string& DashPeriodViewpoint::GetDuration() const
{
    return mPeriod.GetDuration();
}
bool DashPeriodViewpoint::GetBitstreamSwitching() const
{
    return mPeriod.GetBitstreamSwitching();
}

OMAF_NS_END
