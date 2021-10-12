
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

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRJsonObject.h"
#include "Foundation/NVRString.h"

OMAF_NS_BEGIN

/*
 * A very basic class for constructing JSON array. E.g. no editing possible, only adding, and only a limited set of data
 * types supported
 */
class JsonArray : public JsonObject
{
public:
    JsonArray(MemoryAllocator& allocator, size_t capacity = 100);
    virtual ~JsonArray();

    bool_t append(const utf8_t* value);
    bool_t append(int32_t value);
    bool_t append(int64_t value);
    bool_t appendObject(const utf8_t* object);

    const utf8_t* getData(bool_t lineBreak = false);

private:
    bool_t mOpen;
};

OMAF_NS_END
