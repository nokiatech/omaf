
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
#include "Foundation/NVRThreadOperation.h"

OMAF_NS_BEGIN

ThreadOperation::ThreadOperation(void_t* userData, const char_t* name, uint32_t stackSize)
: mThread(name, stackSize)
, mUserData(userData)
{
}

ThreadOperation::~ThreadOperation()
{
    if (mThread.isValid())
    {
        mThread.stop();
        mThread.join();
    }
    
    mUserData = OMAF_NULL;
}

void_t ThreadOperation::run()
{
    if (!mThread.isRunning())
    {
        Thread::EntryFunction function;
        function.bind<ThreadOperation, &ThreadOperation::threadFunction>(this);
        
        mThread.start(function);
    }
}

void_t ThreadOperation::wait()
{
    if (mThread.isValid())
    {
        mThread.join();
    }
}

bool_t ThreadOperation::isRunning() const
{
    return mThread.isRunning();
}

Thread::ReturnValue ThreadOperation::getResult() const
{
    return mResult;
}

Thread::ReturnValue ThreadOperation::threadFunction(const Thread& thread, void_t* userData)
{
    mResult = task(userData);
    
    return mResult;
}

OMAF_NS_END
