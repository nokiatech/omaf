
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
#include <mutex>
#include <algorithm>

#include "mediasteplock.h"
#include "graphbase.h"

namespace VDD
{
    const MediaStepLock::Config MediaStepLock::defaultConfig { 5 };

    class MediaStepLockState
    {
    public:
        using Id = int;

        MediaStepLockState(const MediaStepLock::Config& aConfig);

        std::unique_ptr<MediaStepLockProcessor> newState(GraphBase& aGraphBase, std::shared_ptr<MediaStepLockState> aSelf);

        void operator=(const MediaStepLockState&) = delete;
        MediaStepLockState(const MediaStepLockState&) = delete;
        MediaStepLockState(MediaStepLockState&&) = delete;

        ~MediaStepLockState() = default;

        void submit(Id aId, const Streams& aStreams);

        //eli parempihan se on että MediaStepLockState kerää frameja ja dumppaa niitä kun on riittävästi, tai jos niitä ei ole riittävästi niin kaikkiin menee end of stream

    private:
        MediaStepLock::Config mConfig;

        struct Processor
        {
            int frameCounter = 0; // does not count EndOfStream
            int forwardedFrameCounter = 0;
            MediaStepLockProcessor* processor = nullptr;
            std::list<Streams> frames;
            bool paused = false;
            bool eosReceived = false;
        };

        std::mutex mMutex;
        std::map<Id, Processor> mProcessors;
        Id mIdGen = 0;
        Optional<int> mLastFrame; // if frame number == this value, then the stream has reached end of stream. does not include EOS.
        bool mEosSent = false;

        int getNumberOfAvailableFrames() const;
        int getSmallestFrameCounter() const;
        void workWhileMutexHeld();
   };

    MediaStepLockState::MediaStepLockState(const MediaStepLock::Config& aConfig)
        : mConfig(aConfig)
    {
        // nothihng
    }

    std::unique_ptr<MediaStepLockProcessor> MediaStepLockState::newState(GraphBase& aGraphBase, std::shared_ptr<MediaStepLockState> aSelf)
    {
        int id = ++mIdGen;

        assert(aSelf.get() == this);

        mProcessors[id] = {};

        auto processor = Utils::make_unique<MediaStepLockProcessor>(aGraphBase, id, aSelf);

        mProcessors[id].processor = processor.get();

        return processor;
    }

    /*
     * Cases:
     *
     * A is input
     *
     * State:
     *  Node1: []
     *  Node2: []
     *  Node3: []
     *
     * Ndoe 1 receives A1:
     *
     *  Node1: [A1]
     *  Node2: []
     *  Node3: []
     *
     * submit returns: PAUSE
     *
     * Node 2 receives B1:
     *
     *  Node1: [A1]
     *  Node2: [B1]
     *  Node3: []
     *
     * submit returns: PAUSE
     *
     * Node 1 receives A2:
     *
     *  Node1: [A1, A2]
     *  Node2: [B1]
     *  Node3: []
     *
     * submit returns: PAUSE
     *
     * Node 3 receives C1:
     *
     *  Node1: [A1, A2]
     *  Node2: [B1]
     *  Node3: [C1]
     *
     * submit returns: PASS
     *
     */

    void MediaStepLockState::submit(Id aId, const Streams& aStreams)
    {
        std::unique_lock<std::mutex> lock(mMutex);

        if (!mEosSent)
        {
            Processor& processor = mProcessors.at(aId);

            if (aStreams.isEndOfStream())
            {
                processor.eosReceived = true;
            }
            else
            {
                processor.frames.push_back(aStreams);
                ++processor.frameCounter;
            }

            workWhileMutexHeld();
        }
    }

    void MediaStepLockState::workWhileMutexHeld()
    {
        int numAvailable = getNumberOfAvailableFrames();
        for (int count = 0; count < numAvailable; ++count)
        {
            for (auto& idProcessor : mProcessors)
            {
                auto& processor = idProcessor.second;
                auto& frame = processor.frames.front();
                ++processor.forwardedFrameCounter;
                processor.processor->hasOutput(frame);
                processor.frames.pop_front();
            }
        }

        // if there are no frames available to be submitted then we can try to find a stream that
        // has finished
        if (numAvailable == 0 && !mLastFrame)
        {
            int smallestFrameCounter = getSmallestFrameCounter();
            for (auto& idProcessor : mProcessors)
            {
                auto& processor = idProcessor.second;
                if (processor.frameCounter == smallestFrameCounter && processor.eosReceived)
                {
                    mLastFrame = smallestFrameCounter;
                }
            }
        }

        if (mLastFrame)
        {
            // flush all frames before the last frame
            for (auto& idProcessor : mProcessors)
            {
                auto& processor = idProcessor.second;
                while (processor.forwardedFrameCounter < *mLastFrame)
                {
                    auto& frame = processor.frames.front();
                    ++processor.forwardedFrameCounter;
                    processor.processor->hasOutput(frame);
                    processor.frames.pop_front();
                }
                processor.processor->produceEOS();
                if (processor.paused)
                {
                    // unpause the stream; we will ignore future frames though
                    processor.paused = false;
                    processor.processor->setPaused(false);
                }
            }
            mEosSent = true;
        }
        else
        {
            for (auto& idProcessor : mProcessors)
            {
                auto& processor = idProcessor.second;
                bool needsPausing =
                    static_cast<int>(processor.frames.size()) > mConfig.throttleLimit;
                if (needsPausing != processor.paused)
                {
                    processor.paused = needsPausing;
                    processor.processor->setPaused(needsPausing);
                }
            }
        }
    }

    int MediaStepLockState::getSmallestFrameCounter() const
    {
        int smallestFrameCounter = 0;
        bool first = true;
        for (auto& idProcessor : mProcessors)
        {
            int frameCounter = idProcessor.second.frameCounter;
            if (first)
            {
                smallestFrameCounter = frameCounter;
                first = false;
            }
            else
            {
                smallestFrameCounter = std::min(smallestFrameCounter, frameCounter);
            }
        }
        return smallestFrameCounter;
    }

    int MediaStepLockState::getNumberOfAvailableFrames() const
    {
        int smallestFramesAvailable = 0;
        bool first = true;
        for (auto& idProcessor : mProcessors)
        {
            int framesAvailable = static_cast<int>(idProcessor.second.frames.size());
            if (first)
            {
                smallestFramesAvailable = framesAvailable;
                first = false;
            }
            else
            {
                smallestFramesAvailable = std::min(smallestFramesAvailable, framesAvailable);
            }
        }
        return smallestFramesAvailable;
    }


    MediaStepLockProcessor::MediaStepLockProcessor(GraphBase& aGraphBase,
                                                   int aId,
                                                   std::shared_ptr<MediaStepLockState> aState)
        : AsyncProcessor(aGraphBase, "MediaStepLockProcessor")
        , mId(aId)
        , mState(aState)
    {
        // nothing
    }

    void MediaStepLockProcessor::setPaused(bool aPaused)
    {
        //std::cout << getInfo() << " " << (aPaused ? "paused" : "resumed") << std::endl;
        mPaused = aPaused;
    }

    bool MediaStepLockProcessor::isBlocked() const
    {
        return mPaused;
    }

    void MediaStepLockProcessor::produceEOS()
    {
        std::vector<Data> eos;
        for (auto streamId : mKnownStreams)
        {
            eos.push_back(Data(EndOfStream(), streamId));
        }
        hasOutput({eos.begin(), eos.end()});
        setInactive();
    }

    void MediaStepLockProcessor::hasInput(const Streams& aStreams)
    {
        // We never expect hasInput to be called if it has received endOfStream
        assert(!mEncounteredEndOfStream);
        for (const Data& data : aStreams)
        {
            mKnownStreams.insert(data.getStreamId());
        }
        if (aStreams.isEndOfStream())
        {
            // we expect all the input streams to end at the same time and once
            for (const Data& data : aStreams)
            {
                mActiveStreams.erase(data.getStreamId());
            }
            assert(mActiveStreams.size() == 0);

            mEncounteredEndOfStream = true;
        }
        else
        {
            for (const Data& data : aStreams)
            {
                mActiveStreams.insert(data.getStreamId());
            }
        }

        mState->submit(mId, aStreams);
    }

    MediaStepLock::MediaStepLock(GraphBase& aGraph, const Config& aConfig)
        : mGraph(aGraph), mState(std::make_shared<MediaStepLockState>(aConfig))
    {
        // nothing
    }

    std::unique_ptr<MediaStepLockProcessor>
    MediaStepLock::get()
    {
        return mState->newState(mGraph, mState);
    }
}