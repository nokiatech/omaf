
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
#ifndef UWP_CONNECTION
#include <Foundation\NVRHttp.h>
#include <Foundation\NVRHttpConnection.h>
#include <Foundation\NVRMutex.h>
#include <Foundation\NVRLogger.h>
#include <Foundation\NVRTime.h>
#include <Foundation\NVREvent.h>
#include "Foundation\NVRBandwidthMonitor.h"
OMAF_NS_BEGIN
OMAF_LOG_ZONE(HTTPCONN)

#include <winhttp.h>
#pragma comment(lib,"winhttp.lib")
class HttpConnectionWIN : public HttpConnection
{
public:
    HttpConnectionWIN(MemoryAllocator& allocator);
    ~HttpConnectionWIN();

    const HttpRequestState& getState() const;

    void_t setUserAgent(const utf8_t* aUserAgent);
    void_t setHttpDataProcessor(IHttpDataProcessor* aHttpDataProcessor);

    bool_t setUri(const Url& url);
    void_t setHeaders(const HttpHeaderList& headers);
    void_t setTimeout(uint32_t timeoutMS);
    void_t setKeepAlive(uint32_t timeMS);
    // Async
    HttpRequest::Enum get(DataBuffer<uint8_t>* output);
    // Async
    HttpRequest::Enum post(const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output);
    // Async
    HttpRequest::Enum head();
    // Async
    void_t abortRequest();
    // Sync
    void_t waitForCompletion();
	// Sync
    bool_t hasCompleted();
private:
    enum Method
    {
        GET = 0,
        POST = 1,
        HEAD = 2
    };
    void clear_headers();
    void finalize_request(HttpConnectionState::Enum aState);
    HttpRequest::Enum execute(Method command, const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output);
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    HINTERNET hAbortRequest;
    HINTERNET hFailRequest;
    HINTERNET hCompleteRequest;
    HttpRequestState mInternalState;
    DataBuffer<uint8_t>* mOutput;
    const DataBuffer<uint8_t>* mInput;
    mutable HttpRequestState mState;
    mutable bool_t mHeadersChanged;
    mutable Mutex mMutex;
    wchar_t* mHeaders;
    unsigned int mHeadersLength;
    MemoryAllocator& mAllocator;
    wchar_t* mUri;
    URL_COMPONENTS mUrlComp;
    wchar_t* mHost;
    wchar_t* mPath;
    WINHTTP_STATUS_CALLBACK mPrevCallback;
    static void CALLBACK Callback(_In_ HINTERNET hInternet, _In_ DWORD_PTR dwContext, _In_ DWORD     dwInternetStatus, _In_ LPVOID    lpvStatusInformation, _In_ DWORD     dwStatusInformationLength);

    void completion(_In_ HINTERNET hInternet, _In_ DWORD     dwInternetStatus, _In_ LPVOID    lpvStatusInformation, _In_ DWORD     dwStatusInformationLength);
    uint64_t mRequestStartTime, mRequestDuration;
    Event mRequestCompleteEvent;
    Event mSessionClosedEvent;
    Event mConnectClosedEvent;
    int32_t mTimeoutMS;
    IHttpDataProcessor* mHttpDataProcessor;
};


static void utf8towchar(const utf8_t* data, size_t bytes, wchar_t** res, int* len)
{
    *res = NULL;
    *len = 0;
    if ((data) && (bytes))
    {
        //well.. according to the test file (http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt) windows,chrome fail...
        //and ofcourse wchar is ucs-16 not utf-16, so we are limited to the first 65535 glyphs..
        //the same way as this.. then again, technically URI should should use ascii and percent encoding for anything else.. (somewhat simplified i know)
        *len = MultiByteToWideChar(CP_UTF8, 0, (const char*)data, (int32_t)bytes, NULL, 0);
        if (*len)
        {
            *res = new wchar_t[*len + 1];
            MultiByteToWideChar(CP_UTF8, 0, (const char*)data, (int32_t)bytes, *res, *len + 1);
            (*res)[*len] = 0;//make sure its null terminated.            
        }
    }
}

void CALLBACK HttpConnectionWIN::Callback(_In_ HINTERNET hInternet, _In_ DWORD_PTR dwContext, _In_ DWORD     dwInternetStatus, _In_ LPVOID    lpvStatusInformation, _In_ DWORD     dwStatusInformationLength)
{
    ((HttpConnectionWIN*)dwContext)->completion(hInternet, dwInternetStatus, lpvStatusInformation, dwStatusInformationLength);
}

HttpConnectionWIN::HttpConnectionWIN(MemoryAllocator& allocator) : mAllocator(allocator), mRequestCompleteEvent(false, false), mSessionClosedEvent(false,false), mConnectClosedEvent(false,false), mHttpDataProcessor(OMAF_NULL)
{
    mInternalState = HttpRequestState();
    mInternalState.totalBytes = -1;    
    mUri = NULL;
    mHost = NULL;
    mPath = NULL;
    mHeaders = WINHTTP_NO_ADDITIONAL_HEADERS;
    mHeadersLength = 0;
    mInput = mOutput = OMAF_NULL;
    ZeroMemory(&mUrlComp, sizeof(mUrlComp));
    mUrlComp.dwStructSize = sizeof(mUrlComp);
    mUrlComp.dwSchemeLength = (DWORD)-1;
    mUrlComp.dwHostNameLength = (DWORD)-1;
    mUrlComp.dwUserNameLength = (DWORD)-1;
    mUrlComp.dwPasswordLength = (DWORD)-1;
    mUrlComp.dwUrlPathLength = (DWORD)-1;
    mUrlComp.dwExtraInfoLength = (DWORD)-1;
    hFailRequest = hCompleteRequest = hAbortRequest = hRequest = hConnect = NULL;

    //windows 8.1+.. try anyway. because we expect to be on win8.1+
    hSession = WinHttpOpen(L""/*User agent needs to be defined!*/, WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY/*WINHTTP_ACCESS_TYPE_DEFAULT_PROXY*/, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    if (hSession == OMAF_NULL)
    {
        //Try with legacy proxy handling then..
        hSession = WinHttpOpen(L""/*User agent needs to be defined!*/, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
    }
    
    if (hSession == NULL)//WinHttpConnect returns NULL on failure.
    {
        //This is BAD, no http connections can be made. need to think about some fallback mechanism?
        //If this happen all the following calls will just fail.
        OMAF_LOG_E("WinHttpOpen failed!");
    }
    else
    {
        mPrevCallback = WinHttpSetStatusCallback(hSession, Callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
        DWORD_PTR context = (DWORD_PTR)this;
        WinHttpSetOption(hSession, WINHTTP_OPTION_CONTEXT_VALUE, &context, sizeof(context));

        //https://msdn.microsoft.com/en-us/library/aa384066(v=vs.85).aspx
        DWORD val;
        val = WINHTTP_DECOMPRESSION_FLAG_ALL;
        WinHttpSetOption(hSession, WINHTTP_OPTION_DECOMPRESSION, &val, sizeof(val));

        val = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;//system default
        //val = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
        //val = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(hSession, WINHTTP_OPTION_REDIRECT_POLICY, &val, sizeof(val));
    }
}
void_t HttpConnectionWIN::waitForCompletion()
{
    //wait for it...    
    for (;;)
    {
        if (hasCompleted())
        {
            break;
        }
        mRequestCompleteEvent.wait(5000);
    }
}
bool_t HttpConnectionWIN::hasCompleted()
{
    auto state = getState().connectionState;
    if (state == HttpConnectionState::ABORTED || state == HttpConnectionState::COMPLETED ||
        state == HttpConnectionState::FAILED || state == HttpConnectionState::IDLE ||
        state == HttpConnectionState::INVALID)
    {
        return true;
    }
    return false;
}

HttpConnectionWIN::~HttpConnectionWIN()
{
    abortRequest();
    waitForCompletion();
    //hRequest SHOULD be closed and invalid now.
    //close and wait for hConnect
    if (hConnect != NULL)
    {
        WinHttpCloseHandle(hConnect);
        mConnectClosedEvent.wait();//should be instant
    }
    //close and wait for hSession
    if (hSession != NULL)
    {
        WinHttpCloseHandle(hSession);
        mSessionClosedEvent.wait();//should be instant
    }
    delete[] mHost;
    delete[] mPath;
    delete[] mUri;
    delete[] mHeaders;
}

const HttpRequestState& HttpConnectionWIN::getState() const
{
    mMutex.lock();
    //hrmph.. still a bit stupid.
    //could do this more.. efficiently
    mState.connectionState = mInternalState.connectionState;
    mState.httpStatus = mInternalState.httpStatus;
    if (mHeadersChanged)
    {
        mState.headers = mInternalState.headers;
        mHeadersChanged = false;
    }
    mState.bytesDownloaded = mInternalState.bytesDownloaded;
    mState.bytesUploaded = mInternalState.bytesUploaded;
    mState.totalBytes = mInternalState.totalBytes;
    mState.output = mInternalState.output;
    mState.input = mInternalState.input;
    mMutex.unlock();
    return mState;
}

bool_t HttpConnectionWIN::setUri(const Url& url)
{
    mMutex.lock();
    if ((mInternalState.connectionState == HttpConnectionState::IN_PROGRESS) ||
        (mInternalState.connectionState == HttpConnectionState::ABORTING))
    {
        //cant start new request when old one is still busy.
        mMutex.unlock();
        return false;
    }
    mInput = mOutput = OMAF_NULL;
    mInternalState = HttpRequestState();
    mHeadersChanged = true;

    size_t bytes = url.getSize();
    const utf8_t* data = url.getData();
    wchar_t* res;
    int len;
    utf8towchar(data, bytes, &res, &len);

    URL_COMPONENTS urlComp;

    ZeroMemory(&urlComp, sizeof(urlComp));
    urlComp.dwStructSize = sizeof(urlComp);
    urlComp.dwSchemeLength = (DWORD)-1;
    urlComp.dwHostNameLength = (DWORD)-1;
    urlComp.dwUserNameLength = (DWORD)-1;
    urlComp.dwPasswordLength = (DWORD)-1;
    urlComp.dwUrlPathLength = (DWORD)-1;
    urlComp.dwExtraInfoLength = (DWORD)-1;
    if (!WinHttpCrackUrl(res, 0, 0/*ICU_ESCAPE*/, &urlComp))
    {
        delete[] res;
        mMutex.unlock();
        return false;
    }

    if ((urlComp.nScheme != INTERNET_SCHEME_HTTP) && (urlComp.nScheme != INTERNET_SCHEME_HTTPS))
    {
        //invalid schema..
        delete[] res;
        mMutex.unlock();
        return false;
    }


    //if host/port changed then close connection..
    bool changed = false;
    if ((mUrlComp.dwHostNameLength != urlComp.dwHostNameLength) || (mUrlComp.nPort != urlComp.nPort) || (mUrlComp.nScheme != urlComp.nScheme)) //if hostname changes or port changes or scheme changes, create a new connection.
    {
        //host changed.
        changed = true;
    }
    else
    {
        //okay.. host length,port and scheme matches.. verify that the host is the same
        if (wcsncmp(mUrlComp.lpszHostName, urlComp.lpszHostName, urlComp.dwHostNameLength) != 0)
        {
            //yup it changed
            changed = true;
        }
    }
    if (changed)
    {
        //close previous connection
        if (hConnect != NULL)
        {
            WinHttpCloseHandle(hConnect);
            mConnectClosedEvent.wait();
            //hConnect should be NULL now
            OMAF_ASSERT(hConnect == NULL, "hConnect not NULL after WinHttpCloseHandle!");
        }
        delete[] mHost;
        mHost = new wchar_t[urlComp.dwHostNameLength + 1];
        wcsncpy(mHost, urlComp.lpszHostName, urlComp.dwHostNameLength);
        mHost[urlComp.dwHostNameLength] = 0;
    }

    //TODO: should i check if this has changed?
    delete[] mPath;
    if (urlComp.dwExtraInfoLength)
    {
        //append extra info..
        mPath = new wchar_t[urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength + 1];
        wcsncpy(mPath, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        mPath[urlComp.dwUrlPathLength] = 0;
        wcsncat(mPath, urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
        mPath[urlComp.dwUrlPathLength + urlComp.dwExtraInfoLength] = 0;
    }
    else
    {
        mPath = new wchar_t[urlComp.dwUrlPathLength + 1];
        wcsncpy(mPath, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
        mPath[urlComp.dwUrlPathLength] = 0;
    }


    delete[] mUri;
    mUri = res;
    mUrlComp = urlComp;
    mInternalState.connectionState = HttpConnectionState::IDLE;    
    mMutex.unlock();
    return true;
}
void_t HttpConnectionWIN::setUserAgent(const utf8_t* aUserAgent)
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mMutex.unlock();
        return;
    }
    if ((mInternalState.connectionState == HttpConnectionState::IN_PROGRESS) ||
        (mInternalState.connectionState == HttpConnectionState::ABORTING))
    {
        //cant change state while busy
        mMutex.unlock();
        return;
    }    
    int len = 0;
    wchar_t* res = NULL;
    utf8towchar(aUserAgent, strlen(aUserAgent), &res,&len);
    WinHttpSetOption(hSession, WINHTTP_OPTION_USER_AGENT, res, len);
    delete[] res;
    mMutex.unlock();
}

void_t HttpConnectionWIN::setHttpDataProcessor(IHttpDataProcessor * aHttpDataProcessor)
{
    mHttpDataProcessor = aHttpDataProcessor;
}

void_t HttpConnectionWIN::setHeaders(const HttpHeaderList& headers)
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mMutex.unlock();
        return;
    }
    if ((mInternalState.connectionState == HttpConnectionState::IN_PROGRESS) ||
        (mInternalState.connectionState == HttpConnectionState::ABORTING))
    {
        //cant change state while busy
        mMutex.unlock();
        return;
    }    

    //delete old headers.
    clear_headers();

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        const char* k = it->first.getData();
        const char* v = it->second.getData();
        size_t kl = it->first.getSize();
        size_t vl = it->second.getSize();
        mHeadersLength += MultiByteToWideChar(CP_UTF8, 0, (const char*)k, (int32_t)kl, NULL, 0);
        if (mHeadersLength > 0) mHeadersLength--;
        mHeadersLength += 2; //': '
        mHeadersLength += MultiByteToWideChar(CP_UTF8, 0, (const char*)v, (int32_t)vl, NULL, 0);
        if (mHeadersLength > 0) mHeadersLength--;
        mHeadersLength += 2; //'\r\n'
    }
    mHeaders = new wchar_t[mHeadersLength + 1];
    wchar_t* end = mHeaders + mHeadersLength;
    //create a string holding all headers
    wchar_t* dst = mHeaders;
    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        const char* k = it->first.getData();
        const char* v = it->second.getData();
        size_t kl = it->first.getSize();
        size_t vl = it->second.getSize();
        int wrote = 0;
        wrote = MultiByteToWideChar(CP_UTF8, 0, (const char*)k, (int32_t)kl, dst, (int32_t)(end - dst));
        if (wrote > 0) wrote--;
        dst += wrote;
        *dst++ = ':';
        *dst++ = ' ';
        wrote = MultiByteToWideChar(CP_UTF8, 0, (const char*)v, (int32_t)vl, dst, (int32_t)(end - dst));
        if (wrote > 0) wrote--;
        dst += wrote;
        *dst++ = '\r';
        *dst++ = '\n';
    }
    *dst = 0;
    mMutex.unlock();
}
void_t HttpConnectionWIN::setTimeout(uint32_t timeoutMS)
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mMutex.unlock();
        return;
    }
    if ((mInternalState.connectionState == HttpConnectionState::IN_PROGRESS) ||
        (mInternalState.connectionState == HttpConnectionState::ABORTING))
    {
        //cant change state while busy
        mMutex.unlock();
        return;
    }
    mTimeoutMS = timeoutMS;
    DWORD timeout = timeoutMS;
    BOOL res;
    res = WinHttpSetOption(hSession, WINHTTP_OPTION_RESOLVE_TIMEOUT, &timeout, sizeof(timeout));//dns timeout..
    res = WinHttpSetOption(hSession, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));//max time for connection to be established.
    res = WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));//max time each read can take..
    /*WinHttpSetOption(hSession, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));//max time each write can take..
    WinHttpSetOption(hSession, WINHTTP_OPTION_RECEIVE_RESPONSE_TIMEOUT, &timeout, sizeof(timeout));
    */
    //WinHttpSetTimeouts(hSession, timeoutMS /*dns timeout*/, timeoutMS/*connect timeout*/, timeoutMS/*send timeout*/, timeoutMS/*receive timeout*/);
    mMutex.unlock();
}
void_t HttpConnectionWIN::setKeepAlive(uint32_t timeMS)
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mMutex.unlock();
        return;
    }
    if ((mInternalState.connectionState == HttpConnectionState::IN_PROGRESS) ||
        (mInternalState.connectionState == HttpConnectionState::ABORTING))
    {
        //cant change state while busy
        mMutex.unlock();
        return;
    }    
    //TODO: implement!
    mMutex.unlock();
}

#ifdef OMAF_DEBUG_BUILD
static struct
{
    DWORD status;
    const char* const string;
} status2string[] =
{
WINHTTP_CALLBACK_STATUS_RESOLVING_NAME          ,NULL/*"WINHTTP_CALLBACK_STATUS_RESOLVING_NAME         "*/,
WINHTTP_CALLBACK_STATUS_NAME_RESOLVED           ,NULL/*"WINHTTP_CALLBACK_STATUS_NAME_RESOLVED          "*/,
WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER    ,"WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER   ",
WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER     ,"WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER    ",
WINHTTP_CALLBACK_STATUS_SENDING_REQUEST         ,NULL/*"WINHTTP_CALLBACK_STATUS_SENDING_REQUEST        "*/,
WINHTTP_CALLBACK_STATUS_REQUEST_SENT            ,NULL/*"WINHTTP_CALLBACK_STATUS_REQUEST_SENT           "*/,
WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE      ,NULL/*"WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE     "*/,
WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED       ,NULL/*"WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED      "*/,
WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION      ,"WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION     ",
WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED       ,"WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED      ",
WINHTTP_CALLBACK_STATUS_HANDLE_CREATED          ,NULL/*"WINHTTP_CALLBACK_STATUS_HANDLE_CREATED         "*/,
WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING          ,NULL/*"WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING         "*/,
WINHTTP_CALLBACK_STATUS_DETECTING_PROXY         ,NULL/*"WINHTTP_CALLBACK_STATUS_DETECTING_PROXY        "*/,
WINHTTP_CALLBACK_STATUS_REDIRECT                ,NULL/*"WINHTTP_CALLBACK_STATUS_REDIRECT               "*/,
WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE   ,NULL/*"WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE  "*/,
WINHTTP_CALLBACK_STATUS_SECURE_FAILURE          ,NULL/*"WINHTTP_CALLBACK_STATUS_SECURE_FAILURE         "*/,
WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE       ,NULL/*"WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE      "*/,
WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE          ,NULL/*"WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE         "*/,
WINHTTP_CALLBACK_STATUS_READ_COMPLETE           ,NULL/*"WINHTTP_CALLBACK_STATUS_READ_COMPLETE          "*/,
WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE          ,NULL/*"WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE         "*/,
WINHTTP_CALLBACK_STATUS_REQUEST_ERROR           ,NULL/*"WINHTTP_CALLBACK_STATUS_REQUEST_ERROR          "*/,
WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE    ,NULL/*"WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE   "*/,
WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE ,NULL/*"WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE"*/,
WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE          ,NULL/*"WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE         "*/,
WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE       ,NULL/*"WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE      "*/,
//WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE ,NULL/*"WINHTTP_CALLBACK_STATUS_SETTINGS_WRITE_COMPLETE"*/,
//WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE  ,NULL/*"WINHTTP_CALLBACK_STATUS_SETTINGS_READ_COMPLETE "*/,
(DWORD)-1,NULL };
#endif
void HttpConnectionWIN::completion(_In_ HINTERNET hInternet, _In_ DWORD     dwInternetStatus, _In_ LPVOID    lpvStatusInformation, _In_ DWORD     dwStatusInformationLength)
{
#ifdef OMAF_DEBUG_BUILD
    char* handler = "\tUnknown";
    if (hInternet == hRequest)
    {
        handler = "\tRequest";
    }
    else if (hInternet == hConnect)
    {
        handler = "\tConnection";
    }
    else if (hInternet == hSession)
    {
        handler = "\tSession";
    }
    for (int i = 0;; i++)
    {
        if (status2string[i].status == -1)
        {
            OMAF_LOG_V("%p Unknown status:%d%s", this, dwInternetStatus, handler);
            break;
        }
        if (status2string[i].status == dwInternetStatus)
        {
            if (status2string[i].string)
            {
                OMAF_LOG_V("%p %s%s", this, status2string[i].string, handler);
            }
            break;
        }
    }
#endif
    mMutex.lock();
    if ((dwInternetStatus == WINHTTP_CALLBACK_STATUS_REQUEST_ERROR)&&(hInternet==hAbortRequest))
    {
        //received an error to request that we are aborting. (ie. to the request we are closing)
        WINHTTP_ASYNC_RESULT* res = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;
        if (res->dwError == ERROR_WINHTTP_OPERATION_CANCELLED)
        {
            //This is expected. operation is now cancelled.
            mMutex.unlock();
            return;
        }
        OMAF_LOG_V("Received error other than CANCELLED while aborting!");
        //... which we just handle it as abort completed.
        mMutex.unlock();
        return;        
    }
    if (dwInternetStatus == WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING)
    {
        //absolutely final callback for this handle. handle has been closed already.
        if (hInternet == hAbortRequest)
        {
            OMAF_LOG_V("Request closed");
            hAbortRequest = NULL;
            finalize_request(HttpConnectionState::ABORTED);
        }
        else if (hInternet == hFailRequest)
        {
            //HMM...
            OMAF_LOG_V("Request closed");
            hFailRequest = NULL;
            finalize_request(HttpConnectionState::FAILED);
        }
        else if (hInternet == hCompleteRequest)
        {
            //HMM...
            OMAF_LOG_V("Request closed");
            hCompleteRequest = NULL;

            // call post processor before setting request as completed
            if (mHttpDataProcessor != OMAF_NULL)
            {
                mMutex.unlock();
                mHttpDataProcessor->processHttpData();
                mMutex.lock();
            }

            finalize_request(HttpConnectionState::COMPLETED);
        }
        else if (hInternet == hConnect)
        {
            //should have no requests anymore..
            OMAF_LOG_V("Connection closed");
            hConnect = NULL;
            mConnectClosedEvent.signal();
        }
        else if (hInternet == hSession)
        {
            //should have no requests OR connections anymore..
            OMAF_LOG_V("Session closed");
            hSession = NULL;
            mSessionClosedEvent.signal();
        }
        else
        {
            OMAF_LOG_V("Unknown handle closed");
        }
        mMutex.unlock();
        return;
    }

    if (hInternet != hRequest)
    {
        //not an request event? (or a handle created event...)        
        if (hInternet == hAbortRequest)
        {
            //it is possible that we receive events to a request we just aborted..
            //this is fine. just ignore the event.
            mMutex.unlock();
            return;
        }
        if (dwInternetStatus != WINHTTP_CALLBACK_STATUS_HANDLE_CREATED)
        {
            OMAF_LOG_V("Unhandled WINHTTP event:%d handle unknown.", dwInternetStatus);
        }
        mMutex.unlock();
        return;
    }
    switch (dwInternetStatus)
    {
        case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
        {
            OMAF_LOG_V("Connection teardown starts!");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
        {
            OMAF_LOG_V("Connection closed!");
            break;
        }
        case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
        case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
        case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
        case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
        case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
        case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
        case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
        {
            //we dont actually care about these progress notifications.
            break;
        }
        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
        {
            //failed.. 
            //WINHTTP_ASYNC_RESULT* res = (WINHTTP_ASYNC_RESULT*)lpvStatusInformation;//we dont actually care why it failed.
            hFailRequest = hInternet;
            hRequest = NULL;
            WinHttpCloseHandle(hFailRequest);
            break;
        }
        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        {
            //start receiving response then..
            //if we had input buffer..
            if (mInput)
            {
                mInternalState.bytesUploaded = mInput->getSize();
            }
            if (!WinHttpReceiveResponse(hInternet, NULL))
            {
                hFailRequest = hRequest;
                hRequest = NULL;
                WinHttpCloseHandle(hFailRequest);
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        {
            //headers received
            HttpHeaderList headers;
            DWORD dwSize;
            DWORD dwStatusCode, dwConLen;
            BOOL ret;
            dwSize = sizeof(dwStatusCode);
            ret = WinHttpQueryHeaders(hInternet, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwStatusCode, &dwSize, WINHTTP_NO_HEADER_INDEX);
            mInternalState.httpStatus = dwStatusCode;
            dwSize = sizeof(dwStatusCode);
            if (WinHttpQueryHeaders(hInternet, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &dwConLen, &dwSize, WINHTTP_NO_HEADER_INDEX))
            {
                //YAY!, got total length (which might not be correct, if body is compressed, this is the compressed size, but it's still a good guestimates)
                mInternalState.totalBytes = dwConLen;
            }
            dwSize = 0;
            ret = WinHttpQueryHeaders(hInternet, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &dwSize, WINHTTP_NO_HEADER_INDEX);
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                wchar_t *lpOutBuffer = new wchar_t[dwSize / sizeof(wchar_t)];
                ret = WinHttpQueryHeaders(hInternet, WINHTTP_QUERY_RAW_HEADERS, WINHTTP_HEADER_NAME_BY_INDEX, lpOutBuffer, &dwSize, WINHTTP_NO_HEADER_INDEX);
                for (wchar_t* hdr = lpOutBuffer;;)
                {
                    int kl, vl;
                    wchar_t* k;
                    wchar_t* v;
                    if (*hdr == 0) break;
                    k = hdr;
                    for (kl = 0;; kl++)
                    {
                        if ((k[kl] == 0) || (k[kl] == ':')) break;
                    }
                    v = NULL;
                    if (k[kl] == ':')
                    {
                        v = &k[kl + 1];
                        for (;; v++)
                        {
                            if (*v == ' ') continue;
                            if (*v == '\t') continue;
                            break;
                        }
                        for (vl = 0;; vl++)
                        {
                            if (v[vl] == 0) break;
                        }
                    }
                    int len;
                    char *key, *val;
                    key = val = NULL;
                    //convert key from wchar_t (ucs16/utf16) to utf8
                    len = WideCharToMultiByte(CP_UTF8, 0, k, kl, NULL, 0, NULL, NULL);
                    if (len > 0)
                    {
                        key = new char[len + 1];
                        WideCharToMultiByte(CP_UTF8, 0, k, kl, key, len + 1, NULL, NULL);
                        key[len] = 0;
                    }

                    //convert key from wchar_t (ucs16/utf16) to utf8
                    if (v)
                    {
                        len = WideCharToMultiByte(CP_UTF8, 0, v, vl, NULL, 0, NULL, NULL);
                        if (len > 0)
                        {
                            val = new char[len + 1];
                            WideCharToMultiByte(CP_UTF8, 0, v, vl, val, len + 1, NULL, NULL);
                            val[len] = 0;
                        }
                    }
                    else
                    {
                        val = NULL;
                    }

                    if (key&&val)
                    {
                        headers.add(key, val);
                    }
                    else
                    {
                        if (key == 0)
                        {
                            //HMMM!
                            key = key;
                        }
                    }
                    delete[] key;
                    delete[] val;
                    hdr += wcslen(hdr) + 1;
                }
                delete[] lpOutBuffer;
            }
            mInternalState.headers = headers;
            mHeadersChanged = true;
            //start reads..
            if (mOutput)
            {
                if (mInternalState.totalBytes != -1)
                {
                    if (mOutput->getCapacity() < mInternalState.totalBytes)
                    {
                        mOutput->reserve(dwConLen);
                    }
                }
                else
                {
                    if (mOutput->getCapacity() < 8192)
                    {
                        //make sure we have atleast 8kb of buffer. (8kb is the internal buffer size of winhttp)
                        mOutput->reserve(8192);
                    }
                }
                mOutput->setSize(0);
                if (!WinHttpQueryDataAvailable(hInternet, NULL))
                {
                    hFailRequest = hRequest;
                    hRequest = NULL;
                    WinHttpCloseHandle(hFailRequest);
                }
            }
            else
            {
                //no output buffer, so we can just stop now. 
                //most likely a head call so no data is even expected to come
                //although could be a post where we user expects no return data..... unsure if we MUST read the response body anyway.
                mRequestDuration = Time::getClockTimeUs() - mRequestStartTime;
                OMAF_LOG_V("RequestCompleted:  %lld bytes took: %.4f Ms", mInternalState.totalBytes, mRequestDuration / 1000.f);
                hCompleteRequest = hRequest;
                hRequest = NULL;
                WinHttpCloseHandle(hCompleteRequest);
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
        {
            DWORD availdata = *((DWORD*)lpvStatusInformation);
            if (availdata == 0)
            {
                mRequestDuration = Time::getClockTimeUs() - mRequestStartTime;
                OMAF_LOG_V("RequestCompleted:  %lld bytes took: %.4f Ms", mInternalState.totalBytes, mRequestDuration / 1000.f);
                BandwidthMonitor::notifyDownloadCompleted(mRequestDuration / 1000, mInternalState.bytesDownloaded);
                hCompleteRequest = hRequest;
                hRequest = NULL;
                WinHttpCloseHandle(hCompleteRequest);
            }
            else
            {
                size_t freespace = mOutput->getCapacity() - mOutput->getSize();
                if (freespace < availdata)
                {
                    //either double the size of output buffer, or resize it to needed, depending which is more.
                    size_t need = (availdata - freespace);
                    size_t rs = mOutput->getCapacity();
                    if (rs > need)
                    {
                        need = rs;
                    }
                    else
                    {
                        need = need;
                    }
                    OMAF_LOG_V("Reallocating output was:%lld now:%lld", (uint64_t)mOutput->getCapacity(), (uint64_t)mOutput->getCapacity() + need);
                    mOutput->reserve(mOutput->getCapacity() + need);
                }
                if (!WinHttpReadData(hInternet, mOutput->getDataPtr() + mOutput->getSize(), availdata, 0))
                {
                    hFailRequest = hRequest;
                    hRequest = NULL;
                    WinHttpCloseHandle(hFailRequest);
                }
            }
            break;
        }
        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        {
            //read more..
            if (dwStatusInformationLength == 0)
            {
                //all data read..
                mRequestDuration = Time::getClockTimeUs() - mRequestStartTime;
                OMAF_LOG_V("RequestCompleted:  %lld bytes took: %.4f Ms", mInternalState.totalBytes, mRequestDuration / 1000.f);
                BandwidthMonitor::notifyDownloadCompleted(mRequestDuration / 1000, mInternalState.bytesDownloaded);
                hCompleteRequest = hRequest;
                hRequest = NULL;
                WinHttpCloseHandle(hCompleteRequest);
            }
            else
            {
                //more data.
                mInternalState.bytesDownloaded += dwStatusInformationLength;
                mOutput->setSize(mOutput->getSize() + dwStatusInformationLength);
                if (!WinHttpQueryDataAvailable(hInternet, NULL))
                {
                    hFailRequest = hRequest;
                    hRequest = NULL;
                    WinHttpCloseHandle(hFailRequest);
                }
            }
            break;
        }
        default:
        {
            OMAF_LOG_V("Unhandled WINHTTP state:%d", dwInternetStatus);
            break;
        }
    }
    mMutex.unlock();
}
void HttpConnectionWIN::finalize_request(HttpConnectionState::Enum aState)
{
    OMAF_LOG_V("finalize request:%d", aState);
    clear_headers();    
    mInternalState.connectionState = aState;
    mRequestCompleteEvent.signal();
}
HttpRequest::Enum HttpConnectionWIN::execute(Method command, const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output)
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mMutex.unlock();
        return HttpRequest::FAILED;
    }
    if (mInternalState.connectionState != HttpConnectionState::IDLE)
    {
        //cant start new request when old one is still busy.
        mMutex.unlock();
        return HttpRequest::FAILED;
    }
    OMAF_ASSERT(hRequest == NULL, "hRequest set during execute!");
    OMAF_ASSERT(hFailRequest == NULL, "hFailRequest set during execute!");
    OMAF_ASSERT(hCompleteRequest == NULL, "hCompleteRequest set during execute!");
    OMAF_ASSERT(hAbortRequest == NULL, "hAbortRequest set during execute!");
    const wchar_t* Verb;
    switch (command)
    {
        case Method::GET:
        {
            Verb = L"GET";
            break;
        }
        case Method::POST:
        {
            Verb = L"POST";
            break;
        }
        case Method::HEAD:
        {
            Verb = L"HEAD";
            break;
        }
        default:
        {
            mMutex.unlock();
            return HttpRequest::FAILED;
        }
    }

    mInternalState.bytesDownloaded = 0;
    mInternalState.bytesUploaded = 0;
    mInternalState.totalBytes = -1;
    mInternalState.connectionState = HttpConnectionState::IN_PROGRESS;
    mInternalState.headers.clear();
    mInternalState.httpStatus = 0;
    mInternalState.input = mInput = input;
    mInternalState.output = mOutput = output;

    if (hConnect == NULL)
    {
        hConnect = WinHttpConnect(hSession, mHost, mUrlComp.nPort, 0);
        if (hConnect == NULL)//WinHttpConnect returns NULL on failure.
        {
            finalize_request(HttpConnectionState::FAILED);
            mMutex.unlock();
            return HttpRequest::FAILED;
        }
    }

    DWORD flags = 0;
    if (mUrlComp.nScheme == INTERNET_SCHEME_HTTPS)
    {
        flags = WINHTTP_FLAG_SECURE;
    }
    mRequestStartTime = Time::getClockTimeUs();
    hRequest = WinHttpOpenRequest(
        hConnect,
        Verb,
        mPath,
        NULL,
        WINHTTP_NO_REFERER,
        NULL,
        flags | WINHTTP_FLAG_REFRESH);

    if (hRequest == NULL)//WinHttpOpenRequest returns NULL on failure.
    {
        hRequest = NULL;
        finalize_request(HttpConnectionState::FAILED);
        mMutex.unlock();
        return HttpRequest::FAILED;
    }

    // Send a request.
    const void* out = WINHTTP_NO_REQUEST_DATA;
    DWORD outlen = 0;
    if (command == POST)
    {
        out = mInput->getDataPtr();
        outlen = (DWORD)mInput->getSize();
    }
    
    HttpRequest::Enum result = HttpRequest::OK;
    mRequestCompleteEvent.reset();
    if (!WinHttpSendRequest(hRequest, mHeaders, mHeadersLength, (void*)out, outlen, outlen, 0))
    {
        WinHttpCloseHandle(hRequest);
        hRequest = NULL;
        finalize_request(HttpConnectionState::FAILED);
        mMutex.unlock();
        return HttpRequest::FAILED;
    }

    mMutex.unlock();
    return HttpRequest::OK;

}
void HttpConnectionWIN::clear_headers()
{
    if (mHeaders != WINHTTP_NO_ADDITIONAL_HEADERS)
    {
        delete[] mHeaders;
    }
    mHeaders = WINHTTP_NO_ADDITIONAL_HEADERS;
    mHeadersLength = 0;
}
// Async
HttpRequest::Enum HttpConnectionWIN::get(DataBuffer<uint8_t>* output)
{
    //require output for gets..
    if (output == NULL) return HttpRequest::FAILED;
    return execute(Method::GET, NULL, output);
}
// Async
HttpRequest::Enum HttpConnectionWIN::post(const DataBuffer<uint8_t>* input, DataBuffer<uint8_t>* output)
{
    //require input and output buffers for posts..
    if (output == NULL) return HttpRequest::FAILED;
    if (input == NULL) return HttpRequest::FAILED;
    return execute(Method::POST, input, output);
}
// Async
HttpRequest::Enum HttpConnectionWIN::head()
{
    return execute(Method::HEAD, NULL, NULL);
}
// Async
void_t HttpConnectionWIN::abortRequest()
{
    mMutex.lock();
    if (hSession == OMAF_NULL)
    {
        mInternalState.connectionState = HttpConnectionState::ABORTED;
        mMutex.unlock();
        return;
    }

    if (mInternalState.connectionState == HttpConnectionState::IN_PROGRESS)
    {
        mInternalState.connectionState = HttpConnectionState::ABORTING;
        hAbortRequest = hRequest;
        hRequest = NULL;
        if (hAbortRequest != NULL)
        {
            WinHttpCloseHandle(hAbortRequest);//triggers request failed and close handle.
        }
        else
        {
            OMAF_LOG_E("hAbortRequest is invalid in abortRequest");
            mInternalState.connectionState = HttpConnectionState::ABORTED;
        }
    }
    mMutex.unlock();

}

namespace Http
{
    HttpConnection* createHttpConnectionWIN(MemoryAllocator& allocator)
    {
        HttpConnection* connection = OMAF_NEW(allocator, HttpConnectionWIN)(allocator);
        return connection;
    }
}
OMAF_NS_END
#endif