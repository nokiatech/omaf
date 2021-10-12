
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
#include "bitgenerator.h"

#include <cstring>

namespace VDD {

    BitGenerator::BitGenerator()
        : mCurBit(0)
    {
        // nothing
    }

    BitGenerator::BitGenerator(BitGenerator&& other)
        : mCurBit(other.mCurBit)
        , mData(std::move(other.mData))
    {
        // nothing
    }

    void BitGenerator::bit(bool value)
    {
        --mCurBit;
        if (mCurBit == -1) {
            mCurBit = 7;
            mData.push_back(0);
        }
        mData[mData.size() - 1] |= (uint8_t(value) << mCurBit);
    }

    void BitGenerator::uintn(uint8_t nBits, unsigned value)
    {
        for (int c = nBits - 1; c >= 0; --c) {
            bit((value >> c) & 1);
        }
    }

    void BitGenerator::intn(uint8_t nBits, int value)
    {
        return uintn(nBits, static_cast<unsigned>(value));
    }

    void BitGenerator::float_(float value)
    {
        uint32_t i;
        std::memcpy(reinterpret_cast<char*>(&i), reinterpret_cast<char*>(&value), sizeof(i));
        uintn(32, i);
    }

    const std::vector<uint8_t>& BitGenerator::getData() const
    {
        return mData;
    }

    std::vector<uint8_t>&& BitGenerator::moveData()
    {
        return std::move(mData);
    }

} // VDD
