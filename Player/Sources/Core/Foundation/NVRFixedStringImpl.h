
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
#include "Foundation/NVRAssert.h"
#include "Foundation/NVRConstruct.h"
#include "Foundation/NVRMemoryUtilities.h"

OMAF_NS_BEGIN
    template <size_t N>
    FixedString<N>::FixedString()
    {
        mData.add(Eos);
    }
    
    template <size_t N>
    FixedString<N>::FixedString(const utf8_t* string)
    {
        if(string != OMAF_NULL)
        {
            size_t length = StringByteLength(string) + 1;
            mData.add(string, length, 0);
        }
        else
        {
            mData.add(Eos);
        }
    }
    
    template <size_t N>
    FixedString<N>::FixedString(const utf8_t* string, size_t length)
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
    
    template <size_t N>
    FixedString<N>::FixedString(const FixedString<N>& string)
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
    
    template <size_t N>
    FixedString<N>::~FixedString()
    {
    }
    
    template <size_t N>
    size_t FixedString<N>::getLength() const
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
    
    template <size_t N>
    size_t FixedString<N>::getSize() const
    {
        return mData.getSize();
    }
    
    template <size_t N>
    size_t FixedString<N>::getCapacity() const
    {
        return N;
    }
    
    template <size_t N>
    utf8_t* FixedString<N>::getData()
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }
        
        return OMAF_NULL;
    }
    
    template <size_t N>
    const utf8_t* FixedString<N>::getData() const
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }
        
        return OMAF_NULL;
    }
    
    template <size_t N>
    bool_t FixedString<N>::isEmpty() const
    {
        return (mData.getSize() <= 1) ? true : false;
    }
    
    template <size_t N>
    void_t FixedString<N>::clear()
    {
        mData.clear();

        mData.add(&Eos, 1, 0);
    }

    template <size_t N>
    void_t FixedString<N>::resize(size_t size)
    {
        mData.resize(size);
        mData.add(Eos);
    }

    template <size_t N>
    utf8_t* FixedString<N>::at(size_t index)
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < StringLength(mData.getData()), "");

        size_t offset = CharacterPosition(mData.getData(), index);

        return &mData[offset];
    }

    template <size_t N>
    const utf8_t* FixedString<N>::at(size_t index) const
    {
        OMAF_ASSERT(index >= 0, "");
        OMAF_ASSERT(index < StringLength(mData.getData()), "");

        size_t offset = CharacterPosition(mData.getData(), index);

        return &mData[offset];
    }
    
    template <size_t N>
    void_t FixedString<N>::append(const utf8_t* string)
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
    
    template <size_t N>
    void_t FixedString<N>::append(const utf8_t* string, size_t length)
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
    
    template <size_t N>
    void_t FixedString<N>::append(const FixedString<N>& string)
    {
        append(string.getData(), string.mData.getSize() - 1);
    }
    
    template <size_t N>
    void_t FixedString<N>::appendFormat(const utf8_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        
        appendFormatVar(format, args);
        
        va_end(args);
    }
    
    template <size_t N>
    void_t FixedString<N>::appendFormatVar(const utf8_t* format, va_list args)
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

    template <size_t N>
    FixedString<N> FixedString<N>::substring(size_t index, size_t length) const
    {
        OMAF_ASSERT(index <= getLength(), "");

        size_t stringLength = getLength();
        size_t charCount = (length == Npos || index + length > stringLength) ? stringLength - index : length;

        FixedString<N> string;

        if (charCount > 0)
        {
            string.append(at(index), charCount);
        }

        return string;
    }

    template <size_t N>
    size_t FixedString<N>::findFirst(const utf8_t* string, size_t startIndex) const
    {
        return StringFindFirst(mData.getData(), string, startIndex);
    }

    template <size_t N>
    size_t FixedString<N>::findLast(const utf8_t* string, size_t startIndex) const
    {
        return StringFindLast(mData.getData(), string, startIndex);
    }
    
    template <size_t N>
    ComparisonResult::Enum FixedString<N>::compare(const utf8_t* string) const
    {
        return StringCompare(getData(), string);
    }
    
    template <size_t N>
    ComparisonResult::Enum FixedString<N>::compare(const FixedString<N>& string) const
    {
        return StringCompare(getData(), string.getData());
    }
    
    template <size_t N>
    FixedString<N>& FixedString<N>::operator = (const utf8_t* string)
    {
        if(string != OMAF_NULL)
        {
            mData.clear();
            
            size_t length = StringByteLength(string);
            mData.add(string, length + 1, 0);
        }
        
        return *this;
    }
    
    template <size_t N>
    FixedString<N>& FixedString<N>::operator = (const FixedString<N>& string)
    {
        if (this == &string)
        {
            return *this;
        }
        mData.clear();
        mData.add(string.getData(), string.getSize(), 0);
        
        return *this;
    }
    
    template <size_t N>
    bool_t FixedString<N>::operator == (const utf8_t* string) const
    {
        return (compare(string) == ComparisonResult::EQUAL);
    }
    
    template <size_t N>
    bool_t FixedString<N>::operator == (const FixedString<N>& string) const
    {
        return (compare(string) == ComparisonResult::EQUAL);
    }
    
    template <size_t N>
    bool_t FixedString<N>::operator != (const utf8_t* string) const
    {
        return (compare(string) != ComparisonResult::EQUAL);
    }
    
    template <size_t N>
    bool_t FixedString<N>::operator != (const FixedString<N>& string) const
    {
       return (compare(string) != ComparisonResult::EQUAL); 
    }

    template <size_t N>
    FixedString<N>::operator const utf8_t* () const
    {
        if(mData.getSize() > 0)
        {
            return mData.getData();
        }

        return OMAF_NULL;
    }
OMAF_NS_END
