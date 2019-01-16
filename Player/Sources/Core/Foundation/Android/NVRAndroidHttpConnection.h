
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

#include <android/native_window_jni.h>

#include "Foundation/NVRHttpConnection.h"
#include "Foundation/NVRThread.h"
#include "Foundation/NVRSpinlock.h"
#include "Foundation/NVREvent.h"

#define ANDROID_HTTP_DEFAULT_TIMEOUT 30000


OMAF_NS_BEGIN

    static const char* kHttpHeaderUserAgent = "User-Agent";

    namespace HTTPMethod
    {
        static const char* GET = "GET";
        static const char* HEAD = "HEAD";
        static const char* POST = "POST";
    }

    class AndroidHttpConnection : public HttpConnection
    {
    public:

        AndroidHttpConnection(MemoryAllocator& allocator);

        virtual ~AndroidHttpConnection();

        virtual void_t setUserAgent(const utf8_t* aUserAgent);

        virtual void_t setHttpDataProcessor(IHttpDataProcessor* aHttpDataProcessor);

        virtual const HttpRequestState& getState() const;

        virtual bool_t setUri(const Url& url);

        virtual void_t setHeaders(const HttpHeaderList& headers);

        virtual void_t setTimeout(uint32_t timeoutMS);

        virtual void_t setKeepAlive(uint32_t timeMS);
        // Async
        virtual HttpRequest::Enum get(DataBuffer<uint8_t>* output);
        // Async
        virtual HttpRequest::Enum post(const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output);
        // Async
        virtual HttpRequest::Enum head();
        // Async
        virtual void_t abortRequest();
        // Sync
        virtual void_t waitForCompletion();
        // Sync
        virtual bool_t hasCompleted();
    protected:

        Thread::ReturnValue threadEntry(const Thread& thread, void_t* userData);

    private:

        OMAF_NO_COPY(AndroidHttpConnection);
        OMAF_NO_ASSIGN(AndroidHttpConnection);

        HttpRequest::Enum start(const DataBuffer<uint8_t> *input, DataBuffer<uint8_t> *output, const char* method);

        jclass loadClass(const char* name);

        bool_t check();

        const HttpConnectionState::Enum getConnectionState() const;

        HttpConnectionState::Enum setConnectionState(HttpConnectionState::Enum state);

        void_t addToResponseHeaders(jstring jKey, jstring jValue);

        void_t deleteRef(jobject obj);

        bool_t doRequest();

        void_t checkRequestState();

    private:

        struct JniObjects {
            JNIEnv* jEnv;
            jstring urlString;
            jobject urlObject;
            jobject urlConnectionObject;
            jobject outputStreamObject;
            jbyteArray byteArray;
            jobject inputStreamObject;
            jstring methodString;
            JniObjects(JNIEnv* mJEnv);
            ~JniObjects();
        };
        MemoryAllocator& mAllocator;

        JNIEnv* mJEnv;

        // Java classes
        jclass urlClass;
        jclass inputStreamClass;
        jclass outputStreamClass;
        jclass httpsConnectionClass;

        // Java class methods
        jmethodID getInputStreamMethod;
        jmethodID getErrorStreamMethod;
        jmethodID getOutputStreamMethod;
        jmethodID connectMethod;
        jmethodID getDataLengthMethod;
        jmethodID setRequestMethod;
        jmethodID httpResponseCodeMethod;
        jmethodID setUseCachesMethod;
        jmethodID addRequestPropertyMethod;
        jmethodID setRequestPropertyMethod;
        jmethodID getHeaderFieldKeyMethod;
        jmethodID getHeaderFieldMethod;
        jmethodID setDoOutputMethod;
        jmethodID disconnectMethod;
        jmethodID setConnectTimeoutMethod;
        jmethodID setReadTimeoutMethod;
        jmethodID readMethod;
        jmethodID closeInputStreamMethod;
        jmethodID writeOutputStreamMethod;
        jmethodID flushOutpuStreamMethod;
        jmethodID closeOutputStreamMethod;
        jmethodID constructorMethod;
        jmethodID openConnectionMethod;

        Thread mThread;

        mutable Spinlock mMutex;

        mutable Event mEvent;

        Url mUrl;

        int32_t mTimeoutMs;

        DataBuffer<uint8_t>* mOutputBuffer;

        const DataBuffer<uint8_t>* mInputBuffer;

        HttpHeaderList mHeaders;

        HttpHeaderList mResponseHeaders;

        mutable HttpRequestState mHttpRequestState;

        mutable bool_t mHeadersChanged;

        HttpRequestState mInternalHttpRequestState;

        const char* mMethod;

        Event mStateChangeEvent;

        FixedString1024 mUserAgent;

        uint64_t mRequestStartTime;

        IHttpDataProcessor* mHttpDataProcessor;
    };
OMAF_NS_END
