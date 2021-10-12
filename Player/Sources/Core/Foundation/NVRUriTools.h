
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
#pragma once

#include "Foundation/NVRString.h"
#include "NVREssentials.h"

OMAF_NS_BEGIN

class UriTools
{
public:
    static void_t escapeUri(const String& aInputString, String& aOutputString);

private:
    static bool_t isReserved(uint8_t aCharacter);
};

OMAF_NS_END
