
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

namespace StencilOperation
{
    enum Enum
    {
        INVALID = -1,

        KEEP,
        ZERO,
        REPLACE,
        INCREMENT,
        INCREMENT_WRAP,
        DECREMENT,
        DECREMENT_WRAP,
        INVERT,

        COUNT
    };
}

OMAF_NS_END
