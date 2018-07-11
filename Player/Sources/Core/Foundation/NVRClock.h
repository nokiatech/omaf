
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRDependencies.h"

OMAF_NS_BEGIN

    namespace Clock
    {
        uint64_t getTicks();
        
        uint64_t getMicroseconds();
        uint64_t getMilliseconds();
        float64_t getSeconds();
        
        uint64_t getTicksPerSecond();
        uint64_t getTicksPerMs();
        
        float64_t getSecondsPerTick();
        float64_t getMsPerTick();
        
        uint64_t getSecondsSinceReference();

        time_t getEpochTime();
        uint64_t getRandomSeed();

        bool_t getTimestampString(utf8_t buffer[64]);
        bool_t getTimestampString(utf8_t buffer[64], time_t timestamp);
    }

OMAF_NS_END
