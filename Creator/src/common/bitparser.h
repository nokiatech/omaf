
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

namespace VDD {
    template <typename Iterator>
    class BitParser {
    public:
        BitParser(Iterator begin, Iterator end);
        ~BitParser();

        uint8_t read1();
        uint32_t readn(uint8_t n);
        int32_t readnSigned(uint8_t n);
        float readFloat();

        void byteAlign();

        bool eod() const; // only byte-precise
        bool prematureEod() const; // only byte-precise

    private:
        Iterator mAt;
        Iterator mEnd;
        int      mCurBit;
        bool     mPrematureEod;
    };
}

#include "bitparser.icpp"
