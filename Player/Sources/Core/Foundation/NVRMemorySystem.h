
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

#include "Foundation/NVRBuild.h"
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRMemoryAllocator.h"

OMAF_NS_BEGIN

namespace MemorySystem
{
    extern const uint32_t DEFAULT_SCRATCH_ALLOCATOR_SIZE;

    void_t Create(MemoryAllocator* clientHeapAllocator = OMAF_NULL, size_t scratchAllocatorSize = DEFAULT_SCRATCH_ALLOCATOR_SIZE);
    void_t Destroy();

    MemoryAllocator* DefaultHeapAllocator();
    MemoryAllocator* DefaultScratchAllocator();

    MemoryAllocator* DebugHeapAllocator();
}

OMAF_NS_END
