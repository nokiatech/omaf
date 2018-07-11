
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
#include "Graphics/NVRIndexBufferFormat.h"
#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN

namespace IndexBufferFormat
{
    uint8_t getSize(IndexBufferFormat::Enum format)
    {
        switch (format)
        {
            case IndexBufferFormat::UINT8:  return 1;
            case IndexBufferFormat::UINT16: return 2;
            case IndexBufferFormat::UINT32: return 4;
                
            default:
                OMAF_ASSERT_UNREACHABLE();
                break;
        }
        
        return 0;
    }
}

OMAF_NS_END
