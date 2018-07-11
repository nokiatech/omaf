
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
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <chrono>

#include "parallelgraph.h"

#include "common/utils.h"

namespace VDD
{
    ParallelGraph::ParallelGraph(const Config& aConfig)
        : mConfig(aConfig)
    {
        // nothing
    }

    ParallelGraph::~ParallelGraph()
    {
        std::unique_lock<std::mutex> lock(mWorkMutex);
        mQuit = true;
        mWorkAvailable.notify_all();
        lock.unlock();

        for (auto& thread: mThreads)
        {
            thread.join();
        }
    }

    void ParallelGraph::workerThread(unsigned aThreadId)
    {
        try
        {
            (void) aThreadId;
            bool quit = false;
            // keep workLock at this level to remove the useless lock/unlock between loop iterations
            std::unique_lock<std::mutex> workLock(mWorkMutex);
            do
            {
                mWorkAvailable.wait(workLock, [&]{ return mAborted || mQuit || mNodeAge.size(); });

                // quit only if there's no work left or an abort has been issued
                quit = (mNodeAge.size() == 0 && mQuit) || mAborted;

                if (!quit)
                {
                    // pick a node with oldest data to process
                    auto nodeAgeIt = mNodeAge.begin();
                    auto nodeIt = nodeAgeIt->second.begin();
                    auto node = *nodeIt;
                    auto& nodeInfo = *mNodeInfo[node];

                    // and remove it from the age list (so we won't end up picking next time a node
                    // that doesn't have work waiting activation)
                    nodeAgeIt->second.erase(nodeIt);
                    if (nodeAgeIt->second.size() == 0)
                    {
                        mNodeAge.erase(nodeAgeIt);
                    }
                    workLock.unlock();

                    std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);

                    // if it is running or its outputs are blocked, we cannot do a thing. but what
                    // happens to the activation? we need to ensure that whomever unblocks the
                    // nodeinfo will also add the nodeinfo to mNodeAges
                    if (!nodeInfo.areOutputsBlocked() &&
                        !nodeInfo.running &&
                        nodeInfo.nodeHasWork())
                    {
                        nodeInfo.running = true;
                        double t0 = Utils::getSeconds();
                        std::exception_ptr exception;

                        // remove all work submitted to a terminated node
                        if (nodeInfo.terminated)
                        {
                            nodeInfo.enqueued.clear();
                        }
                        else
                        {
                            // do a complete flush to enqueued items in one go (this is a part that can
                            // result in more than 1 elements being in queue of nodes). to ensure that the
                            // node is never running when it has no enqueued items, the queued element is
                            // removed only after running the node.

                            while (nodeInfo.enqueued.size() > 0 && !nodeInfo.terminated)
                            {
                                auto& views = nodeInfo.enqueued.front();

                                nodeInfoLock.unlock();

                                try
                                {
                                    nodeInfo.processor->hasInput(views);
                                }
                                // other exceptions are just passed forward (and they indicate a
                                // bug) resulting likely in a crash to debug
                                catch (Exception&)
                                {
                                    exception = std::current_exception();
                                }
                                nodeInfoLock.lock();

                                if (exception)
                                {
                                    nodeInfo.terminated = true;
                                }

                                nodeInfo.enqueued.pop_front();
                            }
                        }

                        double t1 = Utils::getSeconds();

                        nodeInfo.runtime += t1 - t0;

                        // we don't update oldestEnqueuedData, because its value is not used until
                        // the node is added back to mNodeAge.

                        std::list<AsyncNode*> wake = checkAndDecrementParentBlockedOutputs(nodeInfo);

                        nodeInfo.running = false;
                        // note: locking workLock before releasing nodeInfoLock. this is safe
                        // because there are only two places it's done at and it is always done in
                        // this order
                        workLock.lock();
                        // mNumWaiting needs to be decremented and checked here, as its state
                        // depends on nodeInfo
                        --mNumWaiting;

                        // this replaces assert(!nodeInfo.nodeHasWork());
                        if (nodeInfo.nodeHasWork() && !nodeInfo.terminated)
                        {
                            std::cerr << "ParallelGraph: Interesting." << std::endl;
                        }

                        assert(mNumWaiting >= 0);
                        nodeInfoLock.unlock();

                        if (exception)
                        {
                            mExceptions.push_back({node, std::move(exception)});
                        }

                        wakeWithWorkMutexHeld(wake);

                        ++mReadySequence;
                        mWorkReady.notify_one();
                    }
                    else
                    {
                        workLock.lock();
                    }
                }
            } while (!quit);
        }
        catch (std::runtime_error& exn)
        {
            std::cerr << "Thread received an unhandled exception: " << exn.what() << std::endl;
        }
    }

    void ParallelGraph::nodeHasInput(AsyncProcessor* aNode, const Views& aViews)
    {
        auto& nodeInfo = *mNodeInfo[aNode];
        std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);

        ++nodeInfo.numInputs;
        if (nodeInfo.terminated)
        {
            // ignore this work
            return;
        }

        assert(aViews.size() > 0);

        bool nodeHadWork = nodeInfo.nodeHasWork();
        nodeInfo.enqueued.push_back(aViews);

        // NOTE! To avoid deadlock, the locking of multiple nodes must always happen child node first!
        if (nodeInfo.isNodeOverEmployed() && !nodeInfo.setParentBlocked)
        {
            for (auto& parent: nodeInfo.parents)
            {
                std::unique_lock<std::mutex> parentNodeInfoLock(parent->nodeInfoMutex);
                assert(nodeInfo.numBlockedOutputs >= 0);
                // std::cout << "++" << parent->node->getId() << "(" << parent->numBlockedOutputs << "->" << parent->numBlockedOutputs + 1 << ")" << std::endl;
                ++parent->numBlockedOutputs;
            }
            nodeInfo.setParentBlocked = true;
        }

        // node was previously unemployed, but now it has work
        if (!nodeHadWork && nodeInfo.nodeHasWork())
        {
            // note: locking mWorkReady while nodeInfoLock is already locked. This should not result
            // in deadlock as we do it only in two places, always in this order, and while the lock
            // is being held we don't perform any kind of waiting
            std::unique_lock<std::mutex> workLock(mWorkMutex);
            // mNumWaiting needs to be checked and incremented here, as its state depends on
            // nodeInfo. moreover, it is decremented elsewhere and asserted to be positive, so
            // this must be in sync with that.
            assert(mNumWaiting >= 0);
            ++mNumWaiting;
            nodeInfoLock.unlock();

            // needs to be called because
            updateNodeAgeWithWorkMutexHeld(aNode, aViews[0].getCommonFrameMeta().presIndex);
            wakeWithWorkMutexHeld(aNode);
        }
    }

    void ParallelGraph::updateNodeAgeWithWorkMutexHeld(const AsyncNode* aNode, Index aIndex)
    {
        auto& nodeInfo = *mNodeInfo[aNode];
        auto oldIndex = nodeInfo.oldestEnqueuedData.load();
        mNodeAge[oldIndex].erase(aNode);
        if (mNodeAge[oldIndex].size() == 0)
        {
            mNodeAge.erase(oldIndex);
        }
        nodeInfo.oldestEnqueuedData.store(aIndex);
    }

    void ParallelGraph::wakeWithWorkMutexHeld(const AsyncNode* aNode)
    {
        auto& nodeInfo = *mNodeInfo[aNode];
        mNodeAge[nodeInfo.oldestEnqueuedData.load()].insert(aNode);
        mWorkAvailable.notify_one();
    }

    void ParallelGraph::wakeWithWorkMutexHeld(const std::list<AsyncNode*>& wake)
    {
        for (auto& wakeNode: wake)
        {
            wakeWithWorkMutexHeld(wakeNode);
        }
    }

    void ParallelGraph::nodeHasOutput(AsyncNode* aNode, const Views& aViews)
    {
        ++mNodeInfo[aNode]->numOutputs;
        for (auto& callback: getNodeCallbacks(aNode))
        {
            if (callback.viewMask == allViews)
            {
                nodeHasInput(callback.processor, aViews);
            }
            else
            {
                Views callbackViews;
                size_t viewIndex = 0;
                size_t mask = callback.viewMask;
                while (mask)
                {
                    if (mask & 1)
                    {
                        callbackViews.push_back(aViews[viewIndex]);
                    }
                    mask >>= 1;
                    ++viewIndex;
                }
                nodeHasInput(callback.processor, callbackViews);
            }
        }
    }

    void ParallelGraph::buildNodeInfo()
    {
        mNodeInfoBuilt = true;

        for (AsyncNode* node: getNodes())
        {
            auto n = Utils::make_unique<NodeInfo>();

            n->node = node;
            if (auto processor = dynamic_cast<AsyncProcessor*>(node))
            {
                n->processor = processor;
            }

            mById.insert({ n->node->getId(), n.get() });
            mNodeInfo.emplace(std::make_pair(node, std::move(n)));
        }

        for (AsyncNode* parentNode: getNodes())
        {
            for (auto& callback: getNodeCallbacks(parentNode))
            {
                auto& nodeInfo = *mNodeInfo[callback.processor];
                nodeInfo.parents.push_back(mNodeInfo[parentNode].get());
                ++mNodeInfo[parentNode]->numOutputNodes;
            }
        }

        for (AsyncNode* node: getNodes())
        {
            node->graphStarted();
        }
    }

    std::list<AsyncNode*> ParallelGraph::checkAndDecrementParentBlockedOutputs(NodeInfo& aNodeInfo)
    {
        std::list<AsyncNode*> wake;
        if (!aNodeInfo.isNodeOverEmployed() && aNodeInfo.setParentBlocked)
        {
            aNodeInfo.setParentBlocked = false;

            // this node used to be blocked (== have enqueued items), but it is no
            // longer. We can reduce the parents' numBlockedOutputs by one.
            for (auto& parent : aNodeInfo.parents)
            {
                // NOTE! To avoid deadlock, the locking of multiple nodes must always happen child node first!
                std::unique_lock<std::mutex> parentNodeInfoLock(parent->nodeInfoMutex);
                // std::cout << node->getId() << ": " << "--node(" << parent->node->getId() << ") (" << parent->numBlockedOutputs << "->" << parent->numBlockedOutputs - 1 << ")" << std::endl;
                --parent->numBlockedOutputs;
                assert(parent->numBlockedOutputs >= 0);

                // if the outputs of the parent node are not blocked and there is work
                // available for the parent, wake the parent up
                if (!parent->areOutputsBlocked() && parent->nodeHasWork())
                {
                    wake.push_back(parent->node);
                }
            }
        }
        return wake;
    }

    const ParallelGraph::NodeInfo& ParallelGraph::nodeInfoFor(int aId) const
    {
        return *mById.at(aId);
    }

    bool ParallelGraph::step(GraphErrors& aErrors)
    {
        // Reading mAborted by ParallelGraph does not need to be synchronzied, because only
        // ParallelGraph writes to it in the first place (from the same thread)
        if (mAborted)
        {
            return false;
        }

        if (!mNodeInfoBuilt)
        {
            buildNodeInfo();
        }

        if (!mThreads.size())
        {
            unsigned numberOfThreads = std::max(1u, std::thread::hardware_concurrency());
            for (unsigned threadId = 0; threadId < numberOfThreads; ++threadId)
            {
                mThreads.emplace_back([this, threadId]{ workerThread(threadId); });
            }
        }

        for (AsyncNode* node : getNodes())
        {
            auto& nodeInfo = *mNodeInfo[node];
            std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);
            if (!nodeInfo.running)
            {
                nodeInfo.isInternallyBlocked = node->isBlocked();
            }
            else
            {
                nodeInfo.isInternallyBlocked = false;
            }

            auto parentsToWake = checkAndDecrementParentBlockedOutputs(nodeInfo);
            if (parentsToWake.size())
            {
                nodeInfoLock.unlock();
                std::unique_lock<std::mutex> workLock(mWorkMutex);
                wakeWithWorkMutexHeld(parentsToWake);
            }
        }

        bool anyActive = false;
        // TODO: still all the source nodes are run in sequence. This probably doesn't matter too
        // much.
        for (AsyncSource* source : getSources())
        {
            if (source->isActive())
            {
                anyActive = true;

                auto& nodeInfo = *mNodeInfo[source];

                bool unblocked;
                {
                    std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);
                    unblocked = !nodeInfo.areOutputsBlocked();

                    assert(nodeInfo.numBlockedOutputs >= 0);

                    // is here a small window where numBlockedOutputs could get increased? regardless it
                    // doesn't matter, as we are able to queue any number of data.
                }

                if (unblocked)
                {
                    double t0 = Utils::getSeconds();
                    source->produce();
                    double t1 = Utils::getSeconds();
                    nodeInfo.runtime += t1 - t0;
                }
            }
        }

        // wait for at least one job to unblock since the before issuing the work, or
        // until there are no more blocked items
        bool keepWorking;
        bool needsAbort;
        {
            std::unique_lock<std::mutex> workLock(mWorkMutex);
            // mExceptions doesn't need to be waited here, as it is also signaled by mReadySequence
            mWorkReady.wait_for(workLock, std::chrono::milliseconds(5000),
                                [&]{ return mReadySequence > mStepReadySequence || mNumWaiting == 0; });
            mStepReadySequence = mReadySequence;
            assert(mNumWaiting >= 0);
            keepWorking = anyActive || mNumWaiting > 0;

            needsAbort = mExceptions.size() > 0;

            for (auto& ptr: mExceptions)
            {
                try
                {
                    std::rethrow_exception(ptr.second);
                }
                // these are the only type of exceptions we store in the first place
                catch (Exception& exn)
                {
                    GraphError error;
                    error.message =
                        "Exception " + std::string(exn.what()) + " from node " +
                        Utils::replace(ptr.first->getInfo(), "\n", " ") +
                        ": "  + exn.message();
                    error.exception = std::current_exception();

                    aErrors.push_back(error);
                }
            }
            mExceptions.clear();
        }

        if (needsAbort)
        {
            abort();
        }

        if (mConfig.enablePerformanceLogging)
        {
            performanceLogging();
        }

        return keepWorking;
    }

    void ParallelGraph::abort()
    {
        for (AsyncSource* source: getSources())
        {
            source->abort();
        }

        std::unique_lock<std::mutex> workLock(mWorkMutex);
        mAborted = true;
        mWorkAvailable.notify_all();
    }

    void ParallelGraph::performanceLogging()
    {
        auto now = Utils::getSeconds();
        if (!mPrevSnapShot)
        {
            mStarted = now;
        }
        auto time = now - mStarted;
        if (now - mLastSnapshot > 0.1)
        {
            mLastSnapshot = now;
            auto perfSnapshot = perfInfoSnapshot();

            if (!mPerfGeneral)
            {
                mPerfGeneral = Utils::make_unique<std::ofstream>("general.csv");
                *mPerfGeneral << "time"
                              << ";" << "count"
                              << ";" << "size"
                              << std::endl;
            }

            {
                auto& allocs = getGlobalDataAllocations();
                *mPerfGeneral << time
                              << ";" << allocs.count
                              << ";" << allocs.size << std::endl;
            }

            if (mPrevSnapShot)
            {
                for (auto& nodeAndPerf : perfSnapshot)
                {
                    auto& curPerf = nodeAndPerf.second;
                    auto& oldPerf = mPrevSnapShot->at(nodeAndPerf.first);

                    std::cerr << (curPerf.running
                                  ? "@"
                                  : (curPerf.numInputs != oldPerf.numInputs
                                     ? (curPerf.numOutputs != oldPerf.numOutputs ? "=" : "<")
                                     : (curPerf.numOutputs != oldPerf.numOutputs ? ">" : ".")));
                }
                std::cerr << std::endl;
            }
            if (!mPrevSnapShot)
            {
                mStarted = now;
                std::ofstream catalogue("perf.csv");
                catalogue << "file;info" << std::endl;
                for (auto& nodeAndPerf : perfSnapshot)
                {
                    const AsyncNode* node = nodeAndPerf.first;
                    mPerfCsv.emplace(std::make_pair(node, std::ofstream(Utils::to_string(node->getId()) + ".csv")));
                    std::list<std::string> fields =
                        { "time", "runtime", "queueLength", "running", "numBlockedOutputs",
                          "cachedOldestEnqueuedData", "setParentBlocked",
                          "numInputs", "numOutputs", "queuedSourceLength", "newestQueuedSource" };
                    bool firstField = true;
                    for (auto field: fields)
                    {
                        if (!firstField)
                        {
                            mPerfCsv[node] << ";";
                        }
                        mPerfCsv[node] << node->getId() << " " << field;
                        firstField = false;
                    }
                    mPerfCsv[node] << std::endl;
                    catalogue << node->getId() << ".csv;" << Utils::replace(node->getInfo(), "\n", " ") << std::endl;
                }
            }

            for (auto& nodeAndPerf : perfSnapshot)
            {
                const AsyncNode* node = nodeAndPerf.first;
                const NodePerfInfo& perfInfo = nodeAndPerf.second;
                mPerfCsv[node] << time;
                mPerfCsv[node].precision(3);
                mPerfCsv[node] << std::fixed;

                if (mPrevSnapShot)
                {
                    const NodePerfInfo& prevPerfInfo = (*mPrevSnapShot)[node];
                    mPerfCsv[node] << ";" << perfInfo.runtime - prevPerfInfo.runtime;
                }
                else
                {
                    mPerfCsv[node] << ";" << perfInfo.runtime;
                }
                mPerfCsv[node] << ";" << perfInfo.queueLength;
                mPerfCsv[node] << ";" << perfInfo.running;
                mPerfCsv[node] << ";" << perfInfo.numBlockedOutputs;
                mPerfCsv[node] << ";" << perfInfo.cachedOldestEnqueuedData;
                mPerfCsv[node] << ";" << perfInfo.setParentBlocked;
                mPerfCsv[node] << ";" << perfInfo.numInputs;
                mPerfCsv[node] << ";" << perfInfo.numOutputs;
                mPerfCsv[node] << std::endl;
            }
            mPrevSnapShot = perfSnapshot;
        }
    }

    std::string ParallelGraph::additionalGraphvizInformation(const AsyncNode* aNode) const
    {
        std::ostringstream st;

        if (mNodeInfoBuilt)
        {
            const NodeInfo& nodeInfo = *mNodeInfo.find(aNode)->second;
            std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);

            st << std::fixed << std::setprecision(3);
            st << "runtime:" << nodeInfo.runtime;
            st << "\n" << "inputs:" << nodeInfo.numInputs;
            st << "\n" << "outputs:" << nodeInfo.numOutputs;
        }

        return st.str();
    }

    ParallelGraph::NodePerfInfos ParallelGraph::perfInfoSnapshot()
    {
        NodePerfInfos perfInfos;

        for (auto& nodeAndInfo: mNodeInfo)
        {
            NodePerfInfo info;
            const NodeInfo& nodeInfo = *nodeAndInfo.second;
            std::unique_lock<std::mutex> nodeInfoLock(nodeInfo.nodeInfoMutex);
            info.runtime = nodeInfo.runtime;
            info.queueLength = nodeInfo.enqueued.size();
            info.running = nodeInfo.running;
            info.numBlockedOutputs = nodeInfo.numBlockedOutputs;
            info.cachedOldestEnqueuedData = nodeInfo.oldestEnqueuedData.load();
            info.setParentBlocked = nodeInfo.setParentBlocked;
            info.numInputs = nodeInfo.numInputs;
            info.numOutputs = nodeInfo.numOutputs;
            perfInfos[nodeAndInfo.first] = info;
        }
        return perfInfos;
    }

    size_t ParallelGraph::numActiveNodes() const
    {
        std::unique_lock<std::mutex> activeNodesLock(mActiveNodesMutex);
        return GraphBase::numActiveNodes();
    }

    void ParallelGraph::setNodeInactive(AsyncNode* mNode)
    {
        std::unique_lock<std::mutex> activeNodesLock(mActiveNodesMutex);
        GraphBase::setNodeInactive(mNode);
    }

    const std::set<AsyncNode*>& ParallelGraph::getActiveNodes()
    {
        std::unique_lock<std::mutex> activeNodesLock(mActiveNodesMutex);
        return GraphBase::getActiveNodes();
    }
}
