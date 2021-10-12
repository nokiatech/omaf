
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
#ifndef STREAMSEGMENTER_BLOCK_HPP
#define STREAMSEGMENTER_BLOCK_HPP

#include <iostream>

#include "bitstream.hpp"
#include "customallocator.hpp"

namespace StreamSegmenter
{
    struct Block
    {
        Block()
            : blockOffset(0)
            , blockSize(0)
        {
        }
        Block(size_t aBlockOffset, size_t aBlockSize);

        size_t blockOffset;
        size_t blockSize;

        Vector<std::uint8_t> getData(std::istream& aStream) const;
        BitStream getBitStream(std::istream& aStream) const;
    };
}

#endif  // STREAMSEGMENTER_BLOCK_HPP
