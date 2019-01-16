
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

namespace IndexBufferFormat
{
    enum Enum
    {
        INVALID = -1,
        
        UINT8,
        UINT16,
        UINT32,

        COUNT
    };
}

namespace IndexBufferFormat
{
    uint8_t getSize(IndexBufferFormat::Enum format);
}

OMAF_NS_END
