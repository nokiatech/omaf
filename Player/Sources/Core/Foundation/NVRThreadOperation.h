
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2021 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "Platform/OMAFCompiler.h"
#include "Platform/OMAFDataTypes.h"
#include "Platform/OMAFPlatformDetection.h"

#include "Foundation/NVRCompatibility.h"
#include "Foundation/NVRNonCopyable.h"
#include "Foundation/NVRThread.h"

OMAF_NS_BEGIN

//
// Single shot thread operation.
//

class ThreadOperation
{
public:
    ThreadOperation(void_t* userData, const char_t* name = OMAF_NULL, uint32_t stackSize = 0);
    virtual ~ThreadOperation();

    // Run thread operation
    void_t run();

    // Waits until thread operation has finished current task
    void_t wait();

    // Returns if thread operation is running
    bool_t isRunning() const;

    // Returns return value of thread operation
    Thread::ReturnValue getResult() const;

public:
    // Task that thread operation will execute
    virtual Thread::ReturnValue task(void_t* userData) = 0;

private:
    Thread::ReturnValue threadFunction(const Thread& thread, void_t* userData);

private:
    OMAF_NO_DEFAULT(ThreadOperation);
    OMAF_NO_COPY(ThreadOperation);
    OMAF_NO_ASSIGN(ThreadOperation);

private:
    Thread mThread;
    void_t* mUserData;

    Thread::ReturnValue mResult;
};

OMAF_NS_END
