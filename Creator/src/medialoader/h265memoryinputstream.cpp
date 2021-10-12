
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
#include <cassert>

#include "h265memoryinputstream.h"

namespace VDD
{
    H265MemoryInputStream::H265MemoryInputStream(const uint8_t* aData, size_t aDataSize)
        : H265InputStream()
        , mDataPtr(aData)
        , mIndex(0)
        , mSize(aDataSize)
    {
    }
    H265MemoryInputStream::~H265MemoryInputStream()
    {
    }
    uint8_t H265MemoryInputStream::getNextByte()
    {
        return mDataPtr[mIndex++];
    }
    bool H265MemoryInputStream::eof()
    {
        return mIndex == mSize;
    }
    void H265MemoryInputStream::rewind(size_t aNrBytes)
    {
        assert(mIndex - aNrBytes >= 0);
        mIndex -= aNrBytes;
    }
}
