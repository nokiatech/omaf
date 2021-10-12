
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

#include <functional>
#include <iostream>
#include <list>
#include <set>
#include <atomic>

#include "common/exceptions.h"
#include "processor/streams.h"
#include "processor/data.h"
#include "processor/processor.h"
#include "processor/sink.h"
#include "processor/source.h"
#include "common/optional.h"

/**
 * GraphBase and AsyncNode enable an asynchronous graph where data flows from one node to another. While
 * the actual data is currently forwarded synchronously, asynchronous functionality can be
 * introduced without changing this API.
 */

namespace VDD {
    class Log;

    using AsyncPushCallback = std::function<void(const Streams& streams)>;

    class GraphBase;

    class AsyncNode;
    class AsyncSource;
    struct AsyncCallback;

    class AsyncProcessor;

    struct AsyncCallback {
        AsyncProcessor* processor;
        StreamFilter streamFilter;
        std::string label; // for annotating graph
        AsyncCallback& setLabel(std::string aLabel);
    };

    typedef unsigned int AsyncNodeId;

    /** @brief AsyncNode is the basic element in the graph. It can
     * produce output by calling its hasOutput, which passes the data
     * forward to functions registered with addCallback.
     */
    class AsyncNode
    {
    public:
        struct ScopeColored
        {
            ScopeColored(std::string aColor)
            {
                AsyncNode::pushDefaultColor(aColor);
            }
            ScopeColored(const ScopeColored&) = delete;
            void operator=(const ScopeColored&) = delete;
            ~ScopeColored()
            {
                AsyncNode::popDefaultColor();
            }
        };

        static std::string debugPrefix;

        AsyncNode(GraphBase&, std::string aName = "node");
        AsyncNode(const AsyncNode&) = delete;
        void operator=(const AsyncNode&) = delete;

        AsyncNodeId getId() const;

        virtual std::string getInfo() const;
        void setName(std::string aName);
        std::string getName() const;

        // For debugging in the GraphViz graph
        static void pushDefaultColor(Optional<std::string> aColor);
        static void popDefaultColor();
        Optional<std::string> getColor() const;
        void setColor(Optional<std::string> aColor);

        GraphBase& getGraph();

        /* Is this node busy or blocked, even though it's not running actively. Will not be called
         * concurrently with hasInput. */
        virtual bool isBlocked() const;

        void setInactive();
        bool isActive() const;

        // Indicate that the graph has started. After this callback the node may call hasOutput.
        virtual void graphStarted();

        virtual void setLog(std::shared_ptr<Log> aLog);

        virtual ~AsyncNode();

    protected:
        friend AsyncCallback& connect(AsyncNode& aFrom, AsyncProcessor& aTo, StreamFilter aStreamFilter);
        AsyncCallback& addCallback(AsyncProcessor& aCallback, StreamFilter aStreamFilter);

        void hasOutput(const Streams& streams);

    private:
        GraphBase& mGraph;
        AsyncNodeId mId; // used for debug output
        std::string mName; // used for debug output
        std::list<AsyncCallback> mCallbacks;
        bool mActive = true;
        size_t mOutputCounter = 0;

        static std::list<Optional<std::string>> sDefaultColor;
        Optional<std::string> mColor;

        friend class GraphBase;
    };

    class AsyncSource : public AsyncNode
    {
    public:
        AsyncSource(GraphBase&, std::string aName = "source");
        ~AsyncSource() override;

        // (maybe) produce output by calling hasOutput
        virtual void produce() = 0;

        // arrange the next 'produce' to produce end-of-stream
        virtual void abort() = 0;
    };

    class AsyncProcessor : public AsyncNode
    {
    public:
        AsyncProcessor(GraphBase&, std::string aName = "processor");
        ~AsyncProcessor();

        virtual void hasInput(const Streams& streams) = 0;
    };

    /** @brief Used for converting functions to AsyncNodes for the purposes
        of connecting ie. debug output */
    class AsyncFunctionSink : public AsyncProcessor
    {
    public:
        AsyncFunctionSink(GraphBase& aGraph,
                          std::string aName,
                          const AsyncPushCallback& aCallback);

        void hasInput(const Streams& streams) override;

    private:
        std::atomic<bool> mRunning = {};
        AsyncPushCallback mCallback;
    };


    // Used to replacing parts in a pipeline with no-op passthrough or implementing self-wired async
    // nodes
    class AsyncForwardProcessor : public AsyncProcessor
    {
    public:
        AsyncForwardProcessor(GraphBase&, std::string aName = "forward");
        ~AsyncForwardProcessor();

        void hasInput(const Streams& streams) override;

    private:
        std::atomic<bool> mRunning = {};
    };

    /** @brief Given an existing Processor, make it an asynchronous processor */
    class AsyncProcessorWrapper : public AsyncProcessor {
    public:
        AsyncProcessorWrapper(GraphBase&, std::string aName, std::unique_ptr<Processor>&& aProcessor);
        AsyncProcessorWrapper(GraphBase&, std::string aName, Processor* aProcessor); // takes ownership
        AsyncProcessorWrapper(GraphBase&, std::string aName); // setProcessor MUST be called before hasInput

        /** @brief Ensure that hasInput cannot be running at the same time. This can be arranged
         * with data dependencies in the graph, ie. so that the node calling this method if before
         * this node in the graph.
         */
        void setProcessor(std::unique_ptr<Processor> aProcessor);
        void hasInput(const Streams& streams) override;
        void setLog(std::shared_ptr<Log> aLog) override;

        std::string getInfo() const override;

        ~AsyncProcessorWrapper();

    private:
        std::atomic<bool> mRunning = {};
        std::unique_ptr<Processor> mProcessor;
        std::shared_ptr<Log> mLog;
    };

    /** @brief Given an existing Processor, make it an asynchronous processor */
    class AsyncSinkWrapper : public AsyncProcessor {
    public:
        AsyncSinkWrapper(GraphBase&, std::string aName, std::unique_ptr<Sink>&& aSink);
        AsyncSinkWrapper(GraphBase&, std::string aName, Sink* aSink); // takes ownership
        AsyncSinkWrapper(GraphBase&, std::string aName); // setSink MUST be called before hasInput

        /** @brief Ensure that hasInput cannot be running at the same time. This can be arranged
         * with data dependencies in the graph, ie. so that the node calling this method if before
         * this node in the graph.
         */
        void setSink(std::unique_ptr<Sink> aSink);
        void hasInput(const Streams& streams) override;
        void setLog(std::shared_ptr<Log> aLog) override;

        std::string getInfo() const override;

        ~AsyncSinkWrapper();

    private:
        std::atomic<bool> mRunning = {};
        std::unique_ptr<Sink> mSink;
        std::shared_ptr<Log> mLog;
    };

    /** @brief Given an existing Processor, make it an asynchronous processor */
    class AsyncSourceWrapper : public AsyncSource {
    public:
        AsyncSourceWrapper(GraphBase&, std::string aName, std::unique_ptr<Source>&& aSource);
        AsyncSourceWrapper(GraphBase&, std::string aName, Source* aSource); // takes ownership

        void produce() override;
        void abort() override;
        void setLog(std::shared_ptr<Log> aLog) override;

        std::string getInfo() const override;

        ~AsyncSourceWrapper();

    private:
        std::unique_ptr<Source> mSource;
    };

    // A mechanism to detect if a given class inherits frmo Source, Processor and Sink, and then
    // provide the appropriate wrapper for that class.
    template <typename T, typename = void>
    struct AsyncNodeWrapperTraits;

    template <typename T>
    struct AsyncNodeWrapperTraits<T, typename std::enable_if<std::is_base_of<Source, T>::value>::type>
    {
        using Wrapper = AsyncSourceWrapper;
    };

    template <typename T>
    struct AsyncNodeWrapperTraits<T, typename std::enable_if<std::is_base_of<Processor, T>::value>::type>
    {
        using Wrapper = AsyncProcessorWrapper;
    };

    template <typename T>
    struct AsyncNodeWrapperTraits<T, typename std::enable_if<std::is_base_of<Sink, T>::value>::type>
    {
        using Wrapper = AsyncSinkWrapper;
    };
};
