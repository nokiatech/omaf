
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

namespace DiscardMask
{
    enum Enum
    {
        INVALID = -1,
        
        NONE = 0,

        DISCARD_COLOR_0 = 0x01,
        DISCARD_COLOR_1 = 0x02,
        DISCARD_COLOR_2 = 0x04,
        DISCARD_COLOR_3 = 0x08,
        DISCARD_COLOR_4 = 0x10,
        DISCARD_COLOR_5 = 0x20,
        DISCARD_COLOR_6 = 0x40,
        DISCARD_COLOR_7 = 0x80,
        
        DISCARD_COLOR_ALL = (DISCARD_COLOR_0 | DISCARD_COLOR_1 | DISCARD_COLOR_2 | DISCARD_COLOR_3 |
                             DISCARD_COLOR_4 | DISCARD_COLOR_5 | DISCARD_COLOR_6 | DISCARD_COLOR_7),
        
        DISCARD_DEPTH = 0x100,
        DISCARD_STENCIL = 0x200,

        ALL = (DISCARD_COLOR_ALL | DISCARD_DEPTH | DISCARD_STENCIL),
        
        COUNT
    };
}

OMAF_NS_END
