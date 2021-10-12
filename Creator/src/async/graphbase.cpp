
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
#include <fstream>

#include "asyncnode.h"
#include "graphbase.h"

#include "common/utils.h"

namespace VDD
{
    const int cWrapWidth = 40;

    namespace {
        std::string stringOfStreamFilter(StreamFilter aStreamFilter)
        {
            std::ostringstream st;
            if (aStreamFilter.allStreams())
            {
                st << "all";
            }
            else
            {
                bool first = true;
                for (auto streamId: aStreamFilter.asSet())
                {
                    if (!first)
                    {
                        st << ",";
                    }
                    first = false;
                    st << to_string(streamId);
                }
            }
            return st.str();
        }

        // For graphviz. Slow but easy.
        std::string quote(const std::string& aString)
        {
            std::string s = aString;
            s = Utils::replace(s, "\"", "\\\"");
            s = Utils::replace(s, "\n", "\\n");
            return s;
        }

        // For graphviz.
        std::string wordWrap(int aWidth, const std::string& aString)
        {
            const bool wrapLongWords = true;
            using index = std::string::difference_type;
            std::string res;
            index cutPoint = 0;
            bool skipCutPoint = false;
            index prevCut = 0;
            auto result = std::back_inserter(res);
            auto begin = aString.begin();
            auto end = aString.end();
            // why use iterators only? so we don't need to deal with signed vs unsigned..
            for (index i = 0; i <= end - begin; ++i)
            {
                auto at = begin + i;
                bool inString = at < end;
                char ch = inString ? *at : 0;
                if (ch == ' ' || ch == '/' || ch == ',')
                {
                    cutPoint = i;
                    skipCutPoint = ch == ' ';
                }
                if (inString && i - prevCut >= aWidth && (wrapLongWords || cutPoint != prevCut))
                {
                    if (cutPoint != prevCut)
                    {
                        // if we want to not put the cut point in, we cut it from the left side.
                        // no, this won't work nicely for skipping consecutive skipped cut points
                        if (!skipCutPoint)
                        {
                            ++cutPoint;
                        }
                        std::copy(begin + prevCut, begin + cutPoint, result);
                        if (skipCutPoint)
                        {
                            ++cutPoint;
                        }
                        prevCut = cutPoint;
                    }
                    else
                    {
                        std::copy(begin + prevCut, begin + i, result);
                        cutPoint = i;
                        prevCut = cutPoint;
                    }
                    *result++ = '\n';
                }
                else if (ch == '\n' || !inString)
                {
                    std::copy(begin + prevCut, begin + i + (inString ? 1 : 0), result);
                    cutPoint = i + 1;
                    prevCut = cutPoint;
                    skipCutPoint = false;
                }
            }
            return res;
        }
    }

    NoActiveSourcesException::NoActiveSourcesException()
        : Exception("NoActiveSourcesException")
    {
        // nothing
    }

    GraphStopGuard::GraphStopGuard(GraphBase& aGraph) : mGraph(aGraph)
    {
        // nothing
    }

    GraphStopGuard::~GraphStopGuard()
    {
        mGraph.stop();
    }

    GraphBase::GraphBase() = default;

    GraphBase::~GraphBase() = default;

    void GraphBase::stop()
    {
        // nothing
    }

    void GraphBase::setErrorSignaled()
    {
        mErrorSignaled = true;
    }

    bool GraphBase::hasErrorSignaled() const
    {
        return mErrorSignaled;
    }

    size_t GraphBase::numActiveNodes() const
    {
        return mActiveNodes.size();
    }

    void GraphBase::graphviz(std::ostream& aStream) const
    {
        aStream << "digraph {" << std::endl;
        aStream << "overlap = false;" << std::endl;
        aStream << "rankdir = LR;" << std::endl;
        for (const AsyncNode* node: mNodes)
        {
            std::string additional = additionalGraphvizInformation(node);
            std::string info = node->getInfo() + (additional.size() ? "\n" + additional : "");
            aStream << node->mId << " [ shape=box";
            if (auto color = node->getColor()) {
                aStream << " color=" << *color;
                aStream << " penwidth=4";
            }
            aStream << " label=\"" << quote(wordWrap(cWrapWidth, info)) << "\"];" << std::endl;
        }
        for (auto& node: mGraphvizNodes)
        {
            aStream << node.first << " [ shape=oval label=\"" << quote(wordWrap(cWrapWidth, node.second)) << "\"];" << std::endl;
        }
        for (const AsyncNode* node: mNodes)
        {
            for (const AsyncCallback& neighbour: node->mCallbacks)
            {
                aStream << node->mId << " -> " << neighbour.processor->mId;
                aStream << " [label=\"" << quote(wordWrap(cWrapWidth, neighbour.label));
                if (!neighbour.streamFilter.allStreams())
                {
                    aStream << (neighbour.label.size() ? "\\n" : "") << "["
                            << quote(wordWrap(cWrapWidth, stringOfStreamFilter(neighbour.streamFilter)))
                            << "]";
                }
                aStream << "\"";
                auto color = node->getColor();
                if (!color)
                {
                    color = neighbour.processor->getColor();
                }
                if (color)
                {
                    aStream << " color=" << *color;
                }
                aStream << "];" << std::endl;
            }
        }
        for (auto& edge: mGraphvizEdges)
        {
            aStream << edge.first << " -> " << edge.second << ";" << std::endl;
        }
        aStream << "}" << std::endl;
    }

    void GraphBase::graphviz(const char* aFilename) const
    {
        std::ofstream out(aFilename);
        graphviz(out);
    }

    void GraphBase::addGraphvizNode(std::string aId, std::string aLabel)
    {
        mGraphvizNodes.insert(std::make_pair(aId, aLabel));
    }

    void GraphBase::addGraphvizEdge(std::string aFrom, std::string aTo)
    {
        mGraphvizEdges.insert(std::make_pair(aFrom, aTo));
    }

    void GraphBase::addGraphvizEdge(const AsyncNode& aFrom, std::string aTo)
    {
        addGraphvizEdge(Utils::to_string(aFrom.mId), aTo);
    }

    void GraphBase::addGraphvizEdge(std::string aFrom, const AsyncNode& aTo)
    {
        addGraphvizEdge(aFrom, Utils::to_string(aTo.mId));
    }

    void GraphBase::addGraphvizEdge(const AsyncNode& aFrom, const AsyncNode& aTo)
    {
        addGraphvizEdge(Utils::to_string(aFrom.mId), Utils::to_string(aTo.mId));
    }

    void GraphBase::registerNode(AsyncNode* aNode)
    {
        mNodes.insert(aNode);
        if (aNode->isActive())
        {
            mActiveNodes.insert(aNode);
        }
    }

    void GraphBase::unregisterNode(AsyncNode* aNode)
    {
        mNodes.erase(aNode);
        mActiveNodes.erase(aNode);
    }

    void GraphBase::registerSource(AsyncSource* aSource)
    {
        mSources.insert(aSource);
    }

    void GraphBase::unregisterSource(AsyncSource* aSource)
    {
        mSources.erase(aSource);
    }

    void GraphBase::setNodeInactive(AsyncNode* aNode)
    {
        mActiveNodes.erase(aNode);
    }

    const AsyncNode* GraphBase::findNodeById(AsyncNodeId aNodeId) const
    {
        const AsyncNode* found = nullptr;
        for (auto it = mNodes.begin(); !found && it != mNodes.end(); ++it)
        {
            if ((*it)->mId == aNodeId)
            {
                found = *it;
            }
        }
        return found;
    }

    AsyncNode* GraphBase::findNodeById(AsyncNodeId aNodeId)
    {
        AsyncNode* found = nullptr;
        for (auto it = mNodes.begin(); !found && it != mNodes.end(); ++it)
        {
            if ((*it)->mId == aNodeId)
            {
                found = *it;
            }
        }
        return found;
    }


    void GraphBase::replaceConnectionsTo(AsyncProcessor& aNodeToReplace, AsyncProcessor& aReplacement)
    {
        if (&aNodeToReplace != &aReplacement)
        {
            for (auto* node : mNodes)
            {
                for (AsyncCallback& callback : getNodeCallbacks(node))
                {
                    if (callback.processor == &aNodeToReplace)
                    {
                        callback.processor = &aReplacement;
                    }
                }
            }
        }
    }

    void GraphBase::replaceConnectionsFrom(AsyncProcessor& aNodeToReplace, AsyncProcessor& aReplacement)
    {
        if (&aNodeToReplace != &aReplacement)
        {
            auto& origCallbacks = getNodeCallbacks(&aNodeToReplace);
            getNodeCallbacks(&aReplacement) = std::move(origCallbacks);
            origCallbacks.erase(origCallbacks.begin(), origCallbacks.end());
        }
    }

    void GraphBase::eliminate(AsyncProcessor& aNodeToEliminate)
    {
        std::list<AsyncCallback>& eliminateCallbacks = getNodeCallbacks(&aNodeToEliminate);
        assert(mNodes.count(&aNodeToEliminate));

        for (auto* node : mNodes)
        {
            std::list<AsyncCallback>& newCallbacks = getNodeCallbacks(node);
            std::list<AsyncCallback> connectedToNode;
            std::list<AsyncCallback> notConnectedToNode;
            std::partition_copy(
                newCallbacks.begin(), newCallbacks.end(), std::back_inserter(connectedToNode),
                std::back_inserter(notConnectedToNode),
                [&](AsyncCallback& callback) { return callback.processor == &aNodeToEliminate; });

            if (connectedToNode.size())
            {
                // Preserve connections that were not related to the node-to-be-removed
                newCallbacks = notConnectedToNode;

                for (AsyncCallback& callback: connectedToNode)
                {
                    for (auto eliminatedCallback: eliminateCallbacks)
                    {
                        eliminatedCallback.streamFilter &= callback.streamFilter;
                        if (eliminatedCallback.label.size() && callback.label.size())
                        {
                            eliminatedCallback.label += " & ";
                        }
                        eliminatedCallback.label += callback.label;
                        newCallbacks.push_back(eliminatedCallback);
                    }
                }
            }
        }

        eliminateCallbacks = {};

        mOwnNodes.remove_if([&](std::unique_ptr<AsyncNode>& node) {
            // remove (and release as it is unique_ptr) the node if we own it
            return node.get() == &aNodeToEliminate;
        });

        mNodes.erase(&aNodeToEliminate);
        mActiveNodes.erase(&aNodeToEliminate);
    }

    std::list<AsyncCallback>& GraphBase::getNodeCallbacks(AsyncNode* aNode)
    {
        return aNode->mCallbacks;
    }

    const std::list<AsyncCallback>& GraphBase::getNodeCallbacks(AsyncNode* aNode) const
    {
        return aNode->mCallbacks;
    }

    const std::set<AsyncNode*>& GraphBase::getActiveNodes()
    {
        return mActiveNodes;
    }

    const std::set<AsyncNode*>& GraphBase::getNodes()
    {
        return mNodes;
    }

    const std::set<AsyncSource*>& GraphBase::getSources()
    {
        return mSources;
    }

    AsyncCallback& connect(AsyncNode& aFrom, const std::string& aName, AsyncPushCallback aCallback, StreamFilter aStreamFilter)
    {
        GraphBase& graph = aFrom.getGraph();
        std::unique_ptr<AsyncFunctionSink> sink(new AsyncFunctionSink(graph, aName, aCallback));
        auto& x = connect(aFrom, *sink, aStreamFilter);
        graph.giveOwnership(std::move(sink));
        return x;
    }

    AsyncCallback& connect(AsyncNode& aFrom, AsyncProcessor& aTo, StreamFilter aStreamFilter)
    {
        return aFrom.addCallback(aTo, aStreamFilter);
    }

    void replaceConnectionsTo(AsyncProcessor& aNodeToReplace, AsyncProcessor& aReplacement)
    {
        GraphBase& graph = aNodeToReplace.getGraph();
        graph.replaceConnectionsTo(aNodeToReplace, aReplacement);
    }

    void replaceConnectionsFrom(AsyncProcessor& aNodeToReplace, AsyncProcessor& aReplacement)
    {
        GraphBase& graph = aNodeToReplace.getGraph();
        graph.replaceConnectionsFrom(aNodeToReplace, aReplacement);
    }

    void eliminate(AsyncProcessor& aNodeToEliminate)
    {
        GraphBase& graph = aNodeToEliminate.getGraph();
        graph.eliminate(aNodeToEliminate);
    }
}
