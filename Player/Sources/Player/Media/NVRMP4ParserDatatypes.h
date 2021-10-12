
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

namespace FileFormatType
{
    enum Enum
    {
        UNDEFINED = -1,
        OMAF,
        OTHER_VR,

        COUNT
    };
}

namespace SeekAccuracy
{
    enum Enum
    {
        INVALID = -1,

        NEAREST_SYNC_FRAME,
        FRAME_ACCURATE,

        COUNT
    };
}

namespace SeekDirection
{
    enum Enum
    {
        INVALID = -1,

        PREVIOUS,
        NEXT,

        COUNT
    };
}


OMAF_NS_END
