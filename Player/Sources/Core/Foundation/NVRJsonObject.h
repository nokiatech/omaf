
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
#pragma once

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRString.h"

OMAF_NS_BEGIN
    /*
     * A very basic class for constructing JSON object. E.g. no editing possible, only adding, and only a limited set of data types supported
     */
    class JsonObject
    {
    public:
        JsonObject(MemoryAllocator& allocator, size_t capacity = 100);
        virtual ~JsonObject();

        bool_t append(const utf8_t* key, const utf8_t* value);
        bool_t append(const utf8_t* key, int32_t value);
        bool_t append(const utf8_t* key, int64_t value);
        bool_t append(const utf8_t* key, uint64_t value);
        bool_t append(const utf8_t* key, float32_t value);
        bool_t appendObject(const utf8_t* key, const utf8_t* object);
        bool_t appendArray(const utf8_t* key, const utf8_t* jsonArray);

        // Note! getData adds } to the end, so it cannot be called twice. Use peekData if you want to e.g. print the data to a log before actual get
        const utf8_t* getData(bool_t lineBreak = false);
        const utf8_t* peekData();
        virtual void_t clear();
        virtual size_t getLength();

    protected:
        
        String mData;
    };
OMAF_NS_END
