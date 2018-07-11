
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

#include "Platform/OMAFCompiler.h"
#include "Foundation/NVRDependencies.h"
#include "Platform/OMAFDataTypes.h"
#include "Foundation/NVRString.h"
#include "Foundation/NVRDataBuffer.h"

OMAF_NS_BEGIN
    namespace Base64
    {
        void_t encode(const String& input, String& output);
        void_t decode(const String& input, DataBuffer<uint8_t>& output);
    }
OMAF_NS_END
