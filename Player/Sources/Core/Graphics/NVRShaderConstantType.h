
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

namespace ShaderConstantType
{
    enum Enum
    {
        INVALID = -1,

        BOOL,
        BOOL2,
        BOOL3,
        BOOL4,

        INTEGER,
        INTEGER2,
        INTEGER3,
        INTEGER4,

        FLOAT,
        FLOAT2,
        FLOAT3,
        FLOAT4,

        MATRIX22,
        MATRIX33,
        MATRIX44,

        SAMPLER_2D,
        SAMPLER_3D,
        SAMPLER_CUBE,

        COUNT
    };
}

OMAF_NS_END
