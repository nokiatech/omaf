
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
#include <IPeriod.h>
#include "DashProvider/NVRDashViewpointInfo.h"
#include "Foundation/NVRFixedArray.h"
#include "NVREssentials.h"


OMAF_NS_BEGIN

typedef FixedArray<dash::mpd::IAdaptationSet*, 256> MPDAdaptationSets;

/*
 * Wrapper class for the dash::mpd::IPeriod, used to be able to filter the adaptation sets by the viewpoint
 */
class DashPeriodViewpoint
{
public:
    DashPeriodViewpoint(dash::mpd::IPeriod& aPeriod);
    DashPeriodViewpoint(dash::mpd::IPeriod& aPeriod, MpdViewpointInfo& aViewpoint);
    virtual ~DashPeriodViewpoint();
    void_t addAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet);
    bool_t removeAdaptationSet(dash::mpd::IAdaptationSet* aAdaptationSet);
    uint32_t getViewpointId() const;
    const MpdViewpointInfo& getViewpoint() const;

public:  // from IPeriod
    virtual const std::vector<dash::mpd::IBaseUrl*>& GetBaseURLs() const;
    virtual dash::mpd::ISegmentBase* GetSegmentBase() const;
    virtual dash::mpd::ISegmentList* GetSegmentList() const;
    virtual dash::mpd::ISegmentTemplate* GetSegmentTemplate() const;
    virtual const MPDAdaptationSets& GetAdaptationSets() const;
    virtual const std::vector<dash::mpd::ISubset*>& GetSubsets() const;
    virtual const std::string& GetXlinkHref() const;
    virtual const std::string& GetXlinkActuate() const;
    virtual const std::string& GetId() const;
    virtual const std::string& GetStart() const;
    virtual const std::string& GetDuration() const;
    virtual bool GetBitstreamSwitching() const;

private:
    dash::mpd::IPeriod& mPeriod;
    MPDAdaptationSets mAdaptationSets;

    const MpdViewpointInfo mViewpoint;
};

OMAF_NS_END
