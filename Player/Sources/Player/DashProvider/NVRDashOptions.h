
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

#include "NVRNamespace.h"

OMAF_NS_BEGIN

namespace LatencyRequirement
{
    enum Enum
    {
        INVALID = -1,
        LOW_LATENCY,
        MEDIUM_LATENCY,
        NOT_LATENCY_CRITICAL,
        COUNT
    };
}


OMAF_NS_END