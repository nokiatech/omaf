
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
#include "Foundation/NVRThread.h"

#if OMAF_PLATFORM_ANDROID
#include "Foundation/Android/NVRAndroid.h"
#endif

#if OMAF_PLATFORM_UWP
    #include <roapi.h>
#endif

#if OMAF_PLATFORM_WINDOWS
#include <combaseapi.h>
#endif

OMAF_NS_BEGIN

// main() thread query
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

Thread::Id Thread::mMainThreadId = ::GetCurrentThreadId();

#elif OMAF_PLATFORM_ANDROID

Thread::Id Thread::mMainThreadId = ::gettid();

#else

    #error Unsupported platform

#endif

// Convert thread priority to OS priority and back
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

static int convertToOSThreadPriority(Thread::Priority::Enum priority)
{
    switch (priority)
    {
        case Thread::Priority::LOWEST:    return THREAD_PRIORITY_LOWEST;
        case Thread::Priority::LOW:       return THREAD_PRIORITY_BELOW_NORMAL;
        case Thread::Priority::NORMAL:    return THREAD_PRIORITY_NORMAL;
        case Thread::Priority::HIGH:      return THREAD_PRIORITY_ABOVE_NORMAL;
        case Thread::Priority::HIGHEST:   return THREAD_PRIORITY_HIGHEST;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return 0;
}

static Thread::Priority::Enum convertToThreadPriority(int priority)
{
    switch (priority)
    {
        case THREAD_PRIORITY_LOWEST:        return Thread::Priority::LOWEST;
        case THREAD_PRIORITY_BELOW_NORMAL:  return Thread::Priority::LOW;
        case THREAD_PRIORITY_NORMAL:        return Thread::Priority::NORMAL;
        case THREAD_PRIORITY_ABOVE_NORMAL:  return Thread::Priority::HIGH;
        case THREAD_PRIORITY_HIGHEST:       return Thread::Priority::HIGHEST;
            
        default:
            OMAF_ASSERT_UNREACHABLE();
            break;
    }
    
    return Thread::Priority::INVALID;
}

#elif OMAF_PLATFORM_ANDROID

static int convertToOSThreadPriority(Thread::Priority::Enum priority, int policy)
{
    OMAF_ASSERT(priority >= Thread::Priority::LOWEST, "");
    OMAF_ASSERT(priority <= Thread::Priority::HIGHEST, "");

    int minPriority = (int)Thread::Priority::LOWEST;
    int maxPriority = (int)Thread::Priority::HIGHEST;

    int minNativePriority = PRIO_MIN;
    int maxNativePriority = PRIO_MAX;

    float step = ((float)maxNativePriority - (float)minNativePriority) / (float)(maxPriority - minPriority);

    int nativePriority = minNativePriority + (step * (int)priority);
    
    return nativePriority;
}

static Thread::Priority::Enum convertToThreadPriority(int priority, int policy)
{
    int minPriority = (int)Thread::Priority::LOWEST;
    int maxPriority = (int)Thread::Priority::HIGHEST;

    int minNativePriority = PRIO_MIN;
    int maxNativePriority = PRIO_MAX;

    float step = ((float)maxPriority - (float)minPriority) / (float)(maxNativePriority - minNativePriority);

    int threadPriority = minPriority + (step * priority);

    return (Thread::Priority::Enum)threadPriority;
}

#endif

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

const Thread::Handle Thread::InvalidHandle = INVALID_HANDLE_VALUE;
const Thread::Id Thread::InvalidId = UINT32_MAX;

#elif OMAF_PLATFORM_ANDROID

const Thread::Handle Thread::InvalidHandle = 0;
const Thread::Id Thread::InvalidId = 0;

#else

    #error Unsupported platform

#endif

const uint32_t Thread::DefaultStackSize = 0;

Thread::Thread()
: mUserData(OMAF_NULL)
, mName(OMAF_NULL)
, mStackSize(DefaultStackSize)
, mPriority(Priority::INHERIT)
, mHandle(InvalidHandle)
, mId(InvalidId)
, mStopping(false)
, mRunning(false)
{
}

Thread::Thread(const char_t* name, uint32_t stackSize)
: mUserData(OMAF_NULL)
, mName(name)
, mStackSize(stackSize)
, mPriority(Priority::INHERIT)
, mHandle(InvalidHandle)
, mId(InvalidId)
, mStopping(false)
, mRunning(false)
{
}

Thread::~Thread()
{
    if (isRunning())
    {
        stop();
        join();
    }
    
    if (isValid())
    {
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        
        ::CloseHandle(mHandle);
    
        mHandle = InvalidHandle;
        mId = InvalidId;
        
#elif OMAF_PLATFORM_ANDROID
        
        ::pthread_detach(mHandle);
        
        mHandle = InvalidHandle;
        mId = InvalidId;
        
#endif
    }
    
    mUserData = OMAF_NULL;
    mName = OMAF_NULL;
    mStackSize = 0;
    
    mHandle = InvalidHandle;
    mId = InvalidId;
    
    mStopping = false;
    mRunning = false;
}

const char_t* Thread::getName() const
{
    return mName;
}

bool_t Thread::setName(const char_t* name)
{
    OMAF_ASSERT(mHandle == InvalidHandle, "Needs to be called before start()");
    
    mName = name;
    
    return true;
}

uint32_t Thread::getStackSize() const
{
    return mStackSize;
}

bool_t Thread::setStackSize(uint32_t stackSize)
{
    OMAF_ASSERT(mHandle == InvalidHandle, "Needs to be called before start()");
    
    mStackSize = stackSize;
    
    return true;
}

Thread::Priority::Enum Thread::getPriority() const
{
    return mPriority;
}

bool_t Thread::setPriority(Thread::Priority::Enum priority)
{
    OMAF_ASSERT(mHandle == InvalidHandle, "Needs to be called before start()");
    OMAF_ASSERT(priority != Thread::Priority::INVALID, "");
    OMAF_ASSERT(priority <= Thread::Priority::INHERIT, "");
    
    mPriority = priority;

    return true;
}

bool_t Thread::start(EntryFunction function, void_t* userData)
{
    OMAF_ASSERT(mHandle == InvalidHandle, "OS thread is already started");
    
    mEntryFunction = function;
    mUserData = userData;
    
    mStopping = false;
    mRunning = true;
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    // Create thread
    mHandle = ::CreateThread(NULL, mStackSize, threadMain, this, 0, &mId);
    OMAF_ASSERT(mHandle != InvalidHandle, "");
    
    if (mHandle != InvalidHandle)
    {
        return false;
    }
    
#elif OMAF_PLATFORM_ANDROID
    
    int result = 0;
    OMAF_UNUSED_VARIABLE(result);
    
    // Set stack size
    pthread_attr_t attr;
    result = ::pthread_attr_init(&attr);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    
    if (mStackSize != DefaultStackSize)
    {
        result = ::pthread_attr_setstacksize(&attr, mStackSize);
        OMAF_ASSERT(result == 0, ::strerror(errno));
    }
    
    // Create thread
    result = ::pthread_create(&mHandle, &attr, threadMain, this);
    OMAF_ASSERT(result == 0, ::strerror(errno));
    
    if (mHandle != InvalidHandle)
    {
        return false;
    }
    
#else
    
    #error Unsupported platform
    
#endif
    
    return true;
}

void_t Thread::stop()
{
    OMAF_ASSERT(mHandle != InvalidHandle, "Invalid OS thread");

    mStopping = true;
}

bool_t Thread::shouldStop() const
{
    return mStopping;
}

bool_t Thread::join()
{
    OMAF_ASSERT(mHandle != InvalidHandle, "Invalid OS thread");
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    // Trying to join current thread, most like this indicates error in application logic.
    if (GetCurrentThreadId() == mId)
    {
        return false;
    }
    
    DWORD result = ::WaitForSingleObject(mHandle, INFINITE);
    
    ::CloseHandle(mHandle);
    
    mHandle = InvalidHandle;
    mId = InvalidId;
    
    if(result != WAIT_OBJECT_0)
    {
        return false;
    }
    
#elif OMAF_PLATFORM_ANDROID
    
    void* status = NULL;
    int result = ::pthread_join(mHandle, &status);
    
    mHandle = InvalidHandle;
    mId = InvalidId;
    
    if(result != 0)
    {
        return false;
    }

#else
    
    #error Unsupported platform
    
#endif
    
    return true;
}

bool_t Thread::isRunning() const
{
    return mRunning;
}

bool_t Thread::isValid() const
{
    return (mHandle != InvalidHandle);
}

void_t Thread::sleep(uint32_t milliseconds)
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::Sleep(milliseconds);

#elif OMAF_PLATFORM_ANDROID

    ::usleep(milliseconds * 1000);

#else

    #error Unsupported platform

#endif
}

void_t Thread::yield()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    ::SwitchToThread();

#elif OMAF_PLATFORM_ANDROID

    ::sched_yield();

#else

    #error Unsupported platform

#endif
}

Thread::Id Thread::getCurrentThreadId()
{
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
    
    return ::GetCurrentThreadId();

#elif OMAF_PLATFORM_ANDROID

    return ::gettid();

#else

    #error Unsupported platform

#endif
}

Thread::Id Thread::getMainThreadId()
{
    return mMainThreadId;
}

bool_t Thread::isMainThread()
{
    return (getMainThreadId() == getCurrentThreadId());
}

#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

DWORD WINAPI Thread::threadMain(LPVOID threadInstance)

#elif OMAF_PLATFORM_ANDROID

void_t* Thread::threadMain(void_t* threadInstance)

#else

    #error Unsupported platform

#endif
{
    Thread* thread = (Thread*)threadInstance;

    // Fetch thread id
    thread->mId = getCurrentThreadId();
    
    // Set OS thread name here since most of the OS thread APIs can only set name for current thread.
    const char_t* threadName = thread->getName();
    
    if (threadName != OMAF_NULL)
    {
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

        // https://msdn.microsoft.com/en-us/library/xcb2z8hs(v=vs.140).aspx

        const DWORD MS_VC_EXCEPTION = 0x406D1388;

        #pragma pack(push, 8)
        
        typedef struct THREADNAME_INFO
        {
            DWORD dwType; // Must be 0x1000.
            LPCSTR szName; // Pointer to name (in user addr space).
            DWORD dwThreadID; // Thread ID (-1=caller thread).
            DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;

        #pragma pack(pop)

        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = threadName;
        info.dwThreadID = thread->mId;
        info.dwFlags = 0;

        #pragma warning(push)
        #pragma warning(disable: 6320 6322)

        // Note: Uses Structured Exception Handling (SEH), not traditional C++ exceptions that are usually turned off.
        __try
        {
            ::RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }

        #pragma warning(pop)

#elif OMAF_PLATFORM_ANDROID

        ::prctl(PR_SET_NAME, threadName, 0, 0, 0);

#else

    #error Unsupported platform

#endif
    }
    
    // Set OS thread priority
    Thread::Priority::Enum priority = thread->mPriority;
    
    if (priority != Thread::Priority::INHERIT)
    {
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP
        
        int priority = convertToOSThreadPriority(thread->mPriority);
        
        ::SetThreadPriority(thread->mHandle, priority);
        
#elif OMAF_PLATFORM_ANDROID
        
        int policy = 0;
        int nativePriority = convertToOSThreadPriority(priority, policy);
        
        int result = ::setpriority(PRIO_PROCESS, thread->mId, nativePriority);
        OMAF_ASSERT(result == 0, "");
        
#else
        
    #error Unsupported platform
        
#endif
    }
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // Begin platform thread section
    //
    ////////////////////////////////////////////////////////////////////////////////
    
    // TODO: CoInitializeEx, attachThread and auto-release pools should be handled in user code, not in OS thread wrapper.
    //       We should carefully investigate each thread instance do needed calls manually where needed.
    
    Thread::ReturnValue threadResult = 0;

#if OMAF_PLATFORM_UWP

    Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);

#elif OMAF_PLATFORM_WINDOWS 

    HRESULT result = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    OMAF_ASSERT(result == S_OK, "CoInitializeEx call failed");
    
#elif OMAF_PLATFORM_ANDROID
    
    Android::attachThread();

#endif
        
    // Call user defined entry function
    threadResult = thread->mEntryFunction.invoke(*thread, thread->mUserData);
    
    thread->mRunning = false;
    
    ////////////////////////////////////////////////////////////////////////////////
    //
    // End platform thread section
    //
    ////////////////////////////////////////////////////////////////////////////////
        
#if OMAF_PLATFORM_UWP

    Windows::Foundation::Uninitialize();

#elif OMAF_PLATFORM_WINDOWS
        
    CoUninitialize();

#elif OMAF_PLATFORM_ANDROID
    
    Android::detachThread();
    
#endif
    
#if OMAF_PLATFORM_WINDOWS || OMAF_PLATFORM_UWP

    return (unsigned)threadResult;

#elif OMAF_PLATFORM_ANDROID

    return (void_t*)threadResult;

#else

    #error Unsupported platform

#endif
}

OMAF_NS_END
