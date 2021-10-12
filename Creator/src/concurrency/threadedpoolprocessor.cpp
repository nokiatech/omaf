
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
#include "threadedpoolprocessor.h"
#include "threadedworkpool.h"

#include "common/utils.h"
#include "log/log.h"

namespace VDD {
    class ThreadedPoolProcessor::Job : public VDD::Job {
    public:
        Job(ThreadedPoolProcessor& aPool, ThreadedPoolProcessor::WorkQueueIndex aIndex, const Streams& aStreams)
            : mPool(aPool)
            , mIndex(aIndex)
            , mStreams(aStreams)
        {
            // nothing
        }

        void run() override
        {
            mPool.process(mIndex, mStreams);
        }

    private:
        ThreadedPoolProcessor& mPool;
        WorkQueueIndex mIndex;
        Streams mStreams;
    };

    ThreadedPoolProcessor::ThreadedPoolProcessor(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    ThreadedPoolProcessor::~ThreadedPoolProcessor()
    {
        // nothing
    }

    void ThreadedPoolProcessor::process(WorkQueueIndex aIndex, const Streams& aStreams)
    {
        auto processor = lendProcessor();
        std::vector<Streams> frames;
        try
        {
            frames = processor->process(aStreams);
        }
        catch (std::runtime_error& exn)
        {
            std::cerr << "ThreadedPoolProcessor: uncaught exception: " << exn.what() << std::endl;
        }
        returnProcessor(std::move(processor));

        std::unique_lock<std::mutex> resultQueueLock(mResultQueueMutex);
        mResultQueue[aIndex] = std::move(frames);
        mResultAvailable.notify_one();
    }

    std::unique_ptr<Processor> ThreadedPoolProcessor::lendProcessor()
    {
        std::unique_lock<std::mutex> lock(mAvailableProcessorsMutex);
        if (mAvailableProcessors.size() == 0)
        {
            auto processor = std::unique_ptr<Processor>(mConfig.makeProcessor());
            processor->setLog(getLog().instance());
            return processor;
        }
        else
        {
            std::unique_ptr<Processor> processor = std::move(mAvailableProcessors.front());
            mAvailableProcessors.pop_front();
            return processor;
        }
    }

    void ThreadedPoolProcessor::returnProcessor(std::unique_ptr<Processor>&& processor)
    {
        std::unique_lock<std::mutex> lock(mAvailableProcessorsMutex);
        mAvailableProcessors.push_back(std::move(processor));
    }

    void ThreadedPoolProcessor::enqueue(const Streams& aStreams)
    {
        WorkQueueIndex index;
        {
            std::unique_lock<std::mutex> lock(mWorkQueueMutex);
            index = mWorkQueueIndex;
            ++mWorkQueueIndex;
        }
        auto job = Utils::make_unique<Job>(*this, index, aStreams);
        mConfig.workpool->enqueue(std::move(job));
    }

    void ThreadedPoolProcessor::pullAvailableJobsFromResultsWithMutexHeld(std::vector<Streams>& aFrames)
    {
        // pull all results once the first one is available
        while (mResultQueue.count(mResultQueueIndex) > 0)
        {
            if (!aFrames.size())
            {
                aFrames = std::move(mResultQueue[mResultQueueIndex]);
            }
            else
            {
                for (Streams& streams: mResultQueue[mResultQueueIndex])
                {
                    aFrames.push_back(streams);
                }
            }
            mResultQueue.erase(mResultQueueIndex);
            ++mResultQueueIndex;
        }
    }

    std::vector<Streams> ThreadedPoolProcessor::dequeue()
    {
        std::vector<Streams> frames;

        if (mEndOfStream)
        {
            //std::cout << "End of stream!" << std::endl;
            // at end of stream we pull all available results
            while (workPending())
            {
                std::unique_lock<std::mutex> resultQueueLock(mResultQueueMutex);
                mResultAvailable.wait(resultQueueLock, [&]{ return mResultQueue.count(mResultQueueIndex) > 0; });

                pullAvailableJobsFromResultsWithMutexHeld(frames);
            }
        }
        else
        {
            //std::cout << "Not end of stream" << std::endl;
            // Normally we pull only the results that are sequentially available
            std::unique_lock<std::mutex> requestQueueLock(mResultQueueMutex);

            pullAvailableJobsFromResultsWithMutexHeld(frames);
        }
        //std::cout << "Done dequeuing" << std::endl;

        return frames;
    }

    bool ThreadedPoolProcessor::workPending()
    {
        std::unique_lock<std::mutex> workQueueLock(mWorkQueueMutex);
        std::unique_lock<std::mutex> resultsQueueLock(mResultQueueMutex);
        //std::cout << "Work pending? " << mResultQueueIndex << " vs " <<  mWorkQueueIndex << std::endl;
        return mResultQueueIndex != mWorkQueueIndex;
    }

    StorageType ThreadedPoolProcessor::getPreferredStorageType() const
    {
        return mPreferredStorageType;
    }

    std::vector<Streams> ThreadedPoolProcessor::process(const Streams& aStreams)
    {
        std::vector<Streams> frames;
        frames = dequeue();
        enqueue(aStreams);
        if (aStreams.isEndOfStream())
        {
            mEndOfStream = true;
            for (Streams& streams: dequeue())
            {
                frames.push_back(streams);
            }
        }
        return frames;
    }
}
