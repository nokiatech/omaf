
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

#include <iostream>
#include <vector>

#include "omaf/parser/h265parser.hpp"

namespace VDD
{
    class H265StdIStream : public H265InputStream
    {
    public:
        H265StdIStream(std::istream& aStream);

        uint8_t getNextByte() override;
        bool eof() override;

        // This implementation is not able to rewind more than 4 consecutive bytes (unless 4 other
        // bytes are read in between)
        void rewind(size_t) override;

    private:
        std::istream& mStream;
        std::vector<uint8_t> mBuffer;  // always hold at least four bytes as required by H265Parser
        std::size_t mBufferSize = 0;
        std::size_t mReadOffset = 0;
        bool mEof = false;

        void fillBuffer();
    };
}  // namespace VDD
