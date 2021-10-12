
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

#include <mutex>
#include <condition_variable>
#include <memory>

#include "threadedworkpool.h"
#include "processor/processor.h"
#include "common/exceptions.h"

namespace VDD {
    class ThreadedPoolProcessor : public Processor {
    public:
        struct Config
        {
            ThreadedWorkPool* workpool;
            std::function<Processor*()> makeProcessor;
        };

        ThreadedPoolProcessor(const Config& aConfig);
        ~ThreadedPoolProcessor();

        StorageType getPreferredStorageType() const override;

        std::vector<Streams> process(const Streams& data) override;

    private:
        class Job;
        friend class Job;

        typedef size_t WorkQueueIndex;

        StorageType mPreferredStorageType;

        // Is there some work sent that we don't have yet results for?
        bool workPending();

        void enqueue(const Streams& streams);

        void process(WorkQueueIndex aIndex, const Streams& aStreams);

        // Dequeue ready jobs, or if mEndOfStream then wait for all jobs to finish
        std::vector<Streams> dequeue();

        // sorry for the name ;), but I want to indicate that the mResultQueueMutex must be held
        // when this function is called.  It returns all the available results from mResultQueue and
        // puts them to aFrames.
        void pullAvailableJobsFromResultsWithMutexHeld(std::vector<Streams>& aFrames);

        // Lend a processor (or create one if all are busy)
        std::unique_ptr<Processor> lendProcessor();

        // Return the processor
        void returnProcessor(std::unique_ptr<Processor>&&);

        // protects mAvailableProcessors
        std::mutex mAvailableProcessorsMutex;
        std::list<std::unique_ptr<Processor>> mAvailableProcessors;

        // protects the mWorkQueue
        std::mutex mWorkQueueMutex;
        // used for keeping a running count of submitted jobs to tag sent jobs with. This makes it
        // easy to pick the next ready item from the queue.
        WorkQueueIndex mWorkQueueIndex = 0;

        // protects the mResultQueue, mResultQueueMutex and mResultQueueIndex
        std::mutex mResultQueueMutex;
        // used to indicate that there is a result available. this is waited when flushing the ready queue.
        std::condition_variable mResultAvailable;
        // index of the next job to pick from mResultQueue
        WorkQueueIndex mResultQueueIndex = 0;
        // map from queue index to job
        std::map<WorkQueueIndex, std::vector<Streams>> mResultQueue;

        // not protected, as this is always set from the process method and read from the dequeue
        // method, which is called only by the process method.
        bool mEndOfStream = false;

        Config mConfig;
    };
}
