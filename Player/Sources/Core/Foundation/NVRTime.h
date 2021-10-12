
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

#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
namespace Time
{
    int32_t getClockTimeMs();
    int64_t getClockTimeUs();

    int32_t getProcessTimeMs();
    int64_t getProcessTimeUs();

    int32_t getThreadTimeMs();
    int64_t getThreadTimeUs();

    time_t fromUTCString(const char_t* timeStr);
}  // namespace Time
OMAF_NS_END
