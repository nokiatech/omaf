
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

#include "h265stdistream.h"

namespace VDD
{
    H265StdIStream::H265StdIStream(std::istream& aStream) : mStream(aStream), mBuffer(1024)
    {
        fillBuffer();
    }

    void H265StdIStream::fillBuffer()
    {
        size_t writeOffset = 0;
        if (mBufferSize)
        {
            // preserve at least 4 last bytes
            writeOffset = std::min(mBufferSize, size_t(4));
            std::copy(&mBuffer[0] + mBufferSize - writeOffset, &mBuffer[0] + mBufferSize,
                      &mBuffer[0]);
        }

        // readsome doesn't work on Windows as we'd hope, so let's just keep track of how much we
        // got
        std::streamsize bytesRead = mStream.tellg();
        mStream.read(reinterpret_cast<char*>(&mBuffer[0] + writeOffset),
                     static_cast<std::streamsize>(mBuffer.size() - writeOffset));
        if (mStream.eof())
        {
            // eof and other errors are handled by dealing with bytesRead
            mStream.clear();
        }
        bytesRead = std::streamsize(mStream.tellg() - std::streampos(bytesRead));
        mReadOffset = writeOffset;
        mBufferSize = size_t(bytesRead) + writeOffset;

        mEof = bytesRead == 0;
    }

    uint8_t H265StdIStream::getNextByte()
    {
        if (mReadOffset == mBufferSize)
        {
            fillBuffer();
        }
        if (mReadOffset != mBufferSize)
        {
            auto value = mBuffer[mReadOffset];
            ++mReadOffset;
            return value;
        }
        else
        {
            return 0;
        }
    }

    void H265StdIStream::rewind(size_t aOffset)
    {
        assert(mReadOffset >= aOffset);
        mReadOffset -= aOffset;
    }

    bool H265StdIStream::eof()
    {
        if (!mEof && mReadOffset == mBufferSize)
        {
            fillBuffer();
        }
        return mReadOffset == mBufferSize;
    }
}  // namespace VDD
