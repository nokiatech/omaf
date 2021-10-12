
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

#include <cstdint>
#include "omaf/parser/h265parser.hpp"

namespace VDD
{
    /**
     *  Read interface to the NAL data stream
     */
    class H265MemoryInputStream : public H265InputStream
    {
    public:
        H265MemoryInputStream(const uint8_t* aData, size_t aDataSize);
        ~H265MemoryInputStream();

        uint8_t getNextByte();
        bool eof();
        void rewind(size_t aNrBytes);

    private:
        const uint8_t* mDataPtr;
        size_t mIndex;
        size_t mSize;
    };
}  // namespace VDD