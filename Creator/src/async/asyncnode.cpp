
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

    AsyncNode::AsyncNode(GraphBase& aGraph, std::string aName)
        : mGraph(aGraph)
        , mId(++nodeIndexGenerator)
        , mName(aName)
    {
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
        return mName + "." + Utils::to_string(mId) + (isActive() ? "*" : "");
    }

    void AsyncNode::graphStarted()
    {
        // nothing
    }

    void AsyncNode::setName(std::string aName)
    {
        mName = aName;
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

    void AsyncNode::hasOutput(const Views& aViews)
    {
        mGraph.nodeHasOutput(this, aViews);
        ++mOutputCounter;
    }

    void AsyncNode::addCallback(AsyncProcessor& aCallback, ViewMask aViewMask)
    {
        mCallbacks.push_back({ &aCallback, aViewMask });
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

    void AsyncFunctionSink::hasInput(const Views& aViews)
    {
        HoldTrue holder(mRunning);
        bool end = aViews[0].isEndOfStream();
        mCallback(aViews);
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

    void AsyncForwardProcessor::hasInput(const Views& aViews)
    {
        HoldTrue holder(mRunning);
        bool end = aViews[0].isEndOfStream();
        hasOutput(aViews);
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
        // nothing
    }

    AsyncProcessorWrapper::AsyncProcessorWrapper(GraphBase& aGraph, std::string aName, Processor* aProcessor)
        : AsyncProcessor(aGraph, aName)
        , mProcessor(std::unique_ptr<Processor>(aProcessor))
    {
        // nothing
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

    void AsyncProcessorWrapper::hasInput(const Views& aViews)
    {
        HoldTrue holder(mRunning);
        for (Views& views: mProcessor->process(aViews))
        {
            if (views[0].isEndOfStream())
            {
                setInactive();
            }
            hasOutput(views);
        }
    }

    AsyncSinkWrapper::AsyncSinkWrapper(GraphBase& aGraph, std::string aName, std::unique_ptr<Sink>&& aSink)
        : AsyncProcessor(aGraph, aName)
        , mSink(std::move(aSink))
    {
        // nothing
    }

    AsyncSinkWrapper::AsyncSinkWrapper(GraphBase& aGraph, std::string aName, Sink* aSink)
        : AsyncProcessor(aGraph, aName)
        , mSink(std::unique_ptr<Sink>(aSink))
    {
        // nothing
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

    void AsyncSinkWrapper::hasInput(const Views& aViews)
    {
        HoldTrue holder(mRunning);
        bool end = aViews[0].isEndOfStream();
        mSink->consume(aViews);
        if (end)
        {
            setInactive();
        }
    }

    AsyncSourceWrapper::AsyncSourceWrapper(GraphBase& aGraph, std::string aName, std::unique_ptr<Source>&& aSource)
        : AsyncSource(aGraph, aName)
        , mSource(std::move(aSource))
    {
        // nothing
    }

    AsyncSourceWrapper::AsyncSourceWrapper(GraphBase& aGraph, std::string aName, Source* aSource)
        : AsyncSource(aGraph, aName)
        , mSource(std::unique_ptr<Source>(aSource))
    {
        // nothing
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
        for (const Views& views: mSource->produce())
        {
            if (views[0].isEndOfStream())
            {
                setInactive();
            }
            hasOutput(views);
        }
    }

    void AsyncSourceWrapper::abort()
    {
        mSource->abort();
    }
}
