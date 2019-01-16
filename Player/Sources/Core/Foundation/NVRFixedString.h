
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

#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFCompiler.h"
#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRFixedArray.h"
#include "Foundation/NVRStringUtilities.h"

OMAF_NS_BEGIN
    template <size_t N>
    class FixedString
    {
        public:

            FixedString();
            FixedString(const utf8_t* string);
            FixedString(const utf8_t* string, size_t length);
            FixedString(const FixedString<N>& string);

            ~FixedString();

            size_t getLength() const;
            size_t getSize() const;
            size_t getCapacity() const;

            utf8_t* getData();
            const utf8_t* getData() const;

            bool_t isEmpty() const;

            void_t clear();
            void_t resize(size_t size);

            utf8_t* at(size_t index);
            const utf8_t* at(size_t index) const;

            void_t append(const utf8_t* string);
            void_t append(const utf8_t* string, size_t length);
            void_t append(const FixedString<N>& string);

            void_t appendFormat(const utf8_t* format, ...);
            void_t appendFormatVar(const utf8_t* format, va_list args);

            FixedString<N> substring(size_t index, size_t length = Npos) const;

            size_t findFirst(const utf8_t* string, size_t startIndex = 0) const;
            size_t findLast(const utf8_t* string, size_t startIndex = Npos) const;

            ComparisonResult::Enum compare(const utf8_t* string) const;
            ComparisonResult::Enum compare(const FixedString<N>& string) const;

            // Operators
            FixedString<N>& operator = (const utf8_t* string);
            FixedString<N>& operator = (const FixedString<N>& string);

            bool_t operator == (const utf8_t* string) const;
            bool_t operator == (const FixedString<N>& string) const;

            bool_t operator != (const utf8_t* string) const;
            bool_t operator != (const FixedString<N>& string) const;

            operator const utf8_t*() const;

        private:

            FixedArray<utf8_t, N + 1> mData;
    };
    
    typedef FixedString<8> FixedString8;
    typedef FixedString<16> FixedString16;
    typedef FixedString<32> FixedString32;
    typedef FixedString<64> FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
    typedef FixedString<1024> FixedString1024;
    typedef FixedString<2048> FixedString2048;
    typedef FixedString<4096> FixedString4096;
    typedef FixedString<8192> FixedString8192;
    typedef FixedString<16384> FixedString16384;
OMAF_NS_END

#include "Foundation/NVRFixedStringImpl.h"
