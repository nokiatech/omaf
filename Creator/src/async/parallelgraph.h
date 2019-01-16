
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

#include <algorithm>
#include <exception>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <set>
#include <map>
#include <chrono>

#include "common/optional.h"
#include "graphbase.h"

namespace VDD {
    class ParallelGraph : public GraphBase
    {
    public:
        struct Config {
            bool enablePerformanceLogging;
            bool enableDebugDump;
        };

        ParallelGraph(const Config& config);
        virtual ~ParallelGraph();

        /** call .produce() on all AsyncSources */
        bool step(GraphErrors& aErrors) override;

        void abort() override;

        size_t numActiveNodes() const override;

    protected:
        void nodeHasOutput(AsyncNode* aNode, const Views& aViews) override;

        void setNodeInactive(AsyncNode* mNode) override;
        const std::set<AsyncNode*>& getActiveNodes() override;

        std::string additionalGraphvizInformation(const AsyncNode*) const override;

    private:
        Config mConfig;

        void nodeHasInput(AsyncProcessor* aNode, const Views& aViews);

        // A node didn't have data before, but now it does, and this is its time index. Ensures the
        // old data is purged from mNodeAge, however does not write new one because
        // wakeWithWorkMutexHeld is to called after this. (But wakeWithWorkMutexHeld can be called
        // without calling updateNodeAgeWithWorkMutexHeld if the Indxe doesn't need to be updated.)
        void updateNodeAgeWithWorkMutexHeld(const AsyncNode* aNode, Index aIndex);

        // Adds a node to the wake up list. Sorry for the long name, but you must have the mutex held!
        void wakeWithWorkMutexHeld(const AsyncNode* aNode);
        void wakeWithWorkMutexHeld(const std::list<AsyncNode*>& wake);

        struct NodeInfo;
        /** If the given node is now unblocked, decrements numBlockedOutputs of given nodes; return
         * the nodes that were to be woken after this */
        std::list<AsyncNode*> checkAndDecrementParentBlockedOutputs(NodeInfo& aNodeInfo);

        std::list<std::thread> mThreads;

        bool mQuit = false;

        // protects numActiveNodes, setNodeInactive, getActiveNodes
        mutable std::mutex mActiveNodesMutex;

        // protects mNodeAge, mExceptions, mNumWaiting, mReadySequence, mAborted
        std::mutex mWorkMutex;
        std::condition_variable mWorkAvailable;
        std::condition_variable mWorkReady; // also signals new mExceptions entries
        std::list<std::pair<const AsyncNode*, std::exception_ptr>> mExceptions;

        void workerThread(unsigned aThreadId);

        struct NodeInfo
        {
            // Protects the whole NodeInfo struct for this node
            mutable std::mutex nodeInfoMutex;

            // Keep a copy here so we don't need to deal with std::pair too much and the parents
            // field is simplier
            AsyncNode* node = nullptr;

            // ..with a different type to avoid expensive dynamic casts during work
            AsyncProcessor* processor = nullptr;

            // Sources are handled specially in areOutputsBlocked
            bool isSource = false;

            // Number of blocked outputs; can never be < 0
            int numBlockedOutputs = 0;

            // Number of outputs; assigned during the buildNodeInfo
            int numOutputNodes = 0;

            // Was parent set blocked? So we can ensure we unblock it only in this case.
            bool setParentBlocked = false;

            // Is this node being processed right now? Then it doesn't need a worker, because it
            // will finish its job.
            bool running = false;

            // A flag set by the step do indicate whether a node is internally blocked (its isBlocked-flag)
            bool isInternallyBlocked = false;

            // Data waiting
            std::list<Views> enqueued;

            // Tell the Index of the oldest data in the qqueue, without need to use mutex
            std::atomic<Index> oldestEnqueuedData = {};

            // Parent nodes; used for updating the numBlockedOutputs of them
            std::list<NodeInfo*> parents;

            // number of seconds this node has been running
            double runtime = 0.0;

            // set when the node is terminated by a exception
            bool terminated = false;

            // number of input invocations; for debugging
            std::atomic<size_t> numInputs;

            // number of outputs produced by node; for debugging
            std::atomic<size_t> numOutputs;

            bool areOutputsBlocked() const {
                return
                    isSource
                    ? numBlockedOutputs >= 1
                    : numBlockedOutputs >= std::max(1, numOutputNodes);
            }
            bool nodeHasWork() const { return enqueued.size() > 0; }
            bool isNodeOverEmployed() const { return enqueued.size() >= 1 || isInternallyBlocked; }
        };

        // mNodeInfo is initialized once and not then modified (because new nodes are not added
        // while running the graph), so it doesn't need to be protected with a mutex
        std::map<const AsyncNode*, std::unique_ptr<NodeInfo>> mNodeInfo;
        std::map<int, NodeInfo*> mById; // purely for debugging
        bool mNodeInfoBuilt = false;

        const NodeInfo& nodeInfoFor(int) const; //purely for debugging

        int mNumWaiting = 0; // number of nodes with views enqueued
        size_t mReadySequence = 0; // incremented when a node becomes unblocked without views enqueued
        size_t mStepReadySequence = 0; // what was mReadySequence the last time we stepped? used to determine progress and throttle ::step.
        bool mAborted = false;

        // The way to find work, with the node with oldest node. Do ensure that a node never ends up
        // in this map more than once.
        std::map<Index, std::set<const AsyncNode*>> mNodeAge;

        // snapshot of node performance information
        struct NodePerfInfo
        {
            double runtime;
            size_t queueLength;
            bool running;
            int numBlockedOutputs;
            Index cachedOldestEnqueuedData;
            bool setParentBlocked;
            size_t numInputs;
            size_t numOutputs;
        };
        using NodePerfInfos = std::map<const AsyncNode*, NodePerfInfo>;

        NodePerfInfos perfInfoSnapshot();
        void performanceLogging();

        double mStarted = 0.0;
        double mLastSnapshot = 0.0;
        Optional<NodePerfInfos> mPrevSnapShot;
        std::unique_ptr<std::ofstream> mPerfGeneral;
        std::map<const AsyncNode*, std::ofstream> mPerfCsv;

        void buildNodeInfo();

        struct DebugPrevNodeInfo {
            size_t in;
            size_t out;
        };
        void debugDump();
        std::string mPrevDebugDump;
        std::chrono::time_point<std::chrono::steady_clock> mPrevDebugDumpTime;
        std::map<int, DebugPrevNodeInfo> mDebugPrevNodeInfo;
    };
}
