
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
#ifndef BITSTREAM_HPP
#define BITSTREAM_HPP

#include <string>
#include <vector>

namespace Parser
{
    class BitStream
    {
    public:
        BitStream();
        BitStream(std::vector<std::uint8_t>& strData);
        ~BitStream();

        std::uint32_t getPos() const;
        std::uint32_t getSize() const;
        void setSize(unsigned int newSize);
        std::vector<std::uint8_t>& getStorage();

        /** Reset offsets to beginning */
        void reset();

        void setPosition(unsigned int position);
        void clear();
        void skipBytes(unsigned int x);
        void setByte(unsigned int offset, std::uint8_t byte);
        void writeBits(unsigned int bits, unsigned int len);
        void write8Bits(unsigned int bits);
        void write16Bits(unsigned int bits);
        void write24Bits(unsigned int bits);
        void write32Bits(unsigned int bits);
        void write64Bits(unsigned long long int bits);
        void write8BitsArray(std::vector<std::uint8_t>& bits, unsigned int len, unsigned int srcOffset = 0);
        void writeString(const std::string& srcString);
        void writeZeroTerminatedString(const std::string& srcString);

        std::uint8_t getByte(unsigned int offset) const;
        unsigned int readBits(int len);
        std::uint8_t read8Bits();
        std::uint16_t read16Bits();
        unsigned int read24Bits();
        unsigned int read32Bits();
        unsigned long long read64Bits();
        void read8BitsArray(std::vector<std::uint8_t>& bits, unsigned int len);
        void readStringWithLen(std::string& dstString, unsigned int len);
        void readStringWithPosAndLen(std::string& dstString, unsigned int pos, unsigned int len);
        void readZeroTerminatedString(std::string& dstString);
        unsigned int readExpGolombCode();
        int readSignedExpGolombCode();

        unsigned int numBytesLeft() const;

        /**
         * Clear destination BitStream and copy part of BitStream to it.
         *
         * @param [in] begin Start offset from the bitstream begin
         * @param [in] end   End offset from the bitstream begin
         * @param [out] dest Destination BitStream.
         */
        void extract(int begin, int end, BitStream& dest) const;

        void append(BitStream& src);

        // Ali's additions
        void writeExpGolombCode(unsigned int code);
        void writeSignedExpGolombCode(int signedVal);
        unsigned int getBitOffset();
        unsigned int getByteOffset();

    private:
        std::vector<std::uint8_t> mStorage;
        unsigned int mCurrByte;
        unsigned int mByteOffset;
        unsigned int mBitOffset;
        bool mStorageAllocated;
    };
}
#endif /* end of include guard: BITSTREAM_HPP */

