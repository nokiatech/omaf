
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

#include "NVRDashAdaptationSet.h"


OMAF_NS_BEGIN

namespace UserActivation
{
    enum Enum
    {
        INVALID = -1,
        NOT_KNOWN,
        ALWAYS_ON,
        USER_ACTIVATEABLE,
        COUNT
    };
}

class DashAdaptationSetAudio : public DashAdaptationSet
{
public:
    DashAdaptationSetAudio(DashAdaptationSetObserver& observer);

    virtual ~DashAdaptationSetAudio();

    virtual AdaptationSetType::Enum getType() const;

public:  // new
    static bool_t isAudio(DashComponents aDashComponents);

    virtual UserActivation::Enum isUserActivateable();
    virtual void_t setUserActivateable(bool_t aIs);

    // DashRepresentationObserver
public:
    virtual void_t onNewStreamsCreated(MediaContent& aContent);

private:
    UserActivation::Enum mUserActivateable;
};
OMAF_NS_END
