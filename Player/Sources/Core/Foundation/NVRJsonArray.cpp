
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
#include "Foundation/NVRJsonArray.h"

OMAF_NS_BEGIN
    JsonArray::JsonArray(MemoryAllocator& allocator, size_t capacity)
    : JsonObject(allocator, capacity)
    , mOpen(false)
    {
    }

    JsonArray::~JsonArray()
    {
    }

    bool_t JsonArray::append(const utf8_t* value)
    {
        if (StringLength(value) == 0)
        {
            return false;
        }
        else if (mData.isEmpty() || !mOpen)
        {
            mData.append("[");
            mOpen = true;
        }
        else if (mData.findLast("[") != mData.getLength()-1)
        {
            mData.append(",");
        }
        mData.append("\"");
        mData.append(value);
        mData.append("\"");
        return true;
    }

    bool_t JsonArray::append(int32_t value)
    {
        if (mData.isEmpty() || !mOpen)
        {
            mData.append("[");
            mOpen = true;
        }
        else if (mData.findLast("[") != mData.getLength() - 1)
        {
            mData.append(",");
        }
        mData.appendFormat("%d", value);

        return true;
    }

    bool_t JsonArray::append(int64_t value)
    {
        if (mData.isEmpty() || !mOpen)
        {
            mData.append("[");
            mOpen = true;
        }
        else if (mData.findLast("[") != mData.getLength() - 1)
        {
            mData.append(",");
        }
        mData.appendFormat("%lld", value);

        return true;
    }

    bool_t JsonArray::appendObject(const utf8_t* object)
    {
        if (mData.isEmpty() || !mOpen)
        {
            mData.append("[");
            mOpen = true;
        }
        else if (mData.findLast("[") != mData.getLength() - 1)
        {
            mData.append(",");
        }
        mData.append(object);

        return true;
    }

    const utf8_t* JsonArray::getData(bool_t lineBreak)
    {
        if (!mData.isEmpty())
        {
            mData.append("]");
            if (lineBreak)
            {
                mData.append("\n");
            }
        }
        return mData.getData();
    }

OMAF_NS_END
