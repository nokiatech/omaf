
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
#include <algorithm>

#include "h265segmentedinputstream.h"

namespace VDD
{
    H265SegmentedInputStream::H265SegmentedInputStream(OpenStream aOpenStream)
        : mOpenStream(aOpenStream)
    {
        if (auto stream = aOpenStream(mCurrentIndex))
        {
            mInputStream = std::move(stream);
        }
    }

    uint8_t H265SegmentedInputStream::getNextByte()
    {
        fillBuffers();

        if (eof())
        {
            return 0u;
        }
        else
        {
            H265InputStream& stream = mReadOffset >= 0 ? *mInputStream : *mPrevStream;
            uint8_t byte = stream.getNextByte();
            ++mReadOffset;

            fillBuffers(); // so eof works OK
            return byte;
        }
    }

    void H265SegmentedInputStream::fillBuffers()
    {
        if (mReadOffset >= 0)
        {
            while (mInputStream && mInputStream->eof())
            {
                mPrevStream = std::move(mInputStream);
                ++mCurrentIndex;
                mInputStream.reset();

                // it's fine for mOpenStream to return null
                mInputStream = mOpenStream(mCurrentIndex);
                mReadOffset = 0;
            }
        }
    }

    bool H265SegmentedInputStream::eof()
    {
        H265InputStream* stream = mReadOffset >= 0 ? mInputStream.get() : mPrevStream.get();
        return !stream || stream->eof();
    }

    void H265SegmentedInputStream::rewind(size_t aCount)
    {
        if (mReadOffset >= static_cast<int>(aCount))
        {
            // simple fastpath
            mReadOffset -= aCount;
            mInputStream->rewind(aCount);
        }
        else
        {
            // and even simpler slowpath
            while (aCount > 0u)
            {
                --mReadOffset;
                --aCount;
                if (mReadOffset < 0 && !mPrevStream)
                {
                    throw H265SegmentedInputStreamError("Cannot rewind; missing segment");
                }
                if (mReadOffset >= 0)
                {
                    mInputStream->rewind(1);
                }
                else
                {
                    mPrevStream->rewind(1);
                }
            }
        }
    }
}  // namespace VDD