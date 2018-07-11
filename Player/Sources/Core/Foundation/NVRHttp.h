
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

#include "Foundation/NVRHttpConnection.h"

OMAF_NS_BEGIN
namespace Http
{
    HttpConnection* createHttpConnection(MemoryAllocator& allocator);

    void_t setDefaultUserAgent(const utf8_t* aUserAgent);

    const utf8_t* getDefaultUserAgent();
}
OMAF_NS_END
