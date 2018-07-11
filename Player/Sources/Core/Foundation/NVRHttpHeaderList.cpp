
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
#include "Foundation/NVRHttpHeaderList.h"

OMAF_NS_BEGIN

    HttpHeaderList::HttpHeaderList()
    :mHeaders()
    {
    };

    HttpHeaderList::HttpHeaderList(const HttpHeaderList& other)
    :mHeaders()
    {
        mHeaders = other.mHeaders;
    }

    HttpHeaderList::~HttpHeaderList(){};

    void_t HttpHeaderList::add(const char* key, const char* value)
    {
        HttpHeaderPair header;
        header.first.append(key);
        header.second.appendFormat("%s", value);
        mHeaders.add(header);
    };

    HttpHeaderPairList::ConstIterator HttpHeaderList::begin() const
    {
        return mHeaders.begin();
    };

    HttpHeaderPairList::ConstIterator HttpHeaderList::end() const
    {
        return mHeaders.end();
    };

    void_t HttpHeaderList::setByteRange(size_t start, size_t end)
    {
        HttpHeaderPair header;
        header.first.append("Range");
        header.second.appendFormat("bytes=%u-%u", start, end);
        mHeaders.add(header);
    };

    void_t HttpHeaderList::clear()
    {
        mHeaders.clear();
    };

    HttpHeaderList& HttpHeaderList::operator = (const HttpHeaderList& other)
    {
        mHeaders = other.mHeaders;
        return *this;
    }

    bool_t HttpHeaderList::operator == (const HttpHeaderList& other) const
    {
        return mHeaders == other.mHeaders;
    }

    bool_t HttpHeaderList::operator != (const HttpHeaderList& other) const
    {
        return mHeaders != other.mHeaders;
    }


OMAF_NS_END
