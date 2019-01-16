
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
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRMemoryAllocator.h"
#include "Foundation/NVRArray.h"
#include "Foundation/NVRStringUtilities.h"

OMAF_NS_BEGIN
    class String
    {
        public:

            String(MemoryAllocator& allocator, size_t capacity = 0);
            String(MemoryAllocator& allocator, const utf8_t* string);
            String(MemoryAllocator& allocator, const utf8_t* string, size_t length);
            String(const String& string);

            ~String();

            MemoryAllocator& getAllocator();

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
            void_t append(const String& string);

            void_t appendFormat(const utf8_t* format, ...);
            void_t appendFormatVar(const utf8_t* format, va_list args);

            String substring(size_t index, size_t length = Npos) const;

            size_t findFirst(const utf8_t* string, size_t startIndex = 0) const;
            size_t findLast(const utf8_t* string, size_t startIndex = 0) const;

            ComparisonResult::Enum compare(const utf8_t* string) const;
            ComparisonResult::Enum compare(const String& string) const;

            // Operators
            String& operator = (const utf8_t* string);
            String& operator = (const String& string);

            bool_t operator == (const utf8_t* string) const;
            bool_t operator == (const String& string) const;

            bool_t operator != (const utf8_t* string) const;
            bool_t operator != (const String& string) const;

            operator const utf8_t*() const;

        private:

            OMAF_NO_DEFAULT(String);

        private:

            MemoryAllocator& mAllocator;

            Array<utf8_t> mData;
    };
OMAF_NS_END
