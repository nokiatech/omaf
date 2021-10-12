
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

#include <condition_variable>
#include <mutex>
#include <list>
#include <map>
#include <memory>
#include <thread>

#include "common/exceptions.h"

namespace VDD
{
    class Job
    {
    public:
        Job();
        virtual ~Job();
        virtual void run() = 0;
    };

    // Note that this threaded work pool does not guarantee that the jobs are taken into execution
    // in the same order as they are submitted
    class ThreadedWorkPool
    {
    public:
        struct Config
        {
            // Maximum number of jobs waiting for a thread. 1 is a valid value (even potentially
            // non-optimal) even if there are more threads, because when a job is started, it is
            // dequeued first.
            size_t maxQueueLength;

            // Number of threads to use for executing the jobs.
            size_t numThreads;
        };

        ThreadedWorkPool(Config aConfig);
        ~ThreadedWorkPool();

        void enqueue(std::unique_ptr<Job>&& job);

    public:
        const Config mConfig;

        struct Thread;

        // All threads are created during the initialization phase
        std::list<Thread> mThreads;

        // When mTerminateThreads, also the "work available" mutex is set
        bool mTerminateThreads = false;

        struct Queue {
            // protects the queue
            std::mutex mutex;
            // used to indicate that there is now more space available in the waiting queue
            std::condition_variable spaceAvailable;
            // used to indicate that there is now more work available in the waiting
            std::condition_variable workAvailable;

            // the queue is priorized by time
            std::multimap<clock_t, std::unique_ptr<Job>> queue;
        };

        // Jobs waiting for a thread to run them
        Queue mWaiting;

        void run(Thread* aThread);
    };
}
