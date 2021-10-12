
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

#include <mutex>

#include "asyncnode.h"
#include "common/exceptions.h"

namespace VDD {
    /**
     * CombineNode is a mechanism for combining two or more sources of data into one. For example,
     * if there are two YUV importing sources each producing one separate Streams with one Data each,
     * CombineNode is able to combine those two frames into one frame that has two streams.
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

        void hasInput(const Streams& input) override;
        bool isBlocked() const override;

    private:
        CombineSinkNode(GraphBase&, std::shared_ptr<CombineSourceNode> aCombineSourceNode,
                        size_t aIndex, bool aRequireSingleStream,
                        Optional<StreamId> aAssignStreamId);

        std::shared_ptr<CombineSourceNode> mCombineSourceNode;
        size_t mIndex; // which output slot to occupy in the final output from CombineSourceNode
        bool mRequireSingleStream;
        Optional<StreamId> mAssignStreamId;

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

        std::string getInfo() const override;

    private:
        CombineSourceNode(GraphBase&);

        std::map<size_t, std::list<Streams>> mFrames;  // one frame queue per each CombineSinkNode
        std::map<size_t, Data> mEmpty;  // empty placeholder object for cases when we need to pad streams
        std::map<size_t, std::list<Data>>
            mFinished;            // which nodes are finished; used to synchronise EndOfStream
        std::mutex mFramesMutex;  // protect read/write from/to mFrames

        size_t addInput(StreamId aStreamId);
        size_t addInput(Optional<StreamId> aStreamId, size_t aIndex);
        void addFrame(size_t aIndex, const Streams& aStreams);
        bool isSourceBlocked(size_t aIndex);

        friend class CombineNode;
        friend class CombineSinkNode;
    };

    /** @brief CombineNode takes n separate inputs behaving as sink, and once those predefined n
     * input have been received, it produces an output where the individual streams are put in the
     * same frame one after another.
     *
     * This allows combining Data from different branches of the graph.
     */
    class CombineNode
    {
    public:
        struct Config {
            std::string labelPrefix;
            bool renumberStreams = false; // if true, output streams will have stream ids 0, 1, 2, ..
            bool requireSingleStream = false; // if true, a sink will check that a single stream is being provided to it
        };

        static const Config sDefaultConfig;

        CombineNode(GraphBase&, const Config& config = sDefaultConfig);

        virtual ~CombineNode();

        /** @brief Get a sink for adding Data to the resulting view with priority aIndex
         * If each input source produces single Data, then aIndex matches the actual index
         * in the output View. But if the sources produce ie. 2 instance of Data, those
         * will be put one after another, so aIndex = 1 will result in a sink that writes
         * to view indices [2..3].

         @note Only of of the getSink variants can be ever used on a CombineNode
        */
        std::unique_ptr<CombineSinkNode> getSink(size_t aIndex, std::string aLabelPrefix = std::string{});

        /** @brief getSink that automatically filters the input to this sink to be assigned the
            given StreamId when read from the source.

            @note Only of of the getSink variants can be ever used on a CombineNode

            @note Requires Config::requireSingleStream to be true */
        std::unique_ptr<CombineSinkNode> getSink(StreamId aStreamId, std::string aLabelPrefix = std::string{});

        /** @brief Get a source for receiving combined Streams. The same frames are duplicated to
         * each created source, each call creates a new separate source.
         *
         * Do not call in vain, as if there is nothing to satisfy the combination criteria, Streams
         * will be buffered forever probably resulting in great memory consumption. */
        std::shared_ptr<CombineSourceNode> getSource();

    private:
        GraphBase& mGraph;
        Config mConfig;

        enum Mode
        {
            Index,
            Stream
        };
        Optional<Mode> mMode;
        size_t mId;
        std::string mLabelPrefix;

        std::shared_ptr<CombineSourceNode> mSource;

        friend class CombineSourceNode;
        friend class CombineSinkNode;
    };


}
