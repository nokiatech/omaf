
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
#pragma once

#include <mutex>

#include "asyncnode.h"

namespace VDD {
    /**
     * CombineNode is a mechanism for combining two or more sources of data into one. For example,
     * if there are two YUV importing sources each producing one separate Views with one Data each,
     * CombineNode is able to combine those two frames into one frame that has two views.
     *
     * CombineNode doesn't require the frames to be produced at the same time. When one frame is
     * provided, it waits until the other source produces a frame as well, and then combines them
     * and passes them on in the same order as it received them.
     *
     *  Import --> CombineSinkNode\
     *                             >--> CombineSourceNode --> Processing
     *  Import --> CombineSinkNode/
     *
     * CombineNode is built out of three classes:
     * - the controlling class CombineNode which is not itself hierarchically part of the AsyncNode
     *   framework
     * - the sink class CombineSinkNode, which is part of the AsyncNode hierarchy by inheriting
     *   from AsyncProcessor, and is created (only) by an instance of CombineNode
     * - the source class CombineSourceNode, which inherits from AsyncSource.
     *
     * The usage is as follows
     * - Instantiate CombineNode with the configuration: number of inputs (you are expected
     *   to create as many Sinks as you create inputs)
     * - Create as many CombineSinkNodes as you specified with the getSink(size_t index) method
     *   and connect your source/processing nodes to these sinks
     * - Get one CombineSourceNode from using the getSource() method and connect the node
     *   to your further sink/proecssing nodes
     *
     * You don't to keep the CombineNode object around after setup.
     */

    class CombineNode;
    class CombineSourceNode;

    class CombineSinkNode : public AsyncProcessor
    {
    public:
        ~CombineSinkNode();

        void hasInput(const Views& input) override;
        bool isBlocked() const override;

    private:
        CombineSinkNode(GraphBase&, std::shared_ptr<CombineSourceNode> aCombineSourceNode, size_t aIndex);

        std::shared_ptr<CombineSourceNode> mCombineSourceNode;
        size_t mIndex; // which output slot to occupy in the final output from CombineSourceNode

        friend class CombineNode;
    };

    class CombineSourceNode : public AsyncSource
    {
    public:
        ~CombineSourceNode();

        void produce() override;

        /* @brief This abort is actually inert. To terminate CombineSourceNode arrange all its
         * inputs to provide end-of-stream. */
        void abort() override;

    private:
        CombineSourceNode(GraphBase&);

        std::vector<std::list<Views>> mFrames; // one frame queue per each CombineSinkNode
        std::map<size_t, std::set<StreamId>> mFinished; // which nodes are finished; used to synchronise EndOfStream
        std::mutex mFramesMutex; // protect read/write from/to mFrames

        void addInput();
        void addFrame(size_t aIndex, const Views& aViews);
        bool isSourceBlocked(size_t aIndex);

        friend class CombineNode;
        friend class CombineSinkNode;
    };

    /** @brief CombineNode takes n separate inputs behaving as sink, and once those predefined n
     * input have been received, it produces an output where the individual views are put in the
     * same frame one after another.
     *
     * This allows combining Data from different branches of the graph.
     */
    class CombineNode
    {
    public:
        CombineNode(GraphBase&);

        virtual ~CombineNode();

        /** @brief Get a sink for adding Data to the resulting view with priority aIndex
         * If each input source produces single Data, then aIndex matches the actual index
         * in the output View. But if the sources produce ie. 2 instance of Data, those
         * will be put one after another, so aIndex = 1 will result in a sink that writes
         * to view indices [2..3]. */
        std::unique_ptr<CombineSinkNode> getSink(size_t aIndex);

        /** @brief Get a source for receiving combined Views. The same frames are duplicated to
         * each created source, each call creates a new separate source.
         *
         * Do not call in vain, as if there is nothing to satisfy the combination criteria, Views
         * will be buffered forever probably resulting in great memory consumption. */
        std::shared_ptr<CombineSourceNode> getSource();

    private:
        GraphBase& mGraph;

        size_t mId;

        std::shared_ptr<CombineSourceNode> mSource;

        friend class CombineSourceNode;
        friend class CombineSinkNode;
    };


}
