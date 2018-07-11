
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
#include "Platform/OMAFDataTypes.h"

#include <NVRNamespace.h>
#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRPair.h"

#pragma once

OMAF_NS_BEGIN

typedef Pair<FixedString128, FixedString1024> HttpHeaderPair;
typedef FixedArray<HttpHeaderPair, 30> HttpHeaderPairList;

class HttpHeaderList
{
public:

    HttpHeaderList();

    HttpHeaderList(const HttpHeaderList& other);

    ~HttpHeaderList();

    void_t add(const char* key, const char* value);

    HttpHeaderPairList::ConstIterator begin() const;

    HttpHeaderPairList::ConstIterator end() const;

    void_t setByteRange(size_t start, size_t end);

    void_t clear();

    HttpHeaderList& operator = (const HttpHeaderList& other);

    bool_t operator == (const HttpHeaderList& other) const;

    bool_t operator != (const HttpHeaderList& other) const;

private:

    HttpHeaderPairList mHeaders;

};

OMAF_NS_END