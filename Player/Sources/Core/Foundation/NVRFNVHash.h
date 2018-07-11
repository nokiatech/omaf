
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

#include "Foundation/NVRCompatibility.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"

OMAF_NS_BEGIN
    OMAF_INLINE uint64_t FNVHash(const char_t* str);

    // Calculate FNV-1a hash for null-terminated C-string, http://isthe.com/chongo/tech/comp/fnv/
    uint64_t FNVHash(const char_t* str)
    {
        static uint64_t offset = 2166136261u;
        static uint64_t prime = 16777619u;
        
        uint64_t hash = offset;
        
        while (*str != OMAF_NULL)
        {
            hash ^= (uint64_t)*str++;
            hash *= prime;
        }
        
        return hash;
    }
OMAF_NS_END
