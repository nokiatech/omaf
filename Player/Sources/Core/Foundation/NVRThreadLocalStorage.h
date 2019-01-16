
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
#pragma once

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFPlatformDetection.h"
#include "Platform/OMAFDataTypes.h"

#include "Foundation/NVRDependencies.h"
#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"

OMAF_NS_BEGIN

//
// Wrapper for OS TLS.
//

class ThreadLocalStorage
{
    public:
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

        typedef DWORD Handle;

#elif OMAF_PLATFORM_ANDROID

        typedef pthread_key_t Handle;

#else

    #error Unsupported platform

#endif
    
        ThreadLocalStorage();

        ~ThreadLocalStorage();
    
        // Get data from calling thread TLS.
        void_t* getValue() const;

        // Get data from calling thread TLS.
        template <typename T>
        T* getValue() const
        {
            return (T*)getValue();
        }
    
        // Set value for calling thread TLS.
        bool_t setValue(void_t* value);

    private:

        OMAF_NO_COPY(ThreadLocalStorage);
        OMAF_NO_ASSIGN(ThreadLocalStorage);

    private:

        Handle mHandle;
};

OMAF_NS_END
