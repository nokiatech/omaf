
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
#ifndef MP4VRFILEALLOCATOR_H
#define MP4VRFILEALLOCATOR_H

#include <cstdlib>
#include "mp4vrfileexport.h"

namespace MP4VR
{
    class MP4VR_DLL_PUBLIC CustomAllocator
    {
    public:
        CustomAllocator();
        virtual ~CustomAllocator();

        /* Allocate n objects each of size size */
        virtual void* allocate(size_t n, size_t size) = 0;

        /* Release a pointer returned from allocate */
        virtual void deallocate(void* ptr) = 0;
    };
}

#endif  // MP4VRFILEALLOCATOR_H
