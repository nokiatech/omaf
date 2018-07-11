
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
#include "Platform/OMAFPlatformDetection.h"
#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRStringUtilities.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRHttpConnection.h"

#if OMAF_PLATFORM_UWP

OMAF_NS_BEGIN
namespace Http
{
    HttpConnection* createHttpConnectionUWP(MemoryAllocator& allocator);
}
OMAF_NS_END

#elif OMAF_PLATFORM_WINDOWS

OMAF_NS_BEGIN
namespace Http
{
    HttpConnection* createHttpConnectionWIN(MemoryAllocator& allocator);
}
OMAF_NS_END

#elif OMAF_PLATFORM_ANDROID

#include "Android/NVRAndroidHttpConnection.h"

#endif

OMAF_NS_BEGIN

OMAF_LOG_ZONE(Http)

namespace Http
{
    static FixedString1024 gDefaultAgent;

    void_t setDefaultUserAgent(const utf8_t* aUserAgent)
    {
        if (StringByteLength(aUserAgent) < gDefaultAgent.getCapacity())
        {
            gDefaultAgent = aUserAgent;
        }
        else
        {
            OMAF_LOG_E("Failed to set Default HTTP User-Agent. Maximum length is %d and tried to set string: %s",
                (gDefaultAgent.getCapacity() - 1), aUserAgent);
        }
    }

    const utf8_t* getDefaultUserAgent()
    {
        return gDefaultAgent.getData();
    }

    HttpConnection* createHttpConnection(MemoryAllocator& allocator)
    {
        HttpConnection* connection =
#if OMAF_PLATFORM_UWP
            createHttpConnectionUWP(allocator);
#elif OMAF_PLATFORM_WINDOWS
        createHttpConnectionWIN(allocator);
#elif OMAF_PLATFORM_ANDROID
            OMAF_NEW(allocator, AndroidHttpConnection)(allocator);
#else
#error No HttpConnection defined
#endif

        OMAF_ASSERT_NOT_NULL(connection);
        connection->setUserAgent(getDefaultUserAgent());
        return connection;
    }
}
OMAF_NS_END
