
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
#include "common/utils.h"

#include "combinenode.h"
#include "graph.h"

namespace VDD {
    namespace {
        size_t combineIndex = 0; // for generating names
    }

    CombineSinkNode::CombineSinkNode(GraphBase& aGraph, std::shared_ptr<CombineSourceNode> aCombineSourceNode, size_t aIndex)
        : AsyncProcessor(aGraph)
        , mCombineSourceNode(aCombineSourceNode)
        , mIndex(aIndex)
    {
        // nothing
    }

    CombineSinkNode::~CombineSinkNode() = default;

    void CombineSinkNode::hasInput(const Views& aViews)
    {
        Views views;
        bool end = true;
        mCombineSourceNode->addFrame(mIndex, aViews);
        if (end)
        {
            setInactive();
        }
    }

    bool CombineSinkNode::isBlocked() const
    {
        return mCombineSourceNode->isSourceBlocked(mIndex);
    }

    CombineSourceNode::CombineSourceNode(GraphBase& aGraph)
        : AsyncSource(aGraph)
    {
        // nothing
    }

    void CombineSourceNode::addInput()
    {
        // Make a slot in mFrames for the new input
        mFrames.push_back({});
    }

    CombineSourceNode::~CombineSourceNode()
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        bool anyFramesAvailable = false;
        for (size_t index = 0; index < mFrames.size(); ++index)
        {
            auto& frames = mFrames.at(index);
            // We ignore the finished-state of inputs, as long as no actual data has been lost
            anyFramesAvailable = anyFramesAvailable || frames.size() > 0;
        }
        // non-even number of frames were received, thus they weren't passed on
        assert(!anyFramesAvailable);
    }

    void CombineSourceNode::produce()
    {
        size_t numFramesAvailable;
        size_t numFinished;
        bool keepPurging;
        do
        {
            Views views;
            numFramesAvailable = 0;
            numFinished = 0;
            std::unique_lock<std::mutex> framesLock(mFramesMutex);

            for (size_t index = 0; index < mFrames.size(); ++index)
            {
                auto& frames = mFrames.at(index);
                if (frames.size())
                {
                    ++numFramesAvailable;
                }
                else if (mFinished.count(index))
                {
                    ++numFinished;
                }
            }

            // something is available from all sinks
            keepPurging = numFinished < mFrames.size() && numFramesAvailable + numFinished == mFrames.size();
            if (keepPurging)
            {
                // if any of the inputs have frames available, don't send EndOfStream yet, but wait
                // for the next call to produce
                for (size_t index = 0; index < mFrames.size(); ++index)
                {
                    auto& frames = mFrames.at(index);
                    assert(frames.size() || mFinished.count(index));
                    if (frames.size())
                    {
                        // takes only the first view of the frame
                        for (auto& view: frames.front())
                        {
                            views.push_back(view);
                        }
                        frames.pop_front();
                    }
                    else if (mFinished.count(index))
                    {
                        // Use empty data as a place holder for finished streams
                        views.push_back(Data());
                    }
                }

                framesLock.unlock();
                hasOutput(views);
                framesLock.lock();
            }
        } while (keepPurging);

        if (numFinished == mFrames.size())
        {
            Views views;
            for (const auto& finished: mFinished) {
                for (StreamId streamId: finished.second) {
                    views.push_back({ EndOfStream(), streamId });
                }
            }
            hasOutput(views);
            setInactive();
        }
    }

    void CombineSourceNode::addFrame(size_t aIndex, const Views& aViews)
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        if (aViews[0].isEndOfStream())
        {
            for (const auto& view: aViews) {
                mFinished[aIndex].insert(view.getStreamId());
            }
        }
        else
        {
            mFrames[aIndex].push_back(aViews);
        }
    }

    bool CombineSourceNode::isSourceBlocked(size_t aIndex)
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        return mFrames[aIndex].size() >= 1;
    }

    void CombineSourceNode::abort()
    {
        // nothing
    }

    CombineNode::CombineNode(GraphBase& aGraph)
        : mGraph(aGraph)
        , mId(++combineIndex)
    {
        mSource = std::shared_ptr<CombineSourceNode>(new CombineSourceNode(mGraph));
        mSource->setName("combineSource(" + Utils::to_string(mId) + ")");
    }

    CombineNode::~CombineNode()
    {
        // nothing
    }

    std::unique_ptr<CombineSinkNode> CombineNode::getSink(size_t index)
    {
        mSource->addInput();
        auto sink = std::unique_ptr<CombineSinkNode>(new CombineSinkNode(mGraph, mSource, index));
        sink->setName("combineSink(" + Utils::to_string(mId) + ", " + Utils::to_string(index) + ")");
        mGraph.addGraphvizEdge(*sink, *mSource);
        return sink;
    }

    std::shared_ptr<CombineSourceNode> CombineNode::getSource()
    {
        return mSource;
    }
}
