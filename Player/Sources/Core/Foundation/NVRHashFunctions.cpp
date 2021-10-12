
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
#include "Foundation/NVRHashFunctions.h"

#include "Foundation/NVRAssert.h"
#include "Foundation/NVRFNVHash.h"

OMAF_NS_BEGIN
HashValue HashFunction<char_t*>::operator()(const char_t* key) const
{
    return (HashValue) FNVHash(key);
}

HashValue HashFunction<const char_t*>::operator()(const char_t* key) const
{
    return (HashValue) FNVHash(key);
}
OMAF_NS_END
