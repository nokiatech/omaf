
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

#include "NVRNamespace.h"

OMAF_NS_BEGIN
    namespace AudioReturnValue
    {
        enum Enum
        {
            INVALID = -1,
            
            OK,
            
            // This source has now given all the samples it has, there will be no more.
            END_OF_FILE,
            
            // This source is currently out of samples, but it might get more later
            OUT_OF_SAMPLES,
            
            // There was unrecoverable error while processing the request.
            FAIL,
            
            // Source has not been initialized. Do that first, then try again.
            NOT_INITIALIZED,
            
            COUNT
        };
    }
OMAF_NS_END
