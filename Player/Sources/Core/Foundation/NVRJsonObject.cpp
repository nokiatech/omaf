
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
#include "Foundation/NVRJsonObject.h"
#include "Foundation/NVRJsonArray.h"

OMAF_NS_BEGIN
JsonObject::JsonObject(MemoryAllocator& allocator, size_t capacity)
    : mData(allocator, capacity)
{
}

JsonObject::~JsonObject()
{
}

bool_t JsonObject::append(const utf8_t* key, const utf8_t* value)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":\"");
    mData.append(value);
    mData.append("\"");
    return true;
}

bool_t JsonObject::append(const utf8_t* key, int32_t value)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");

    mData.appendFormat("%d", value);

    return true;
}

bool_t JsonObject::append(const utf8_t* key, int64_t value)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");

    mData.appendFormat("%lld", value);

    return true;
}

bool_t JsonObject::append(const utf8_t* key, float32_t value)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");

    mData.appendFormat("%f", value);

    return true;
}

bool_t JsonObject::append(const utf8_t* key, uint64_t value)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");

    mData.appendFormat("%llu", value);

    return true;
}

bool_t JsonObject::appendObject(const utf8_t* key, const utf8_t* object)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");
    mData.append(object);
    return true;
}

bool_t JsonObject::appendArray(const utf8_t* key, const utf8_t* jsonArray)
{
    if (StringLength(key) == 0)
    {
        return false;
    }
    if (mData.isEmpty())
    {
        mData.append("{");
    }
    else
    {
        mData.append(",");
    }
    mData.append("\"");
    mData.append(key);
    mData.append("\":");
    mData.append(jsonArray);
    return true;
}

// Note! getData adds } to the end, so it cannot be called twice. Use peekData if you want to e.g. print the data to a
// log before actual get
const utf8_t* JsonObject::getData(bool_t lineBreak)
{
    if (!mData.isEmpty())
    {
        mData.append("}");
        if (lineBreak)
        {
            mData.append("\n");
        }
    }
    return mData.getData();
}

// returns the current data, which may not be a valid JSON (closing bracket may be missing)
const utf8_t* JsonObject::peekData()
{
    return mData.getData();
}

void_t JsonObject::clear()
{
    mData.clear();
}

size_t JsonObject::getLength()
{
    if (mData.isEmpty())
    {
        return 0;
    }
    return mData.getLength() + 1;
}
OMAF_NS_END
