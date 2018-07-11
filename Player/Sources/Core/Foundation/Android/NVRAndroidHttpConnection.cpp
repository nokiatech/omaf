
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
#include "Foundation/Android/NVRAndroid.h"

#include "NVRAndroidHttpConnection.h"
#include "Foundation/NVRLogger.h"
#include "Foundation/NVRBandwidthMonitor.h"
#include "Foundation/NVRTime.h"

OMAF_NS_BEGIN
    OMAF_LOG_ZONE(AndroidHttpConnection)

    AndroidHttpConnection::JniObjects::JniObjects(JNIEnv* env)
    : jEnv(env)
    , urlString(NULL)
    , urlObject(NULL)
    , urlConnectionObject(NULL)
    , outputStreamObject(NULL)
    , byteArray(NULL)
    , inputStreamObject(NULL)
    , methodString(NULL)
    {
    }

    AndroidHttpConnection::JniObjects::~JniObjects()
    {
        if (urlString != NULL)
        {
            jEnv->DeleteLocalRef(urlString);
        }
        if (urlObject != NULL)
        {
            jEnv->DeleteLocalRef(urlObject);
        }
        if (urlConnectionObject != NULL)
        {
            jEnv->DeleteLocalRef(urlConnectionObject);
        }
        if (outputStreamObject != NULL)
        {
            jEnv->DeleteLocalRef(outputStreamObject);
        }
        if (byteArray != NULL)
        {
            jEnv->DeleteLocalRef(byteArray);
        }
        if (inputStreamObject != NULL)
        {
            jEnv->DeleteLocalRef(inputStreamObject);
        }
        if (methodString != NULL)
        {
            jEnv->DeleteLocalRef(methodString);
        }
    }

    AndroidHttpConnection::AndroidHttpConnection(MemoryAllocator& allocator)
    : mAllocator(allocator)
    , mThread()
    , mMutex()
    , mEvent(false, false)
    , mUrl()
    , mTimeoutMs(ANDROID_HTTP_DEFAULT_TIMEOUT)
    , mOutputBuffer(OMAF_NULL)
    , mInputBuffer(OMAF_NULL)
    , mHeaders()
    , mResponseHeaders()
    , mHttpRequestState()
    , mInternalHttpRequestState()
    , mMethod(HTTPMethod::GET)
    , mStateChangeEvent(false,false)
    , mHeadersChanged(false)
    , mUserAgent()
    , mRequestStartTime(0)
    , mHttpDataProcessor(OMAF_NULL)
    {
        Thread::EntryFunction function;
        function.bind<AndroidHttpConnection, &AndroidHttpConnection::threadEntry>(this);

        mThread.setPriority(Thread::Priority::INHERIT);
        mThread.setName("HTTP::AndroidHttpConnection");

        mThread.start(function);
    }

    AndroidHttpConnection::~AndroidHttpConnection()
    {
        abortRequest();
        waitForCompletion();

        mMutex.lock();
        mInternalHttpRequestState.connectionState = HttpConnectionState::INVALID;
        mMutex.unlock();

        if (mThread.isValid() && mThread.isRunning())
        {
            mThread.stop();
            mEvent.reset();
            mEvent.signal();
            mThread.join();
        }
    }

    void_t AndroidHttpConnection::waitForCompletion()
    {
        //TODO: there must be a better way!
        //wait for it...
        for (;;)
        {
            if (hasCompleted())
            {
                break;
            }
            mStateChangeEvent.wait(5000);
        }
    }
    bool_t AndroidHttpConnection::hasCompleted()
    {
        HttpConnectionState::Enum state = getConnectionState();
        if (state == HttpConnectionState::ABORTED || state == HttpConnectionState::COMPLETED ||
            state == HttpConnectionState::FAILED || state == HttpConnectionState::IDLE ||
            state == HttpConnectionState::INVALID)
        {
            return true;
        }
        return false;
    }

    const HttpRequestState &AndroidHttpConnection::getState() const
    {
        Spinlock::ScopeLock lock(mMutex);
        mHttpRequestState.connectionState = mInternalHttpRequestState.connectionState;
        mHttpRequestState.httpStatus = mInternalHttpRequestState.httpStatus;

        if (mHeadersChanged)
        {
            mHttpRequestState.headers = mInternalHttpRequestState.headers;
            mHeadersChanged = false;
        }

        mHttpRequestState.bytesDownloaded = mInternalHttpRequestState.bytesDownloaded;
        mHttpRequestState.bytesUploaded = mInternalHttpRequestState.bytesUploaded;
        mHttpRequestState.totalBytes = mInternalHttpRequestState.totalBytes;
        mHttpRequestState.input = mInternalHttpRequestState.input;
        mHttpRequestState.output = mInternalHttpRequestState.output;

        return mHttpRequestState;
    }

    const HttpConnectionState::Enum AndroidHttpConnection::getConnectionState() const
    {
        Spinlock::ScopeLock lock(mMutex);
        return mInternalHttpRequestState.connectionState;
    }

    HttpConnectionState::Enum AndroidHttpConnection::setConnectionState(HttpConnectionState::Enum state)
    {
        Spinlock::ScopeLock lock(mMutex);
        mInternalHttpRequestState.connectionState = state;
        mStateChangeEvent.signal();
    }

    void_t AndroidHttpConnection::checkRequestState()
    {
        if (mInternalHttpRequestState.connectionState != HttpConnectionState::IDLE)
        {
            mInternalHttpRequestState = HttpRequestState();
            mInternalHttpRequestState.connectionState = HttpConnectionState::IDLE;

            mHeadersChanged = true;
        }
    }

    bool_t AndroidHttpConnection::setUri(const Url &url)
    {
        if (url.isEmpty())
        {
            return false;
        }

        Spinlock::ScopeLock lock(mMutex);

        if (mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS ||
            mInternalHttpRequestState.connectionState == HttpConnectionState::ABORTING)
        {
            // not possible to change while in progress
            return false; // error
        }

        checkRequestState();

        mUrl = url;

        mStateChangeEvent.signal();
        return true;
    }

    void_t AndroidHttpConnection::setHeaders(const HttpHeaderList &headers)
    {
        Spinlock::ScopeLock lock(mMutex);

        if ((mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS) ||
            (mInternalHttpRequestState.connectionState == HttpConnectionState::ABORTING))
        {
            // not possible to change while in progress
            return;
        }

        mHeaders = headers;
    }

    void_t AndroidHttpConnection::setTimeout(uint32_t timeoutMS)
    {
        Spinlock::ScopeLock lock(mMutex);

        if ((mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS) ||
            (mInternalHttpRequestState.connectionState == HttpConnectionState::ABORTING))
        {
            // not possible to change while in progress
            return;
        }

        mTimeoutMs = timeoutMS;
    }

    void_t AndroidHttpConnection::setKeepAlive(uint32_t timeMS)
    {
        // Android takes care of this
    }

    HttpRequest::Enum AndroidHttpConnection::get(DataBuffer<uint8_t> *output)
    {
        return start(OMAF_NULL, output, HTTPMethod::GET);
    }

    HttpRequest::Enum AndroidHttpConnection::post(const DataBuffer<uint8_t> *input, DataBuffer<uint8_t> *output)
    {
        return start(input, output, HTTPMethod::POST);
    }

    HttpRequest::Enum AndroidHttpConnection::head()
    {
        return start(OMAF_NULL, OMAF_NULL, HTTPMethod::HEAD);
    }

    void_t AndroidHttpConnection::abortRequest()
    {
        Spinlock::ScopeLock lock(mMutex);

        if (mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS)
        {
            mInternalHttpRequestState.connectionState = HttpConnectionState::ABORTING;
            mStateChangeEvent.signal();
        }
    }

    HttpRequest::Enum AndroidHttpConnection::start(const DataBuffer<uint8_t> *input, DataBuffer<uint8_t> *output, const char* method)
    {
        if (mUrl.isEmpty() || mInternalHttpRequestState.connectionState != HttpConnectionState::IDLE || !mThread.isValid() || !mThread.isRunning() || mThread.shouldStop())
        {
            return HttpRequest::FAILED;
        }

        OMAF_LOG_D("AndroidHttpConnection::start %s", mUrl.getData());

        mMutex.lock();

        mMethod = method;

        mRequestStartTime = Time::getClockTimeUs();
        mInternalHttpRequestState.connectionState = HttpConnectionState::IN_PROGRESS;

        mStateChangeEvent.signal();

        mOutputBuffer = output;

        mInputBuffer = input;

        mMutex.unlock();

        mEvent.reset();

        mEvent.signal();

        return HttpRequest::OK;
    }

    jclass AndroidHttpConnection::loadClass(const char* name)
    {
        const jclass newClass = mJEnv->FindClass(name);

        if (newClass == 0)
        {
            OMAF_ASSERT(false, "Class cannot be found");
            OMAF_LOG_E("Unable to load java class: %s", name);
        }

        return newClass;
    }

    Thread::ReturnValue AndroidHttpConnection::threadEntry(const Thread& thread, void_t* userData)
    {

        // get jEnv inside the thread
        mJEnv = OMAF::Private::Android::getJNIEnv();

        if (mJEnv == 0)
        {
            OMAF_LOG_E("No Java ENV. Aborting!");
            mThread.stop();
            return 0;
        }

        // Init JNI classes and menthods
        urlClass = loadClass("java/net/URL");

        inputStreamClass = loadClass("java/io/InputStream");
        outputStreamClass = loadClass("java/io/OutputStream");
        httpsConnectionClass = loadClass("javax/net/ssl/HttpsURLConnection");

        if (inputStreamClass == 0 || outputStreamClass == 0|| httpsConnectionClass == 0)
        {
            mThread.stop();
            return 0;
        }

        getInputStreamMethod = mJEnv->GetMethodID(httpsConnectionClass, "getInputStream", "()Ljava/io/InputStream;");
        getErrorStreamMethod = mJEnv->GetMethodID(httpsConnectionClass, "getErrorStream", "()Ljava/io/InputStream;");
        getOutputStreamMethod = mJEnv->GetMethodID(httpsConnectionClass, "getOutputStream", "()Ljava/io/OutputStream;");

        connectMethod = mJEnv->GetMethodID(httpsConnectionClass, "connect", "()V");
        getDataLengthMethod = mJEnv->GetMethodID(httpsConnectionClass, "getContentLength", "()I");
        setRequestMethod = mJEnv->GetMethodID(httpsConnectionClass, "setRequestMethod", "(Ljava/lang/String;)V");
        httpResponseCodeMethod = mJEnv->GetMethodID(httpsConnectionClass, "getResponseCode", "()I");
        setUseCachesMethod = mJEnv->GetMethodID(httpsConnectionClass, "setUseCaches", "(Z)V");
        addRequestPropertyMethod = mJEnv->GetMethodID(httpsConnectionClass, "addRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");
        setRequestPropertyMethod = mJEnv->GetMethodID(httpsConnectionClass, "setRequestProperty", "(Ljava/lang/String;Ljava/lang/String;)V");

        getHeaderFieldKeyMethod = mJEnv->GetMethodID(httpsConnectionClass, "getHeaderFieldKey", "(I)Ljava/lang/String;");
        getHeaderFieldMethod = mJEnv->GetMethodID(httpsConnectionClass, "getHeaderField", "(I)Ljava/lang/String;");
        setDoOutputMethod = mJEnv->GetMethodID(httpsConnectionClass, "setDoOutput", "(Z)V");
        disconnectMethod = mJEnv->GetMethodID(httpsConnectionClass, "disconnect", "()V");
        setConnectTimeoutMethod = mJEnv->GetMethodID(httpsConnectionClass, "setConnectTimeout", "(I)V");
        setReadTimeoutMethod = mJEnv->GetMethodID(httpsConnectionClass, "setReadTimeout", "(I)V");


        readMethod = mJEnv->GetMethodID(inputStreamClass, "read", "([BII)I");
        closeInputStreamMethod = mJEnv->GetMethodID(inputStreamClass, "close", "()V");

        writeOutputStreamMethod = mJEnv->GetMethodID(outputStreamClass, "write", "([BII)V");
        flushOutpuStreamMethod = mJEnv->GetMethodID(outputStreamClass, "flush", "()V");
        closeOutputStreamMethod = mJEnv->GetMethodID(outputStreamClass, "close", "()V");

        constructorMethod = mJEnv->GetMethodID(urlClass, "<init>", "(Ljava/lang/String;)V");
        openConnectionMethod = mJEnv->GetMethodID(urlClass, "openConnection", "()Ljava/net/URLConnection;");

        while (mThread.isRunning() && !mThread.shouldStop())
        {
            if (getConnectionState() == HttpConnectionState::ABORTING)
            {
                setConnectionState(HttpConnectionState::ABORTED);
            }

            if (getConnectionState() != HttpConnectionState::IN_PROGRESS)
            {
                mEvent.wait();
            }

            if (getConnectionState() != HttpConnectionState::IN_PROGRESS)
            {
                continue;
            }

            bool_t result = doRequest();

            mMutex.lock();

            mUrl.clear();

            mMethod = HTTPMethod::GET;

            mHeaders.clear();

            mInternalHttpRequestState.output = mOutputBuffer;
            mInternalHttpRequestState.input = mInputBuffer;

            mOutputBuffer = OMAF_NULL;
            mInputBuffer = OMAF_NULL;

            if (result && mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS)
            {
                BandwidthMonitor::notifyDownloadCompleted((Time::getClockTimeUs() - mRequestStartTime) / 1000, mInternalHttpRequestState.bytesDownloaded);
                if (mHttpDataProcessor != OMAF_NULL)
                {
                    mMutex.unlock();
                    mHttpDataProcessor->processHttpData();
                    mMutex.lock();
                }
                mInternalHttpRequestState.connectionState = HttpConnectionState::COMPLETED;
            }
            else if (mInternalHttpRequestState.connectionState == HttpConnectionState::ABORTING)
            {
                mInternalHttpRequestState.connectionState = HttpConnectionState::ABORTED;
            }
            else
            {
                mInternalHttpRequestState.connectionState = HttpConnectionState::FAILED;
            }

            mStateChangeEvent.signal();

            mMutex.unlock();
        }

        return 0;
    }

    bool_t AndroidHttpConnection::doRequest()
    {
        static const jboolean doCache = 0;

        static const jboolean doOutput = 1;

        JniObjects jo(mJEnv);

        jo.urlString = mJEnv->NewStringUTF(mUrl.getData());

        if (!check())
        {
            return false;
        }

        jo.urlObject = mJEnv->NewObject(urlClass, constructorMethod, jo.urlString);

        if (!check())
        {
            return false;
        }

        jo.urlConnectionObject = mJEnv->CallObjectMethod(jo.urlObject, openConnectionMethod);

        if (!check())
        {
            return false;
        }

        // SSL properties? https.setHostnameVerifier(DO_NOT_VERIFY);

        HttpHeaderPairList::ConstIterator it = mHeaders.begin();

        for (; it != mHeaders.end() ; ++it)
        {
            HttpHeaderPair header = (*it);
            jstring key = mJEnv->NewStringUTF(header.first.getData());

            if (!check() || key == NULL)
            {
                deleteRef(key);
                return false;
            }

            jstring value = mJEnv->NewStringUTF(header.second.getData());

            if (!check() || value == NULL)
            {
                deleteRef(key);
                deleteRef(value);
                return false;
            }

            mJEnv->CallVoidMethod(jo.urlConnectionObject, addRequestPropertyMethod, key, value);

            bool_t success = check();

            deleteRef(key);
            deleteRef(value);

            if (!success)
            {
                OMAF_ASSERT(false, "Exception was thrown!!");
                OMAF_LOG_E("Aborting, unable to set HTTP headers!");
                return false;
            }
        }

        if (!mUserAgent.isEmpty())
        {
            jstring key = mJEnv->NewStringUTF(kHttpHeaderUserAgent);

            if (!check() || key == NULL)
            {
                deleteRef(key);
                return false;
            }

            jstring value = mJEnv->NewStringUTF(mUserAgent.getData());

            if (!check() || value == NULL)
            {
                deleteRef(key);
                deleteRef(value);
                return false;
            }

            mJEnv->CallVoidMethod(jo.urlConnectionObject, setRequestPropertyMethod, key, value);

            bool_t success = check();

            deleteRef(key);
            deleteRef(value);

            if (!success)
            {
                OMAF_ASSERT(false, "Exception was thrown!!");
                OMAF_LOG_E("Aborting, unable to set HTTP User Agent header!");
                return false;
            }
        }

        mJEnv->CallVoidMethod(jo.urlConnectionObject, setUseCachesMethod, doCache);

        if (!check())
        {
            return false;
        }

        jo.methodString = mJEnv->NewStringUTF(mMethod);

        if (!check() || jo.methodString == NULL)
        {
            return false;
        }

        mJEnv->CallVoidMethod(jo.urlConnectionObject, setRequestMethod, jo.methodString);

        if (!check())
        {
            return false;
        }

        // DO POST
        if (mInputBuffer != OMAF_NULL && mInputBuffer->getSize() > 0)
        {
            const jsize SIZE = mInputBuffer->getSize();

            mJEnv->CallVoidMethod(jo.urlConnectionObject, setDoOutputMethod, doOutput);

            if (!check())
            {
                return false;
            }

            jo.byteArray = mJEnv->NewByteArray(SIZE);

            if (!check())
            {
                return false;
            }

            jboolean isCopy = 0;

            jbyte *javaPrimitiveArray = (jbyte *) mJEnv->GetPrimitiveArrayCritical(jo.byteArray, &isCopy);

            /* We need to check in case the VM tried to make a copy. */
            if (javaPrimitiveArray == NULL || isCopy)
            {
                // ERROR
                OMAF_ASSERT_UNREACHABLE();
                setConnectionState(HttpConnectionState::FAILED);
                return false;
            }

            memcpy(javaPrimitiveArray, mInputBuffer->getDataPtr(), SIZE);

            mJEnv->ReleasePrimitiveArrayCritical(jo.byteArray, javaPrimitiveArray, JNI_ABORT);

            jo.outputStreamObject = mJEnv->CallObjectMethod(jo.urlConnectionObject, getOutputStreamMethod);

            if (!check())
            {
                return false;
            }

            mJEnv->CallVoidMethod(jo.outputStreamObject, writeOutputStreamMethod, jo.byteArray, 0, mInputBuffer->getSize());

            if (!check())
            {
                return false;
            }

            mJEnv->CallVoidMethod(jo.outputStreamObject, flushOutpuStreamMethod);

            if (!check())
            {
                return false;
            }

            mJEnv->CallVoidMethod(jo.outputStreamObject, closeOutputStreamMethod);

            if (!check())
            {
                return false;
            }

            deleteRef(jo.byteArray);

            jo.byteArray = NULL;

            mMutex.lock();
            mInternalHttpRequestState.bytesUploaded = SIZE;
            mMutex.unlock();
        }

        mMutex.lock();
        jint timeOut = mTimeoutMs;
        mMutex.unlock();

        mJEnv->CallVoidMethod(jo.urlConnectionObject, setConnectTimeoutMethod, timeOut);

        if (!check())
        {
            return false;
        }

        mJEnv->CallVoidMethod(jo.urlConnectionObject, setReadTimeoutMethod, timeOut);

        if (!check())
        {
            return false;
        }

        mJEnv->CallVoidMethod(jo.urlConnectionObject, connectMethod);

        if (!check())
        {
            return false;
        }

        jint httpResponse = mJEnv->CallIntMethod(jo.urlConnectionObject, httpResponseCodeMethod);

        if (!check())
        {
            return false;
        }

        mMutex.lock();
        mInternalHttpRequestState.httpStatus = httpResponse;
        mMutex.unlock();

        jint contentLength = mJEnv->CallIntMethod(jo.urlConnectionObject, getDataLengthMethod);

        if (!check())
        {
            return false;
        }

        if (contentLength > 0)
        {
            mMutex.lock();
            mInternalHttpRequestState.totalBytes = static_cast<size_t>(contentLength);
            mMutex.unlock();
        }

        mResponseHeaders.clear();


        for (jint i = 0;; ++i)
        {
            jstring jHeaderKey = (jstring)mJEnv->CallObjectMethod(jo.urlConnectionObject, getHeaderFieldKeyMethod, i);

            if (!check())
            {
                OMAF_ASSERT(false, "Exception was thrown!!");
                return false;
            }

            jstring jHeaderValue = (jstring)mJEnv->CallObjectMethod(jo.urlConnectionObject, getHeaderFieldMethod, i);

            if (!check())
            {
                deleteRef(jHeaderKey);
                OMAF_ASSERT(false, "Exception was thrown!!");
                return false;
            }

            if (jHeaderKey == NULL || jHeaderValue == NULL)
            {
                break;
            }

            addToResponseHeaders(jHeaderKey, jHeaderValue);

            deleteRef(jHeaderKey);
            deleteRef(jHeaderValue);
        }

        mMutex.lock();
        mInternalHttpRequestState.headers = mResponseHeaders;
        mHeadersChanged = true;
        mMutex.unlock();

        if (mOutputBuffer != OMAF_NULL)
        {

            if (httpResponse >= 200 && httpResponse < 300)
            {
                // get content stream
                jo.inputStreamObject = mJEnv->CallObjectMethod(jo.urlConnectionObject, getInputStreamMethod);
            }
            else
            {
                // get error stream
                jo.inputStreamObject = mJEnv->CallObjectMethod(jo.urlConnectionObject, getErrorStreamMethod);
            }

            if (!check())
            {
                return false;
            }

            const jsize SIZE = 1024 * 10;

            size_t totalRead = 0;

            mOutputBuffer->clear();

            size_t targetCapacity;

            if (contentLength < 0)
            {
                targetCapacity = mOutputBuffer->getCapacity() > 0 ? mOutputBuffer->getCapacity() : 1024 * 10;
            }
            else
            {
                targetCapacity = static_cast<size_t>(mInternalHttpRequestState.totalBytes);
            }

            if (mOutputBuffer->getCapacity() < targetCapacity)
            {
                mOutputBuffer->reAllocate(targetCapacity);
            }

            OMAF_ASSERT(jo.byteArray == NULL, "Byte Array must be null here!");

            jo.byteArray = mJEnv->NewByteArray(SIZE);

            while (getConnectionState() == HttpConnectionState::IN_PROGRESS)
            {
                jint read = mJEnv->CallIntMethod(jo.inputStreamObject, readMethod, jo.byteArray, 0, SIZE);

                if (!check())
                {
                    return false;
                }

                if (read < 0)
                {
                    // No more data to read
                    break;
                }
                else
                {
                    if (totalRead + read > mOutputBuffer->getCapacity())
                    {
                        size_t targetSize = mOutputBuffer->getCapacity() * 2;

                        if (targetSize < totalRead + read)
                        {
                            targetSize = totalRead + read;
                        }

                        mOutputBuffer->reserve(targetSize);
                    }

                    jboolean isCopy = 0;

                    jbyte *javaPrimitiveArray = (jbyte *) mJEnv->GetPrimitiveArrayCritical(jo.byteArray, &isCopy);

                    /* We need to check in case the VM tried to make a copy. */
                    if (javaPrimitiveArray == NULL || isCopy)
                    {
                        // ERROR
                        OMAF_ASSERT_UNREACHABLE();
                        setConnectionState(HttpConnectionState::FAILED);
                        return false;
                    }

                    memcpy(mOutputBuffer->getDataPtr() + totalRead, javaPrimitiveArray, read);

                    mJEnv->ReleasePrimitiveArrayCritical(jo.byteArray, javaPrimitiveArray, JNI_ABORT);

                    if (!check())
                    {
                        return false;
                    }

                    totalRead += read;

                    mOutputBuffer->setSize(totalRead);

                }

            }

            mJEnv->CallVoidMethod(jo.inputStreamObject, closeInputStreamMethod);

            if (!check())
            {
                return false;
            }

            mMutex.lock();
            mInternalHttpRequestState.bytesDownloaded = totalRead;
            mMutex.unlock();
        }
//        else no content, must be a head request

        mJEnv->CallVoidMethod(jo.urlConnectionObject, disconnectMethod);

        return check();
    }

    bool_t AndroidHttpConnection::check()
    {
        bool_t flag = mJEnv->ExceptionCheck();

        if (flag)
        {
            Spinlock::ScopeLock lock(mMutex);

            if (mInternalHttpRequestState.connectionState != HttpConnectionState::ABORTING)
            {
                mJEnv->ExceptionDescribe();
            }

            mJEnv->ExceptionClear();
        }

        return !flag;
    }

    void_t AndroidHttpConnection::addToResponseHeaders(jstring jKey, jstring jValue)
    {
        const char *k = mJEnv->GetStringUTFChars(jKey, JNI_FALSE);
        const char *v = mJEnv->GetStringUTFChars(jValue, JNI_FALSE);

        mResponseHeaders.add(k, v);

        mJEnv->ReleaseStringUTFChars(jValue, v);
        mJEnv->ReleaseStringUTFChars(jKey, k);
    }

    void_t AndroidHttpConnection::deleteRef(jobject obj)
    {
        if (obj != NULL && mJEnv != NULL)
        {
            mJEnv->DeleteLocalRef(obj);
        }
    }

    void_t AndroidHttpConnection::setUserAgent(const utf8_t *aUserAgent)
    {
        Spinlock::ScopeLock lock(mMutex);

        if ((mInternalHttpRequestState.connectionState == HttpConnectionState::IN_PROGRESS) ||
            (mInternalHttpRequestState.connectionState == HttpConnectionState::ABORTING))
        {
            // not possible to change while in progress
            return;
        }
        mUserAgent = aUserAgent;
    }

    void_t AndroidHttpConnection::setHttpDataProcessor(IHttpDataProcessor* aHttpDataProcessor)
    {
        mHttpDataProcessor = aHttpDataProcessor;
    }

OMAF_NS_END
