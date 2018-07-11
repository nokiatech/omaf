
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
#include "Foundation/NVRStringUtilities.h"

#include "Foundation/NVRAssert.h"

OMAF_NS_BEGIN
    size_t StringLength(const utf8_t* string)
    {
        if(string == OMAF_NULL)
        {
            return 0;
        }
        
        size_t i = 0;
        size_t size = 0;
        
        while(string[i] != Eos)
        {
            if((string[i] & 0xc0) != 0x80)
            {
                size++;
            }
            
            i++;
        }
        
        return size;
    }
    
    size_t StringByteLength(const utf8_t* string)
    {
        if(string == OMAF_NULL)
        {
            return 0;
        }
        
        const utf8_t* end = string;
        
        while(*end != Eos)
        {
            ++end;
        }
        
        return (size_t)(end - string);
    }
    
    size_t StringByteLength(const utf8_t* string, size_t index)
    {
        if(string == OMAF_NULL)
        {
            return 0;
        }
        
        const utf8_t* end = string;
        
        size_t i = 0;
        
        while(*end != Eos)
        {
            if((*end & 0xc0) != 0x80)
            {
                if(i == index + 1)
                {
                    return (size_t)(end - string);
                }
                
                i++;
            }
            
            ++end;
        }
        
        return (size_t)(end - string);
    }
    
    size_t StringFormatVar(char_t* buffer, size_t bufferSize, const char_t* format, va_list args)
    {
        if(buffer == OMAF_NULL || bufferSize == 0 || format == OMAF_NULL)
        {
            return 0;
        }
        
        va_list copy;
        va_copy(copy, args);
        
        size_t result = ::vsnprintf(buffer, bufferSize, format, copy);
        
        va_end(copy);
        
        return result;
    }
    
    size_t StringFormatLengthVar(const char_t* format, va_list args)
    {
        if(format == OMAF_NULL)
        {
            return 0;
        }
        
        va_list copy;
        va_copy(copy, args);
        
        size_t result = ::vsnprintf(NULL, 0, format, copy);
        
        va_end(copy);
        
        return result;
    }
    
    size_t StringFormatLength(const char_t* format, ...)
    {
        va_list args;
        va_start(args, format);
        
        size_t result = StringFormatLengthVar(format, args);
        
        va_end(args);
        
        return result;
    }
    
    size_t CharacterPosition(const utf8_t* string, size_t index)
    {
        OMAF_ASSERT_NOT_NULL(string);
        OMAF_ASSERT(index < StringLength(string), "");
        
        size_t i = 0;
        size_t count = 0;
        
        while(string[i] != Eos)
        {
            if((string[i] & 0xc0) != 0x80)
            {
                if(count == index)
                {
                    return i;
                }
                
                count++;
            }
            
            i++;
        }
        
        return 0;
    }
    
    size_t CharacterPositionPrevious(const utf8_t* string, const utf8_t* beginning)
    {
        OMAF_ASSERT_NOT_NULL(string);
        OMAF_ASSERT_NOT_NULL(beginning);
        
        const utf8_t* current = string;
        
        while(--current >= beginning)
        {
            if((*current & 0xc0) != 0x80)
            {
                return (string - current);
            }
        }
        
        return 1;
    }
    
    size_t StringFindFirst(const utf8_t* source, const utf8_t* string, size_t startIndex)
    {
        if(source == NULL || string == NULL)
        {
            return Npos;
        }
        
        if(startIndex >= StringLength(source))
        {
            return Npos;
        }
        
        size_t length = StringLength(string);
        size_t offset = startIndex;
        
        const utf8_t* current = source + offset;
        
        size_t index = 0;
        
        while(*current != Eos)
        {
            if(StringCompare(current, string, length) == ComparisonResult::EQUAL)
            {
                return index;
            }
            
            current += 1;
            
            ++index;
        }
        
        return Npos;
    }
    
    size_t StringFindLast(const utf8_t* source, const utf8_t* string, size_t startIndex)
    {
        if(source == NULL || string == NULL)
        {
            return Npos;
        }
        
        size_t sourceLength = StringLength(source);
        size_t stringLength = StringLength(string);
        
        if((startIndex != Npos) && (startIndex + stringLength > sourceLength))
        {
            return Npos;
        }
        
        if(startIndex == Npos)
        {
            startIndex = sourceLength - stringLength;
        }
        
        if(sourceLength != 0)
        {
            size_t index = (startIndex >= sourceLength) ? sourceLength : (startIndex + 1);
            size_t startOffset = startIndex;
            
            const utf8_t* current = source + startOffset;
            const utf8_t* beginning = source;
            
            while(index != 0)
            {
                --index;
                
                if(StringCompare(current, string, stringLength) == ComparisonResult::EQUAL)
                {
                    return index;
                }
                
                size_t previousOffset = CharacterPositionPrevious(current, beginning);
                current -= previousOffset;
            }
        }
        
        return Npos;
    }
    
    ComparisonResult::Enum StringCompare(const utf8_t* l, const utf8_t* r)
    {
        if(l == OMAF_NULL || r == OMAF_NULL)
        {
            return ComparisonResult::INVALID;
        }
        
        size_t lengthL = StringLength(l);
        size_t lengthR = StringLength(r);
        
        if(lengthL < lengthR)
        {
            return ComparisonResult::LESS;
        }
        else if(lengthL > lengthR)
        {
            return ComparisonResult::GREATER;
        }
        
        while(*l != Eos && *r != Eos)
        {
            utf8_t charL = *l;
            utf8_t charR = *r;
            
            int32_t charDiff = charL - charR;
            
            if(charDiff < 0)
            {
                return ComparisonResult::LESS; // left is smaller than right
            }
            else if (charDiff > 0)
            {
                return ComparisonResult::GREATER; // left is greater than right
            }
            
            ++l;
            ++r;
        }
        
        return ComparisonResult::EQUAL;
    }
    
    ComparisonResult::Enum StringCompare(const utf8_t* l, const utf8_t* r, size_t length)
    {
        if(l == OMAF_NULL || r == OMAF_NULL)
        {
            return ComparisonResult::INVALID;
        }
        
        if(length == 0)
        {
            return ComparisonResult::EQUAL;
        }
        
        size_t count = 0;
        
        while(*l != Eos && *r != Eos)
        {
            utf8_t char0 = *l;
            utf8_t char1 = (uint32_t)*r;
            
            int32_t charDiff = char0 - char1;
            
            if(charDiff < 0)
            {
                return ComparisonResult::LESS;  // left is smaller than right
            }
            else if (charDiff > 0)
            {
                return ComparisonResult::GREATER; // left is greater than right
            }
            
            if((*l & 0xc0) != 0x80)
            {
                count++;
                
                if(count == length)
                {
                    return ComparisonResult::EQUAL;
                }
            }
            
            ++l;
            ++r;
        }
        
        if (*l != Eos)
        {
            return ComparisonResult::GREATER;
        }
        else if (*r != Eos)
        {
            return ComparisonResult::LESS;
        }
        else
        {
            OMAF_ASSERT_UNREACHABLE();
        }
        
        OMAF_ASSERT_UNREACHABLE();
        
        return ComparisonResult::INVALID;
    }
OMAF_NS_END
