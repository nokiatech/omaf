
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

#include "NVREssentials.h"

OMAF_NS_BEGIN

namespace DebugMode
{
    enum Enum
    {
        INVALID = -1,

        NONE = 0,

        DEBUG_STATS = 0x1,
        DEBUG_TEXT = 0x2,

        ALL = (DEBUG_STATS | DEBUG_TEXT),

        COUNT = 5
    };
}

OMAF_NS_END
