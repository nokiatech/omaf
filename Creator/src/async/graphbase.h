
/**
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018-2019 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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

#include "asyncnode.h"
#include "common/exceptions.h"

/**
 * Graph and AsyncNode enable an asynchronous graph where data flows from one node to another. While
 * the actual data is currently forwarded synchronously, asynchronous functionality can be
 * introduced without changing this API.
 */

namespace VDD {
    using AsyncPushCallback = std::function<void(const Views& views)>;

    class AsyncNode;
    class AsyncSource;
    struct AsyncCallback;

    class NoActiveSourcesException : public Exception
    {
    public:
        NoActiveSourcesException();
    };

    struct GraphError
    {
        std::string message;

        // can be null if the error doesn't originate from an exception
        std::exception_ptr exception;

        GraphError() = default;
    };

    using GraphErrors = std::list<GraphError>;

    class GraphBase
    {
    public:
        GraphBase();
        virtual ~GraphBase();

        /** @brief call .produce() on all AsyncSources. Return true if there was work left.
         * If nodes signaled some errors, they will be returned in aErrors.
         */
        virtual bool step(GraphErrors& aErrors) = 0;

        /** @brief call .abort() on all AsyncSources. This will eventually cause the operations to
         * stop.
         */
        virtual void abort() = 0;

        /* future improvement */
        /* for example job management goes through this */

        /* A place to store nodes that don't otherwise have storage */
        template <typename T> T* giveOwnership(std::unique_ptr<T>&& mNode);

        /* Produce a nice graph you can visualize with http://www.graphviz.org/ */
        void graphviz(std::ostream& aStream) const;

        void addGraphvizNode(std::string aId, std::string aLabel);
        void addGraphvizEdge(std::string aFrom, std::string aTo);
        void addGraphvizEdge(const AsyncNode& aFrom, std::string aTo);
        void addGraphvizEdge(std::string aFrom, const AsyncNode& aTo);
        void addGraphvizEdge(const AsyncNode& aFrom, const AsyncNode& aTo);

        // override this if you need to ensure thread-shafety
        virtual size_t numActiveNodes() const;

        // linear time way to search a node for debugging purposes
        const AsyncNode* findNodeById(AsyncNodeId aNodeId) const;
        AsyncNode* findNodeById(AsyncNodeId aNodeId);

    private:
        /* A way to tell the graph that this node exists in the graph */
        friend class AsyncNode;
        void registerNode(AsyncNode* mNode);
        void unregisterNode(AsyncNode* mNode);

        /* A way to tell the graph that this is an originating node (creates
           data from thin air) and its .produce() should be called */
        friend class AsyncSource;
        void registerSource(AsyncSource* mSource);
        void unregisterSource(AsyncSource* mSource);

        // Node: mOwnNodes must be last so that destruction work properly
        std::set<AsyncNode*> mNodes;
        std::set<AsyncNode*> mActiveNodes;
        std::set<AsyncSource*> mSources;
        std::list<std::unique_ptr<AsyncNode>> mOwnNodes;

        // Nodes that exist purely for visualization purposes
        std::map<std::string, std::string> mGraphvizNodes;
        std::multimap<std::string, std::string> mGraphvizEdges;

    protected:
        // override this if you need to ensure thread-shafety
        virtual void setNodeInactive(AsyncNode* mNode);

        virtual void nodeHasOutput(AsyncNode* aNode, const Views& aViews) = 0;

        // Do you need to provide some additional information to the produce graph?
        virtual std::string additionalGraphvizInformation(const AsyncNode*) const { return ""; }

        const std::list<AsyncCallback>& getNodeCallbacks(AsyncNode* aNode);

        // override this if you need to ensure thread-shafety
        virtual const std::set<AsyncNode*>& getActiveNodes();

        const std::set<AsyncNode*>& getNodes();
        const std::set<AsyncSource*>& getSources();
    };

    template <typename T>
    T* GraphBase::giveOwnership(std::unique_ptr<T>&& mNode)
    {
        T* ptr = mNode.get();
        mOwnNodes.push_back(std::move(mNode));
        return ptr;
    }

    /** Connect output from one node to the input of another */
    void connect(AsyncNode& aFrom, AsyncProcessor& aTo, ViewMask aViewMask = allViews);

    /** Connect output from one node to a function */
    void connect(AsyncNode& aFrom, const std::string& aName, const AsyncPushCallback aCallback, ViewMask aViewMask = allViews);
}
