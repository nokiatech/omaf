
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
#include "bitstream.hpp"
//#include "log.hpp"
#include <assert.h>
#include <cstdlib>

using namespace std;

namespace Parser
{
    BitStream::BitStream() :
        mStorage(),
        mCurrByte(0),
        mByteOffset(0),
        mBitOffset(0),
        mStorageAllocated(true)
    {
    }

    BitStream::BitStream(vector<uint8_t>& strData) :
        mStorage(strData),
        mCurrByte(0),
        mByteOffset(0),
        mBitOffset(0),
        mStorageAllocated(false)
    {
    }

    BitStream::~BitStream()
    {
        if (mStorageAllocated == true)
        {
            mStorage.clear();
        }
    }

    void BitStream::setPosition(const unsigned int position)
    {
        mByteOffset = position;
    }


    uint32_t BitStream::getPos() const
    {
        return mByteOffset;
    }

    uint32_t BitStream::getSize() const
    {
        return mStorage.size();
    }

    void BitStream::setSize(const unsigned int newSize)
    {
        mStorage.resize(newSize);
    }

    vector<uint8_t>& BitStream::getStorage()
    {
        return mStorage;
    }

    void BitStream::reset()
    {
        mCurrByte = 0;
        mBitOffset = 0;
        mByteOffset = 0;
    }

    void BitStream::clear()
    {
        mStorage.clear();
    }

    void BitStream::skipBytes(const unsigned int x)
    {
        mByteOffset += x;
    }

    void BitStream::setByte(const unsigned int offset, const uint8_t byte)
    {
        mStorage.at(offset) = byte;
    }

    uint8_t BitStream::getByte(const unsigned int offset) const
    {
        return mStorage.at(offset);
    }

    unsigned int BitStream::numBytesLeft() const
    {
        return mStorage.size() - mByteOffset;
    }
    void BitStream::extract(const int begin, const int end, BitStream& dest) const
    {
        dest.clear();
        dest.reset();
        dest.mStorage.insert(dest.mStorage.begin(), mStorage.begin() + begin, mStorage.begin() + end);
    }

    void BitStream::write8Bits(const unsigned int bits)
    {
        mStorage.push_back(static_cast<uint8_t>(bits));
    }

    void BitStream::write16Bits(const unsigned int bits)
    {
        mStorage.push_back(static_cast<uint8_t>((bits >> 8) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits) & 0xff));
    }

    void BitStream::write24Bits(const unsigned int bits)
    {
        mStorage.push_back(static_cast<uint8_t>((bits >> 16) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 8) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits) & 0xff));
    }

    void BitStream::write32Bits(const unsigned int bits)
    {
        mStorage.push_back(static_cast<uint8_t>((bits >> 24) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 16) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 8) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits) & 0xff));
    }

    // TODO: check if this actually works;
    void BitStream::write64Bits(const unsigned long long int bits)
    {
        mStorage.push_back(static_cast<uint8_t>((bits >> 56) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 48) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 40) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 32) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 24) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 16) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits >> 8) & 0xff));
        mStorage.push_back(static_cast<uint8_t>((bits) & 0xff));
    }

    void BitStream::write8BitsArray(vector<uint8_t>& bits, const unsigned int len, const unsigned int srcOffset)
    {
        mStorage.insert(mStorage.end(), bits.begin() + srcOffset, bits.begin() + srcOffset + len);
    }

    void BitStream::writeBits(const unsigned int bits, unsigned int len)
    {
        unsigned int numBitsLeftInByte;

        do
        {
            numBitsLeftInByte = 8 - mBitOffset;
            if (numBitsLeftInByte > len)
            {
                mCurrByte = mCurrByte | ((bits & ((1 << len) - 1)) << (numBitsLeftInByte - len));
                mBitOffset += len;
                len = 0;
            }
            else
            {
                mCurrByte = mCurrByte | ((bits >> (len - numBitsLeftInByte)) & ((1 << numBitsLeftInByte) - 1));
                mStorage.push_back((uint8_t)mCurrByte);
                mBitOffset = 0;
                mCurrByte = 0;
                len -= numBitsLeftInByte;
            }
        } while (len > 0);
    }

    void BitStream::writeString(const string& srcString)
    {
        if (srcString.length() == 0)
        {
            //logWarning() << "BitStream::writeString called for zero-length string." << endl;
        }

        for (const auto character : srcString)
        {
            mStorage.push_back(character);
        }
    }

    void BitStream::writeZeroTerminatedString(const string& srcString)
    {
        for (const auto character : srcString)
        {
            mStorage.push_back(character);
        }
        mStorage.push_back('\0');
    }

    uint8_t BitStream::read8Bits()
    {
        const uint8_t ret = mStorage.at(mByteOffset);
        ++mByteOffset;
        return ret;
    }

    uint16_t BitStream::read16Bits()
    {
        uint16_t ret = mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        return ret;
    }

    unsigned int BitStream::read24Bits()
    {
        unsigned int ret = mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        return ret;
    }

    unsigned int BitStream::read32Bits()
    {
        unsigned int ret = mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        return ret;
    }

    unsigned long long int BitStream::read64Bits()
    {
        unsigned long long int ret = mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;
        ret = (ret << 8) | mStorage.at(mByteOffset);
        mByteOffset++;

        return ret;
    }

    void BitStream::read8BitsArray(vector<uint8_t>& bits, const unsigned int len)
    {
        bits.insert(bits.end(), mStorage.begin() + mByteOffset, mStorage.begin() + mByteOffset + len);
        mByteOffset += len;
    }

    unsigned int BitStream::readBits(const int len)
    {
        unsigned int returnBits = 0;
        int numBitsLeftInByte = 8 - mBitOffset;

        if (numBitsLeftInByte >= len)
        {
            returnBits = ((mStorage).at(mByteOffset) >> (numBitsLeftInByte - len)) & ((1 << len) - 1);
            mBitOffset += len;
        }
        else
        {
            int numBitsToGo = len - numBitsLeftInByte;
            returnBits = (mStorage).at(mByteOffset) & ((1 << numBitsLeftInByte) - 1);
            mByteOffset++;
            mBitOffset = 0;
            while (numBitsToGo > 0)
            {
                if (numBitsToGo >= 8)
                {
                    returnBits = (returnBits << 8) | (mStorage).at(mByteOffset);
                    mByteOffset++;
                    numBitsToGo -= 8;
                }
                else
                {
                    returnBits = (returnBits << numBitsToGo)
                        | (((mStorage).at(mByteOffset) >> (8 - numBitsToGo)) & ((1 << numBitsToGo) - 1));
                    mBitOffset += numBitsToGo;
                    numBitsToGo = 0;
                }
            }
        }

        if (mBitOffset == 8)
        {
            mByteOffset++;
            mBitOffset = 0;
        }

        return returnBits;
    }

    void BitStream::readStringWithLen(string& dstString, const unsigned int len)
    {
        dstString.clear();
        for (unsigned int i = 0; i < len; i++)
        {
            char currChar = read8Bits();
            dstString += currChar;
        }
    }

    void BitStream::readStringWithPosAndLen(string& dstString, const unsigned int pos, const unsigned int len)
    {
        dstString.clear();
        for (unsigned int i = 0; i < len; i++)
        {
            char currChar = getByte(pos + i);
            dstString += currChar;
        }
    }

    void BitStream::readZeroTerminatedString(string& dstString)
    {
        char currChar;

        dstString.clear();
        currChar = read8Bits();
        while (currChar != '\0')
        {
            dstString += currChar;
            currChar = read8Bits();
        }
    }

    unsigned int BitStream::readExpGolombCode()
    {
        int leadingZeroBits = -1;
        unsigned int codeNum;
        int tmpBit = 0;

        while (tmpBit == 0)
        {
            tmpBit = readBits(1);
            leadingZeroBits++;
        }

        codeNum = (1 << leadingZeroBits) - 1 + readBits(leadingZeroBits);

        return codeNum;
    }

    int BitStream::readSignedExpGolombCode()
    {
        unsigned int codeNum = readExpGolombCode();
        int signedVal = int((codeNum + 1) >> 1);

        if ((codeNum & 1) == 0)
        {
            signedVal = -signedVal;
        }

        return signedVal;
    }

    // Ali's additions
    void BitStream::writeExpGolombCode(unsigned int codeNum)
    {
        if (codeNum == 0)
        {
            writeBits(1, 1);
            return;
        }

        codeNum++;
        int numBitsInCode = 2;

        while ((codeNum >> numBitsInCode) != 0)
        {
            numBitsInCode++;
        }

        int numLeadingZeros = numBitsInCode - 1;

        writeBits(0, numLeadingZeros);
        writeBits(codeNum, numBitsInCode);
    }

    void BitStream::writeSignedExpGolombCode(int signedVal)
    {
        if (signedVal == 0)
        {
            writeBits(1, 1);
            return;
        }

        unsigned int codeNum = (std::abs(signedVal) << 1) - 1 + (signedVal < 0 ? 1 : 0);

        writeExpGolombCode(codeNum);
    }

    unsigned int BitStream::getBitOffset()
    {
        return mBitOffset;
    }

    unsigned int BitStream::getByteOffset()
    {
        return mByteOffset;
    }

    void BitStream::append(BitStream& src)
    {
        assert(mBitOffset == 0);

        mStorage.insert(mStorage.end(), src.mStorage.begin(), src.mStorage.end());
        if (src.getBitOffset() != 0)
        {
            mBitOffset = src.getBitOffset();
            mCurrByte = src.mCurrByte;
        }
    }
}