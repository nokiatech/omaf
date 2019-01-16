
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
#include <vector>

namespace VDD {

    class BitGenerator {
    public:
        BitGenerator();
        BitGenerator(const BitGenerator& other) = default;
        BitGenerator(BitGenerator&& other);

        const std::vector<uint8_t>& getData() const;
        std::vector<uint8_t>&& moveData();

        void bit(bool value);
        void uintn(uint8_t nBits, unsigned value);
        void intn(uint8_t nBits, int value);
        void float_(float value);

    private:
        int                  mCurBit;
        std::vector<uint8_t> mData;
    };

} // VDD
