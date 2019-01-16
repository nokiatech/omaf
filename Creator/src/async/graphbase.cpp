
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
#include <sstream>

#include "asyncnode.h"
#include "graphbase.h"

#include "common/utils.h"

namespace VDD
{
    namespace {
        std::string stringOfViewMask(ViewMask aViewMask)
        {
            std::ostringstream st;
            size_t viewIndex = 0;
            size_t mask = aViewMask;
            bool first = true;
            while (mask)
            {
                if (mask & 1)
                {
                    if (!first)
                    {
                        st << ",";
                    }
                    first = false;
                    st << viewIndex;
                }
                mask >>= 1;
                ++viewIndex;
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
    }

    NoActiveSourcesException::NoActiveSourcesException()
        : Exception("NoActiveSourcesException")
    {
        // nothing
    }

    GraphBase::GraphBase() = default;

    GraphBase::~GraphBase() = default;

    size_t GraphBase::numActiveNodes() const
    {
        return mActiveNodes.size();
    }

    void GraphBase::graphviz(std::ostream& aStream) const
    {
        aStream << "digraph {" << std::endl;
        aStream << "rankdir = LR;" << std::endl;
        for (const AsyncNode* node: mNodes)
        {
            std::string additional = additionalGraphvizInformation(node);
            std::string info = node->getInfo() + (additional.size() ? "\n" + additional : "");
            aStream << node->mId << " [ shape=box label=\"" << quote(info) << "\"];" << std::endl;
        }
        for (auto& node: mGraphvizNodes)
        {
            aStream << node.first << " [ shape=oval label=\"" << quote(node.second) << "\"];" << std::endl;
        }
        for (const AsyncNode* node: mNodes)
        {
            for (const AsyncCallback& neighbour: node->mCallbacks)
            {
                aStream << node->mId << " -> " << neighbour.processor->mId;
                if (neighbour.viewMask != allViews)
                {
                    aStream << " [label=\"[" << stringOfViewMask(neighbour.viewMask) << "]\"]" << std::endl;
                }
                aStream << ";" << std::endl;
            }
        }
        for (auto& edge: mGraphvizEdges)
        {
            aStream << edge.first << " -> " << edge.second << ";" << std::endl;
        }
        aStream << "}" << std::endl;
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

    const std::list<AsyncCallback>& GraphBase::getNodeCallbacks(AsyncNode* aNode)
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

    void connect(AsyncNode& aFrom, const std::string& aName, AsyncPushCallback aCallback, ViewMask aViewMask)
    {
        GraphBase& graph = aFrom.getGraph();
        std::unique_ptr<AsyncFunctionSink> sink(new AsyncFunctionSink(graph, aName, aCallback));
        connect(aFrom, *sink, aViewMask);
        graph.giveOwnership(std::move(sink));
    }

    void connect(AsyncNode& aFrom, AsyncProcessor& aTo, ViewMask aViewMask)
    {
        aFrom.addCallback(aTo, aViewMask);
    }
}
