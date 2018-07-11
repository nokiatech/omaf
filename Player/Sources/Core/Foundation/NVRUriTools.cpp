
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
#include "Foundation/NVRUriTools.h"
OMAF_NS_BEGIN


bool_t UriTools::isReserved(uint8_t character)
{
    if (character >= '0' && character <= '9')
    {
        return false;
    }
    else if (character >= 'a' && character <= 'z')
    {
        return false;
    }
    else if (character >= 'A' && character <= 'Z')
    {
        return false;
    }
    else if (character == '-' || character == '.' || character == '~' || character == '_')
    {
        return false;
    }

    return true;
}

void_t UriTools::escapeUri(const String& aInputString, String& aOutputString)
{
    aOutputString.clear();
    size_t length = aInputString.getLength();

    uint8_t inputChar = 0;
    size_t inputIndex = 0;
    size_t lastInputIndex = 0;
    while (length--)
    {
        inputChar = *aInputString.at(inputIndex);
        if (!isReserved(inputChar))
        {
            inputIndex++;
        }
        else
        {
            aOutputString.append(aInputString.substring(lastInputIndex, inputIndex - lastInputIndex));
            inputIndex++;
            lastInputIndex = inputIndex;
            aOutputString.appendFormat("%%%02X", inputChar);
        }
    }
}

OMAF_NS_END