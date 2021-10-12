
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

#include "Graphics/NVRDependencies.h"

OMAF_NS_BEGIN

#define OMAF_ENABLE_DX_DEBUGGING 0

#if OMAF_ENABLE_DX_DEBUGGING && OMAF_ENABLE_LOG

#define OMAF_DX_CHECK(call)                                                                                      \
    {                                                                                                            \
        do                                                                                                       \
        {                                                                                                        \
            HRESULT result = call;                                                                               \
                                                                                                                 \
            if (FAILED(result))                                                                                  \
            {                                                                                                    \
                DWORD errorCode = GetLastError();                                                                \
                char* errorStr;                                                                                  \
                                                                                                                 \
                if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, \
                                   MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), (LPTSTR) &errorStr, 0, NULL))  \
                {                                                                                                \
                    OMAF_ASSERT(false, "");                                                                      \
                }                                                                                                \
                                                                                                                 \
                OMAF_LOG_D("D3D11 error:%d %s", int(errorCode), errorStr);                                       \
                LocalFree(errorStr);                                                                             \
                                                                                                                 \
                OMAF_ASSERT(false, "");                                                                          \
            }                                                                                                    \
        } while (false);                                                                                         \
    }

#else

#define OMAF_DX_CHECK(call) call

#endif

OMAF_NS_END
