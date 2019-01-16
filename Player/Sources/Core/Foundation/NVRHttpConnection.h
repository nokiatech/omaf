
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
#pragma once

#include <NVRNamespace.h>
#include <OMAFPlayerDataTypes.h>

#include "Foundation/NVRFixedString.h"
#include "Foundation/NVRDataBuffer.h"
#include "Foundation/NVRPair.h"
#include "Foundation/NVRHttpHeaderList.h"

OMAF_NS_BEGIN

namespace HttpConnectionState
{
    enum Enum
    {
        INVALID = -1,

        IDLE,
        IN_PROGRESS,
        COMPLETED,// => IDLE
        FAILED,// => IDLE
        ABORTING,
        ABORTED,// => IDLE

        COUNT
    };
};

namespace HttpRequest
{
    enum Enum
    {
        INVALID = -1,

        OK,
        FAILED,

        COUNT
    };
};

typedef int64_t HttpResult;
typedef FixedString1024 Url;

struct HttpRequestState
{
    HttpConnectionState::Enum connectionState;
    HttpResult httpStatus;
    HttpHeaderList headers;
    uint64_t bytesDownloaded;
    uint64_t bytesUploaded;
    uint64_t totalBytes;
    const DataBuffer<uint8_t>* output; // Not own
    const DataBuffer<uint8_t>* input; // Not own

    HttpRequestState()
    : connectionState(HttpConnectionState::INVALID)
    , httpStatus(0)
    , headers()
    , bytesDownloaded(0)
    , bytesUploaded(0)
    , totalBytes(-1)
    , output(OMAF_NULL)
    , input(OMAF_NULL)
    {
    }
};

// http data post processor.
// The http download thread calls this before setting download state as completed
class IHttpDataProcessor
{
    public:
    virtual void_t processHttpData() = 0;
};

class HttpConnection
{
public:

    virtual ~HttpConnection() {};

    virtual const HttpRequestState& getState() const = 0;

    // per instance
    virtual void_t setUserAgent(const utf8_t* aUserAgent) = 0;
    // per instance
    virtual void_t setTimeout(uint32_t timeoutMS) = 0;

    // per instance
    virtual void_t setHttpDataProcessor(IHttpDataProcessor* aHttpDataProcessor) = 0;
    // per request
    virtual bool_t setUri(const Url& url) = 0;
    // per request
    virtual void_t setHeaders(const HttpHeaderList& headers) = 0;
    // per instance, if implemented
    virtual void_t setKeepAlive(uint32_t timeMS) = 0;
    // Async
    virtual HttpRequest::Enum get(DataBuffer<uint8_t>* output) = 0;
    // Async
    virtual HttpRequest::Enum post(const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output) = 0;
    // Async
    virtual HttpRequest::Enum head() = 0;
    // Async
    virtual void_t abortRequest() = 0;
    // Sync
    virtual void_t waitForCompletion() = 0;
    // Sync
    virtual bool_t hasCompleted() = 0;

};
OMAF_NS_END
