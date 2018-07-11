
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "Foundation/NVRString.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    String::String(MemoryAllocator& allocator, size_t capacity)
    : mAllocator(allocator)
    , mData(allocator, capacity + 1)
    {
        mData.add(Eos);
    }
    
    String::String(MemoryAllocator& allocator, const utf8_t* string)
    : mAllocator(allocator)
    , mData(allocator)
    {
        if(string != OMAF_NULL)
        {
            size_t length = StringByteLength(string) + 1;
            mData.add(string, length, 0);
        }
    }
    
    String::String(MemoryAllocator& allocator, const utf8_t* string, size_t length)
    : mAllocator(allocator)
    , mData(allocator)
    {
        if(string != OMAF_NULL)
        {
            size_t bytes = StringByteLength(string, length);
            
            if(bytes > 0)
            {
                mData.add(string, bytes);
            }
        }
        
        mData.add(Eos);
    }
    
    String::String(const String& string)
    : mAllocator(string.mAllocator)
    , mData(string.mAllocator)
    {
        size_t bytes = string.getSize();
        
        if(bytes > 0)
        {
            mData.add(string.getData(), bytes);
        }
        else
        {
            mData.add(Eos);
        }
    }
    
    String::~String()
    {
    }
    
    MemoryAllocator& String::getAllocator()
    {
        return mAllocator;
    }
    
    size_t String::getLength() const
    {
        if(mData.getSize() != 0)
        {
            const utf8_t* data = mData.getData();
            
            if(data != OMAF_NULL)
            {
                return StringLength(data);
            }
        }
        
        return 0;
    }
    
    size_t String::getSize() const
    {
        return mData.getSize();
    }
    
    size_t String::getCapacity() const
    {
        size_t capacity = mData.getCapacity();
        
        return (capacity == 0) ? 0 : capacity - 1;
    }
    
    utf8_t* String::getData()
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }
        
        return OMAF_NULL;
    }
    
    const utf8_t* String::getData() const
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }
        
        return OMAF_NULL;
    }
    
    bool_t String::isEmpty() const
    {
        return (mData.getSize() <= 1) ? true : false;
    }
    
    void_t String::clear()
    {
        mData.clear();
        
        mData.add(&Eos, 1, 0);
    }
    
    void_t String::resize(size_t size)
    {
        mData.resize(size);
        mData[size - 1] = Eos;
    }
    
    utf8_t* String::at(size_t index)
    {
        OMAF_ASSERT(index < StringLength(mData.getData()), "");
        
        size_t offset = CharacterPosition(mData.getData(), index);
        
        return &mData[offset];
    }
    
    const utf8_t* String::at(size_t index) const
    {
        OMAF_ASSERT(index < StringLength(mData.getData()), "");
        
        size_t offset = CharacterPosition(mData.getData(), index);
        
        return &mData[offset];
    }
    
    void_t String::append(const utf8_t* string)
    {
        if(string != OMAF_NULL)
        {
            // Check null-termination
            OMAF_ASSERT(mData.getSize() > 0, "");
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
            
            size_t bufferLength = mData.getSize();
            size_t stringLength = StringByteLength(string);
            
            if(stringLength > 0)
            {
                mData.add(string, stringLength, bufferLength - 1);
            }
            
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
        }
    }
    
    void_t String::append(const utf8_t* string, size_t length)
    {
        if(string != OMAF_NULL && length > 0)
        {
            OMAF_ASSERT(mData.getSize() > 0, "");
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
            
            size_t bufferLength = mData.getSize();
            size_t stringByteLength = StringByteLength(string, length - 1);
            
            if(stringByteLength > 0)
            {
                mData.add(string, stringByteLength, bufferLength - 1);
            }
            
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
        }
    }
    
    void_t String::append(const String& string)
    {
        append(string.getData(), string.mData.getSize() - 1);
    }
    
    void_t String::appendFormat(const utf8_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        
        appendFormatVar(format, args);
        
        va_end(args);
    }
    
    void_t String::appendFormatVar(const utf8_t* format, va_list args)
    {
        size_t length = StringFormatLengthVar(format, args);
        
        if(length > 0)
        {
            // Check null-termination
            OMAF_ASSERT(mData.getSize() > 0, "");
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
            
            size_t oldSize = mData.getSize();
            
            mData.resize(length + oldSize);
            
            utf8_t* data = mData.getData();
            
            OMAF_ASSERT_NOT_NULL(data);
            OMAF_ASSERT(data + oldSize - 1 >= data, "");
            
            StringFormatVar(data + oldSize - 1, length + 1, format, args);
            
            OMAF_ASSERT(mData[mData.getSize() - 1] == Eos, "");
        }
    }

    String String::substring(size_t index, size_t length) const
    {
        OMAF_ASSERT(index <= getLength(), "");

        size_t stringLength = getLength();
        size_t charCount = (length == Npos || index + length > stringLength) ? stringLength - index : length;

        String string(mAllocator, charCount);

        if (charCount > 0)
        {
            string.append(at(index), charCount);
        }

        return string;
    }
    
    size_t String::findFirst(const utf8_t* string, size_t startIndex) const
    {
        return StringFindFirst(mData.getData(), string, startIndex);
    }
    
    size_t String::findLast(const utf8_t* string, size_t startIndex) const
    {
        if (StringLength(string) + 1 > mData.getSize())
        {
            return Npos;
        }
        if (startIndex == 0)
        {
            startIndex = mData.getSize() - StringLength(string) - 1;
        }
        return StringFindLast(mData.getData(), string, startIndex);
    }
    
    ComparisonResult::Enum String::compare(const utf8_t* string) const
    {
        return StringCompare(getData(), string);
    }
    
    ComparisonResult::Enum String::compare(const String& string) const
    {
        return StringCompare(getData(), string.getData());
    }
    
    String& String::operator = (const utf8_t* string)
    {
        if(string != OMAF_NULL)
        {
            mData.clear();
            
            size_t length = StringByteLength(string);
            mData.add(string, length + 1, 0);
        }
        
        return *this;
    }
    
    String& String::operator = (const String& string)
    {
        if (this == &string)
        {
            return *this;
        }
        mData.clear();
        mData.add(string.getData(), string.getSize(), 0);
        
        return *this;
    }
    
    bool_t String::operator == (const utf8_t* string) const
    {
        return (compare(string) == ComparisonResult::EQUAL);
    }
    
    bool_t String::operator == (const String& string) const
    {
        return (compare(string) == ComparisonResult::EQUAL);
    }
    
    bool_t String::operator != (const utf8_t* string) const
    {
        return (compare(string) != ComparisonResult::EQUAL);
    }
    
    bool_t String::operator != (const String& string) const
    {
        return (compare(string) != ComparisonResult::EQUAL);
    }

    String::operator const utf8_t* () const
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }

        return OMAF_NULL;
    }
OMAF_NS_END
