
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
#include "customallocator.hpp"

class DefaultAllocator : public MP4VR::CustomAllocator
{
public:
    DefaultAllocator()
    {
    }
    ~DefaultAllocator()
    {
    }

    void* allocate(size_t n, size_t size) override
    {
        return malloc(n * size);
    }
    void deallocate(void* ptr) override
    {
        free(ptr);
    }
};

char defaultAllocatorData[sizeof(DefaultAllocator)];
MP4VR::CustomAllocator* defaultAllocator;
static MP4VR::CustomAllocator* customAllocator;

bool setCustomAllocator(MP4VR::CustomAllocator* customAllocator_)
{
    if (!customAllocator || !customAllocator_)
    {
        customAllocator = customAllocator_;
        return true;
    }
    else
    {
        return false;
    }
}

MP4VR::CustomAllocator* getDefaultAllocator()
{
    if (!defaultAllocator)
    {
        defaultAllocator = new (defaultAllocatorData) DefaultAllocator();
    }
    return defaultAllocator;
}

MP4VR::CustomAllocator* getCustomAllocator()
{
    if (!customAllocator)
    {
        defaultAllocator = getDefaultAllocator();
        customAllocator  = defaultAllocator;
    }
    return customAllocator;
}

void* customAllocate(size_t size)
{
    return getCustomAllocator()->allocate(size, 1);
}

void customDeallocate(void* ptr)
{
    getCustomAllocator()->deallocate(ptr);
}
