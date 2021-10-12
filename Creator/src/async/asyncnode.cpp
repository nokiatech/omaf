
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
#include <sstream>

#include "asyncnode.h"
#include "graph.h"

#include "common/utils.h"

namespace VDD
{
    namespace
    {
        AsyncNodeId nodeIndexGenerator = 0;

        class HoldTrue
        {
        public:
            HoldTrue(std::atomic<bool>& aFlag)
                : mFlag(aFlag)
            {
                assert(!mFlag.load());
                mFlag.store(true);
            }
            ~HoldTrue()
            {
                assert(mFlag.load());
                mFlag.store(false);
            }

            HoldTrue(const HoldTrue&) = delete;
            HoldTrue& operator=(const HoldTrue&) = delete;

        private:
            std::atomic<bool>& mFlag;
        };
    }

    std::string AsyncNode::debugPrefix;
    std::list<Optional<std::string>> AsyncNode::sDefaultColor;

    AsyncCallback& AsyncCallback::setLabel(std::string aLabel)
    {
        label = aLabel;
        return *this;
    }

    AsyncNode::AsyncNode(GraphBase& aGraph, std::string aName)
        : mGraph(aGraph)
        , mId(++nodeIndexGenerator)
        , mName(aName)
    {
        if (sDefaultColor.size()) {
            mColor = sDefaultColor.front();
        }
        mGraph.registerNode(this);
    }

    void AsyncNode::setLog(std::shared_ptr<Log>)
    {
        // do nothing by default
    }

    AsyncNodeId AsyncNode::getId() const
    {
        return mId;
    }

    std::string AsyncNode::getInfo() const
    {
        return debugPrefix + mName + "." + Utils::to_string(mId) + (isActive() ? "*" : "");
    }

    Optional<std::string> AsyncNode::getColor() const
    {
        return mColor;
    }

    void AsyncNode::pushDefaultColor(Optional<std::string> aColor)
    {
        sDefaultColor.push_front(aColor);
    }

    void AsyncNode::popDefaultColor()
    {
        sDefaultColor.pop_front();
    }

    void AsyncNode::setColor(Optional<std::string> aColor)
    {
        mColor = aColor;
    }

    void AsyncNode::graphStarted()
    {
        // nothing
    }

    void AsyncNode::setName(std::string aName)
    {
        mName = aName;
    }

    std::string AsyncNode::getName() const
    {
        return mName;
    }

    GraphBase& AsyncNode::getGraph()
    {
        return mGraph;
    }

    AsyncNode::~AsyncNode()
    {
        mGraph.unregisterNode(this);
    }

    void AsyncNode::setInactive()
    {
        mActive = false;
        mGraph.setNodeInactive(this);
    }

    bool AsyncNode::isActive() const
    {
        return mActive;
    }

    bool AsyncNode::isBlocked() const
    {
        return false;
    }

    void AsyncNode::hasOutput(const Streams& aStreams)
    {
        assert(aStreams.begin() != aStreams.end());
        mGraph.nodeHasOutput(this, aStreams);
        ++mOutputCounter;
    }

    AsyncCallback& AsyncNode::addCallback(AsyncProcessor& aCallback, StreamFilter aStreamFilter)
    {
        mCallbacks.push_back({ &aCallback, aStreamFilter, "" });
        return mCallbacks.back();
    }

    AsyncSource::AsyncSource(GraphBase& aGraph, std::string aName)
        : AsyncNode(aGraph, aName)
    {
        getGraph().registerSource(this);
    }

    AsyncSource::~AsyncSource()
    {
        getGraph().unregisterSource(this);
    }

    AsyncProcessor::AsyncProcessor(GraphBase& aGraph, std::string aName)
        : AsyncNode(aGraph, aName)
    {
        // nothing
    }

    AsyncProcessor::~AsyncProcessor() = default;

    AsyncFunctionSink::AsyncFunctionSink(GraphBase& aGraph,
                                         std::string aName,
                                         const AsyncPushCallback& aCallback)
        : AsyncProcessor(aGraph, aName)
        , mCallback(aCallback)
    {
        // nothing
    }

    void AsyncFunctionSink::hasInput(const Streams& aStreams)
    {
        HoldTrue holder(mRunning);
        bool end = aStreams.isEndOfStream();
        mCallback(aStreams);
        if (end)
        {
            setInactive();
        }
    }

    AsyncForwardProcessor::AsyncForwardProcessor(GraphBase& aGraph, std::string aName)
        : AsyncProcessor(aGraph, aName)
    {
        // nothing
    }

    AsyncForwardProcessor::~AsyncForwardProcessor() = default;

    void AsyncForwardProcessor::hasInput(const Streams& aStreams)
    {
        HoldTrue holder(mRunning);
        bool end = aStreams.isEndOfStream();
        hasOutput(aStreams);
        if (end)
        {
            setInactive();
        }
        assert(mRunning.load());
    }

    AsyncProcessorWrapper::AsyncProcessorWrapper(GraphBase& aGraph, std::string aName, std::unique_ptr<Processor>&& aProcessor)
        : AsyncProcessor(aGraph, aName)
        , mProcessor(std::move(aProcessor))
    {
        mProcessor->setId(getId());
        mProcessor->ready();
    }

    AsyncProcessorWrapper::AsyncProcessorWrapper(GraphBase& aGraph, std::string aName, Processor* aProcessor)
        : AsyncProcessor(aGraph, aName)
        , mProcessor(std::unique_ptr<Processor>(aProcessor))
    {
        mProcessor->setId(getId());
        mProcessor->ready();
    }

    AsyncProcessorWrapper::AsyncProcessorWrapper(GraphBase& aGraph, std::string aName)
        : AsyncProcessor(aGraph, aName)
    {
        // nothing
    }

    std::string AsyncProcessorWrapper::getInfo() const
    {
        std::string info = AsyncProcessor::getInfo();
        std::string additional = mProcessor->getGraphVizDescription();
        if (additional.size()) {
            info += "\n";
            info += additional;
        }
        return info;
    }

    void AsyncProcessorWrapper::setLog(std::shared_ptr<Log> aLog)
    {
        mLog = aLog;
        if (mProcessor)
        {
            mProcessor->setLog(aLog);
        }
    }

    AsyncProcessorWrapper::~AsyncProcessorWrapper() = default;

    void AsyncProcessorWrapper::setProcessor(std::unique_ptr<Processor> aProcessor)
    {
        HoldTrue holder(mRunning);
        mProcessor = std::move(aProcessor);
        if (mLog)
        {
            mProcessor->setLog(mLog);
        }
    }

    void AsyncProcessorWrapper::hasInput(const Streams& aStreams)
    {
        HoldTrue holder(mRunning);
        for (Streams& streams: mProcessor->process(aStreams))
        {
            if (streams.isEndOfStream())
            {
                setInactive();
            }
            hasOutput(streams);
        }
    }

    AsyncSinkWrapper::AsyncSinkWrapper(GraphBase& aGraph, std::string aName, std::unique_ptr<Sink>&& aSink)
        : AsyncProcessor(aGraph, aName)
        , mSink(std::move(aSink))
    {
        mSink->setId(getId());
        mSink->ready();
    }

    AsyncSinkWrapper::AsyncSinkWrapper(GraphBase& aGraph, std::string aName, Sink* aSink)
        : AsyncProcessor(aGraph, aName)
        , mSink(std::unique_ptr<Sink>(aSink))
    {
        mSink->setId(getId());
        mSink->ready();
    }

    AsyncSinkWrapper::AsyncSinkWrapper(GraphBase& aGraph, std::string aName)
        : AsyncProcessor(aGraph, aName)
    {
        // nothing
    }

    AsyncSinkWrapper::~AsyncSinkWrapper() = default;

    std::string AsyncSinkWrapper::getInfo() const
    {
        std::string info = AsyncProcessor::getInfo();
        std::string additional = mSink->getGraphVizDescription();
        if (additional.size()) {
            info += "\n";
            info += additional;
        }
        return info;
    }

    void AsyncSinkWrapper::setLog(std::shared_ptr<Log> aLog)
    {
        mLog = aLog;
        if (mSink)
        {
            mSink->setLog(aLog);
        }
    }

    void AsyncSinkWrapper::setSink(std::unique_ptr<Sink> aSink)
    {
        HoldTrue holder(mRunning);
        mSink = std::move(aSink);
        if (mLog)
        {
            mSink->setLog(mLog);
        }
    }

    void AsyncSinkWrapper::hasInput(const Streams& aStreams)
    {
        HoldTrue holder(mRunning);
        bool end = aStreams.isEndOfStream();
        mSink->consume(aStreams);
        if (end)
        {
            setInactive();
        }
    }

    AsyncSourceWrapper::AsyncSourceWrapper(GraphBase& aGraph, std::string aName, std::unique_ptr<Source>&& aSource)
        : AsyncSource(aGraph, aName)
        , mSource(std::move(aSource))
    {
        mSource->setId(getId());
        mSource->ready();
    }

    AsyncSourceWrapper::AsyncSourceWrapper(GraphBase& aGraph, std::string aName, Source* aSource)
        : AsyncSource(aGraph, aName)
        , mSource(std::unique_ptr<Source>(aSource))
    {
        mSource->setId(getId());
        mSource->ready();
    }

    std::string AsyncSourceWrapper::getInfo() const
    {
        std::string info = AsyncSource::getInfo();
        std::string additional = mSource->getGraphVizDescription();
        if (additional.size()) {
            info += "\n";
            info += additional;
        }
        return info;
    }

    void AsyncSourceWrapper::setLog(std::shared_ptr<Log> aLog)
    {
        mSource->setLog(aLog);
    }

    AsyncSourceWrapper::~AsyncSourceWrapper() = default;

    void AsyncSourceWrapper::produce()
    {
        for (const Streams& streams: mSource->produce())
        {
            if (streams.isEndOfStream())
            {
                setInactive();
            }
            hasOutput(streams);
        }
    }

    void AsyncSourceWrapper::abort()
    {
        mSource->abort();
    }
}
