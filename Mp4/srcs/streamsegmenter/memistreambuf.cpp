
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
#include "memistreambuf.hpp"

namespace StreamSegmenter
{
    MemIStreambuf::MemIStreambuf(const char* aData, size_t aLength)
    {
        auto data = const_cast<char*>(aData);
        setg(data, data, data + aLength);
    }
}
