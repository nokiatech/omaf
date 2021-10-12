
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

namespace ComputeBufferAccess
{
    enum Enum
    {
        INVALID = -1,

        NONE = 0x0,   // Buffer has no read / write access by compute shader
        READ = 0x1,   // Buffer is read by compute shader.
        WRITE = 0x2,  // Buffer will be written by compute shader, buffer cannot be written by CPU.
        READ_WRITE =
            READ | WRITE,  // Buffer will be read and written by compute shader, buffer cannot be written by CPU.

        COUNT = 4
    };
}

OMAF_NS_END
