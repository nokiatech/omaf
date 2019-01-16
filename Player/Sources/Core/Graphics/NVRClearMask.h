
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

namespace ClearMask
{
    enum Enum
    {
        INVALID = -1,
        
        NONE = 0,

        COLOR_BUFFER = 0x01,
        DEPTH_BUFFER = 0x02,
        STENCIL_BUFFER = 0x04,

        ALL = (COLOR_BUFFER | DEPTH_BUFFER | STENCIL_BUFFER),
        
        COUNT
    };
}

OMAF_NS_END
