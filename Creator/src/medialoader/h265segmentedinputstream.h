
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

#include <functional>
#include <iostream>
#include <vector>

#include "omaf/parser/h265parser.hpp"

namespace VDD
{
    class H265SegmentedInputStreamError : H265InputStreamException
    {
    public:
        H265SegmentedInputStreamError(std::string aWhat) : H265InputStreamException(aWhat)
        {
        }

        const char* what() const throw() override
        {
            return "H265SegmentedStreamError";
        }
    };

    class H265SegmentedInputStream : public H265InputStream
    {
    public:
        using OpenStream = std::function<std::unique_ptr<H265InputStream>(size_t index)>;

        H265SegmentedInputStream(OpenStream aOpenStream);

        uint8_t getNextByte() override;
        bool eof() override;

        // This implementation is not able to rewind more than the underlying streams
        void rewind(size_t) override;

    private:
        OpenStream mOpenStream;
        std::unique_ptr<H265InputStream> mInputStream;
        // mPrevStream may ba accessed with rewind
        std::unique_ptr<H265InputStream> mPrevStream;
        int mReadOffset = 0; // may be negative if we are in mPrevStream
        size_t mCurrentIndex = 0u;

        void fillBuffers();
    };
}  // namespace VDD
