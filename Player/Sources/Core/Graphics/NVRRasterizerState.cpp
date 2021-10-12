
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
#include "Graphics/NVRRasterizerState.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN

bool_t equals(const RasterizerState& left, const RasterizerState& right)
{
    int32_t result = MemoryCompare(&left, &right, sizeof(RasterizerState));

    return (result == 0) ? true : false;
}

OMAF_NS_END
