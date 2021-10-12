
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
#include "block.hpp"
#include "api/streamsegmenter/exceptions.hpp"

namespace StreamSegmenter
{
    Block::Block(size_t aBlockOffset, size_t aBlockSize)
        : blockOffset(aBlockOffset)
        , blockSize(aBlockSize)
    {
        // nothing
    }

    Vector<std::uint8_t> Block::getData(std::istream& aStream) const
    {
        Vector<std::uint8_t> buffer(blockSize);
        aStream.seekg(std::streamoff(blockOffset));
        aStream.read(reinterpret_cast<char*>(&buffer[0]), std::streamsize(blockSize));
        return buffer;
    }

    BitStream Block::getBitStream(std::istream& aStream) const
    {
        Vector<std::uint8_t> buffer(blockSize);
        aStream.seekg(std::streamoff(blockOffset));
        aStream.read(reinterpret_cast<char*>(&buffer[0]), std::streamsize(blockSize));
        if (!aStream)
        {
            throw PrematureEndOfFile();
        }
        return BitStream(buffer);
    }

}  // namespace StreamSegmenter
