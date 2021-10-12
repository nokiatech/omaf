
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

#include "DashProvider/NVRDashRepresentation.h"

OMAF_NS_BEGIN

class DashRepresentationOverlay : public DashRepresentation
{
public:
    DashRepresentationOverlay();
    virtual ~DashRepresentationOverlay();

    virtual void_t createVideoSource(sourceid_t& sourceId, SourceType::Enum sourceType, StereoRole::Enum channel);

protected:
    virtual void_t initializeVideoSource();

private:
    sourceid_t mSourceId;
    SourceType::Enum mSourceType;
    StereoRole::Enum mRole;
};
OMAF_NS_END
