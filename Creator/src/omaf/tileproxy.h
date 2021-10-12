
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

#include <memory>
#include <mutex>
#include <vector>

#include "async/future.h"
#include "async/asyncnode.h"
#include "processor/data.h"
#include "common/rational.h"
#include "common/optional.h"
#include "tileconfig.h"


namespace VDD {
    class TileProxySourceNode;
    class TileProxySinkNode;

    class TileProxy
    {
    public:
        struct Config
        {
            size_t tileCount = 0;
            StreamAndTrackIds extractors;
            TileMergeConfig tileMergingConfig;
            PipelineOutput outputMode;
        };
        TileProxy(Config config);
        virtual ~TileProxy() = default;

    protected:

        virtual bool extractorDataCollectionReady() const;
        virtual bool extractorReadyForEoS() const;
        virtual void processExtractors(std::vector<Streams>& aTiles);
        virtual void createEoS(std::vector<Streams>& aTiles);
        virtual CPUDataVector createExtractorSEI(VDD::Optional<RegionPacking> aRegionPacking, unsigned int aTemporalIdPlus1);

    private:
        using SinkId = size_t;

        Data collectExtractors();

        /** @brief The functions below are used by these two classes */
        friend class TileProxySourceNode;
        friend class TileProxySinkNode;

        /** @brief Submit data for processing */
        void submit(const Streams& aData, SinkId aSinkId);

        /** @brief A way for TileProxySinkNode to tell which tile streams are incoming. This allows
         * detecting when input streams are finished.

         @see TileProxySinkNode::TileProxySinkNode
         @see TileProxyConnector::getSink */
        void addTileStreamIds(const std::set<StreamId> aStreamIds);

        /** @brief Move mTiles contents to the return value and atomically indicate if the stream is
         * now finished */
        std::vector<Streams> moveTiles(bool& aIsEndOfStream);

        /** @brief These streams are full enough, no need to fill in. Used for determining blocking
         * status.
         */
        std::set<StreamId> fullTiles() const;

        /** @brief Get an id for a sink. Used for pacing the sink frame rates. */
        SinkId getNextSinkId();

        /** @brief Output buffer is full. Used for determining blocking status. */
        bool isOutputFull() const;

        /** @brief Is this sink ahead others? If so, it should be blocked, otherwise combinenodes ahead will cause
            trouble (deadlock). */
        bool isAhead(SinkId aSinkId) const;

    protected:
        mutable std::mutex mMutex; // protects mTiles, mExtractorCache, mAvailable*

        TileMergeConfig mTileMergingConfig;
        size_t mTileCount;

        //cache for extractors
        struct ViewData {
            VDD::Data data;
            bool endOfStream;
        };
        std::map<StreamId, std::list<ViewData>> mExtractorCache;
        bool mExtractorSEICreated = false;
        PipelineOutput mOutputMode;

    private:
        StreamId mExtractorStreamId;
        TrackId mExtractorTrackId;

        /** @brief Waiting to be produced by TileProxySourceNode. @see moveTiles */
        std::vector<Streams> mTiles;

        /** @brief Input tile stream IDs, collected dwith addTileStreamIds */
        std::set<StreamId> mTileStreamIds;
        /** @brief The tile stream IDs that have delivede EndOfStream */
        std::set<StreamId> mFinishedTileStreams;
        bool mExtractorFinished = false; // set after calling createEoS

        // SinkInfo is used for pausing sinks that are too far ahead
        struct SinkInfo
        {
            Index latestCodingIndex = 0;
        };
        SinkId mNextSinkId = 0;
        std::map<SinkId, SinkInfo> mSinkInfo;
    };

    class TileProxySinkNode : public AsyncProcessor
    {
    public:
        TileProxySinkNode(GraphBase& aGraph, std::shared_ptr<TileProxy> aTileProxy,
                          const std::set<StreamId>& aStreamIds);

        void hasInput(const Streams& aInput) override;
        bool isBlocked() const override;

    private:
        std::shared_ptr<TileProxy> mTileProxy;
        std::set<StreamId> mStreamIds;
        std::set<StreamId> mActiveStreams;
        size_t mSinkId;
    };

    class TileProxySourceNode : public AsyncSource
    {
    public:
        TileProxySourceNode(GraphBase& aGraph, std::shared_ptr<TileProxy> aTileProxy);

        void produce() override;

        void abort() override
        {
            /* This abort is actually inert. To terminate TileProxySourceNode arrange all its
             * inputs to provide end-of-stream to all relevant streams. */
        }

    private:
        std::shared_ptr<TileProxy> mTileProxy;
    };

    class TileProxyConnector
    {
    public:
        TileProxyConnector(GraphBase& aGraph, std::unique_ptr<TileProxy> aTileProxy);
        ~TileProxyConnector() = default;

        /** @briedf Construct a sink to the TileProxy.

            aStreamIds ids required so sinks know when those particular ids are blocking and
           finished.
        */
        TileProxySinkNode* getSink(const std::set<StreamId>& aStreamIds);

        /** @brief Access the source node pointer. The source is created upon the first call and is
           automatically inserted to the mGraph for storage.  */
        TileProxySourceNode* getSource();

    private:
        GraphBase& mGraph;
        std::shared_ptr<TileProxy> mTileProxy;
        TileProxySourceNode* mSource = nullptr;
    };
}
