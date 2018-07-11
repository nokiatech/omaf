
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
#include "threadedworkpool.h"

namespace VDD
{
    struct ThreadedWorkPool::Thread
    {
        std::thread thread;
    };

    Job::Job() = default;

    Job::~Job() = default;

    ThreadedWorkPool::ThreadedWorkPool(Config aConfig)
        : mConfig(aConfig)
    {
        for (size_t n = 0; n < mConfig.numThreads; ++n)
        {
            mThreads.push_back(Thread { std::thread() });
            Thread* thread = &mThreads.back();
            thread->thread = std::thread([this,thread](){ run(thread); });
        }
    }

    void ThreadedWorkPool::run(ThreadedWorkPool::Thread*)
    {
        /* the Thread* is not actually (currently) used here.. */
        bool exit = false;
        while (!exit) {
            std::unique_ptr<Job> job;
            {
                std::unique_lock<std::mutex> lock(mWaiting.mutex);
                mWaiting.workAvailable.wait(lock, [&]{ return mWaiting.queue.size() > 0 || mTerminateThreads; });
                if (mWaiting.queue.size() > 0)
                {
                    // choose the oldest job
                    auto which = mWaiting.queue.begin();
                    job = std::move(which->second);
                    mWaiting.queue.erase(which);
                    mWaiting.spaceAvailable.notify_one();
                }
                else if (mTerminateThreads)
                {
                    exit = true;
                }
            }
            if (job)
            {
                job->run();
            }
        }
    }

    void ThreadedWorkPool::enqueue(std::unique_ptr<Job>&& job)
    {
        std::unique_lock<std::mutex> lock(mWaiting.mutex);
        mWaiting.spaceAvailable.wait(lock, [&]{ return mWaiting.queue.size() < mConfig.maxQueueLength; });
        mWaiting.queue.insert(std::make_pair(clock(), std::move(job)));
        mWaiting.workAvailable.notify_one();
    }

    ThreadedWorkPool::~ThreadedWorkPool()
    {
        {
            std::unique_lock<std::mutex> lock(mWaiting.mutex);
            mTerminateThreads = true;
            mWaiting.workAvailable.notify_all();
        }
        for (auto& thread: mThreads)
        {
            thread.thread.join();
        }
    }
}
