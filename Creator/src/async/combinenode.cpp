
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

#include "common/utils.h"
#include "omaf/extractor.h"
#include "combinenode.h"
#include "graph.h"

namespace VDD {
    namespace {
        size_t combineIndex = 0; // for generating names
    }

    CombineSinkNode::CombineSinkNode(GraphBase& aGraph,
                                     std::shared_ptr<CombineSourceNode> aCombineSourceNode,
                                     size_t aIndex, bool aRequireSingleStream,
                                     Optional<StreamId> aAssignStreamId)
        : AsyncProcessor(aGraph)
        , mCombineSourceNode(aCombineSourceNode)
        , mIndex(aIndex)
        , mRequireSingleStream(aRequireSingleStream)
        , mAssignStreamId(aAssignStreamId)
    {
        // nothing
    }

    CombineSinkNode::~CombineSinkNode() = default;

    void CombineSinkNode::hasInput(const Streams& aStreams)
    {
        bool end = true;
        assert(!mRequireSingleStream || aStreams.size() == 1);
        if (mAssignStreamId) {
            Streams streams;
            for (auto& data: aStreams) {
                streams.add(data.withStreamId(*mAssignStreamId));
            }
            mCombineSourceNode->addFrame(mIndex, streams);
        } else {
            mCombineSourceNode->addFrame(mIndex, aStreams);
        }
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

    size_t CombineSourceNode::addInput(Optional<StreamId> aStreamId, size_t aIndex)
    {
        assert(!mFrames.count(aIndex));
        mFrames[aIndex] = {};
        mEmpty[aIndex] = Data{};
        if (aStreamId)
        {
            mEmpty[aIndex] = mEmpty[aIndex].withStreamId(*aStreamId);
        }
        return aIndex;
    }

    size_t CombineSourceNode::addInput(StreamId aStreamId)
    {
        return addInput(aStreamId, mFrames.size());
    }

    CombineSourceNode::~CombineSourceNode()
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        bool anyFramesAvailable = false;
        for (auto& [index, frames]: mFrames)
        {
            // We ignore the finished-state of inputs, as long as no actual data has been lost
            anyFramesAvailable = anyFramesAvailable || frames.size() > 0;
        }
        // non-even number of frames were received, thus they weren't passed on
        assert(getGraph().hasErrorSignaled() || !anyFramesAvailable);
    }

    std::string CombineSourceNode::getInfo() const
    {
        std::ostringstream st;

        st << AsyncSource::getInfo();
        st << "\nqueued:";
        for (auto& [index, streams]: mFrames)
        {
            st << " " << index << ":" << streams.size();
        }

        st << "\nfinished: ";
        bool first = true;
        for (auto& [index, fin]: mFinished)
        {
            if (!first)
            {
                st << ", ";
            }
            st << index << ":";
            for (auto& stream : fin)
            {
                st << " " << to_string(stream.getStreamId());
            }
            first = false;
        }

        return st.str();
    }

    void CombineSourceNode::produce()
    {
        size_t numFramesAvailable;
        size_t numFinished;
        bool keepPurging;
        do
        {
            numFramesAvailable = 0;
            numFinished = 0;
            std::unique_lock<std::mutex> framesLock(mFramesMutex);

            for (auto& [index, frames]: mFrames)
            {
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
                Streams streams;
                // if any of the inputs have frames available, don't send EndOfStream yet, but wait
                // for the next call to produce
                for (auto& [index, frames]: mFrames)
                {
                    assert(frames.size() || mFinished.count(index));
                    if (frames.size())
                    {
                        // takes only the first view of the frame
                        for (auto& view: frames.front())
                        {
                            assert(!view.isEndOfStream());
                            streams.add(view);
                        }
                        frames.pop_front();
                    }
                    else if (mFinished.count(index))
                    {
                        // Use empty data as a place holder for finished streams
                        auto empty = mEmpty.find(index);
                        if (empty != mEmpty.end())
                        {
                            streams.add(empty->second);
                        }
                    }
                }

                assert(streams.size());
                framesLock.unlock();
                hasOutput(streams);
                framesLock.lock();
            }
        } while (keepPurging);

        if (numFinished == mFrames.size())
        {
            Streams streams;
            for (const auto& finished: mFinished)
            {
                for (auto& data : finished.second)
                {
                    streams.add(data);
                }
            }
            hasOutput(streams);
            setInactive();
        }
    }

    void CombineSourceNode::addFrame(size_t aIndex, const Streams& aStreams)
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        if (aStreams.isEndOfStream())
        {
            for (const auto& view : aStreams)
            {
                mFinished[aIndex].push_back(view);
            }
        }
        else
        {
            mFrames[aIndex].push_back(aStreams);
        }
    }

    bool CombineSourceNode::isSourceBlocked(size_t aIndex)
    {
        std::unique_lock<std::mutex> framesLock(mFramesMutex);
        /* So you've reached this code maybe because the pipeline is stuck? Well it is doubtful that
         * increasing this number further is then the correct solution. However, the scenario why
         * this number was bumped up from 1 is as follows:
         *
         * - Video source A is a it ahead video source B (at frame 777, B is at fram 774)
         *
         * - TileProducer has therefore produced more frames for A and B (more data objects( and in
         * this case it then means it produces Steams at double the rate, so assumably two per one
         * time unit.
         *
         * - TileProxySink as processed the frames from A for time unit t_B+1 (t_B would be the time
         * stamp of the most recent sample from B)
         *
         * - TileProxySink is now stooed and TileProxySource has processed the frames it has been
         * given and forwarded them
         *
         * - Half of the segmenters have been able to produce 85 segments while the other half has
         * produced 86 segments. The half with 86 segments isn't feeling very accepting, because
         * they are (accordding to the isSourceBlocked condition) one block ahead the others.
         *
         * - (And here is the weak point of the argument, because in principle it should happen so
         * that each CombineSink handles the frames in the buffers in parallel and information about
         * that is the delivered to MPOD writer. Possible reason why this doesn't occur> the node
         * priorization has not and wont't give CombineNode execution time?)
         *
         * - The half that has been able to produce 86 segments would get the aforementioned frame
         * for processing, but it's not able to, because its output is blocked
         *
         * - When output is blocked TileProxySource is no longer processed
         *
         * - Deadlock, because TileProxySrouce cannot deliver older frames to open the lock in
         * CombineNode
         *
         * - This flex in isSourceBlocked enables the time-based synchronization of TileProxySink to
         * work better in the presence of multiple Streams per time unit
         */
        return mFrames[aIndex].size() >= 5;
    }

    void CombineSourceNode::abort()
    {
        // nothing
    }

    const CombineNode::Config CombineNode::sDefaultConfig;

    CombineNode::CombineNode(GraphBase& aGraph, const Config& aConfig)
        : mGraph(aGraph)
        , mConfig(aConfig)
        , mId(++combineIndex)
        , mLabelPrefix(aConfig.labelPrefix)
    {
        mSource = std::shared_ptr<CombineSourceNode>(new CombineSourceNode(mGraph));
        mSource->setName((mLabelPrefix.size() ? mLabelPrefix + "\n" : std::string()) +
                         "combineSource(" + Utils::to_string(mId) +
                         (mConfig.renumberStreams ? ", renumberStreams" : "") +
                         (mConfig.requireSingleStream ? ", requireSingleStream" : "") + ")");
    }

    CombineNode::~CombineNode()
    {
        // nothing
    }

    std::unique_ptr<CombineSinkNode> CombineNode::getSink(size_t aIndex, std::string aLabelPrefix)
    {
        assert(!mMode || *mMode == Mode::Index);
        mMode = Mode::Index;
        Optional<StreamId> assignStreamId;
        if (mConfig.renumberStreams)
        {
            assignStreamId = StreamId(aIndex);
        }
        mSource->addInput(assignStreamId, aIndex);
        auto sink = std::unique_ptr<CombineSinkNode>(new CombineSinkNode(
            mGraph, mSource, aIndex, mConfig.requireSingleStream, assignStreamId));
        sink->setName((mLabelPrefix.size() ? mLabelPrefix + "\n" : std::string()) +
                      (aLabelPrefix.size() ? aLabelPrefix + "\n" : std::string()) + "combineSink(" +
                      Utils::to_string(mId) + ", " + Utils::to_string(aIndex) + ")");
        mGraph.addGraphvizEdge(*sink, *mSource);
        return sink;
    }

    std::unique_ptr<CombineSinkNode> CombineNode::getSink(StreamId aStreamId, std::string aLabelPrefix)
    {
        assert(!mMode || *mMode == Mode::Stream);
        mMode = Mode::Stream;
        assert(mConfig.requireSingleStream); // maybe possible to not require, but let's not think about it now.
        auto index = mSource->addInput(aStreamId);
        auto sink = std::unique_ptr<CombineSinkNode>(
            new CombineSinkNode(mGraph, mSource, index, mConfig.requireSingleStream, {aStreamId}));
        sink->setName((mLabelPrefix.size() ? mLabelPrefix + "\n" : std::string()) +
                      (aLabelPrefix.size() ? aLabelPrefix + "\n" : std::string()) + "combineSink(" +
                      Utils::to_string(mId) + ", StreamId(" + to_string(aStreamId) + "))");
        mGraph.addGraphvizEdge(*sink, *mSource);
        return sink;
    }

    std::shared_ptr<CombineSourceNode> CombineNode::getSource()
    {
        return mSource;
    }
}
