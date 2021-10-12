
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
#include <algorithm>
#include <iterator>
#include <math.h>
#include <sstream>
#include "tileproxy.h"
#include "tileutil.h"
#include "async/graphbase.h"
#include "processor/data.h"
#include "parser/bitstream.hpp"
#include "omafproperties.h"
#include "log/logstream.h"

namespace VDD {

    TileProxy::TileProxy(Config aConfig)
        : mTileMergingConfig(aConfig.tileMergingConfig)
        , mTileCount(aConfig.tileCount)
        , mOutputMode(aConfig.outputMode)
        , mExtractorStreamId(aConfig.extractors.front().first)
        , mExtractorTrackId(aConfig.extractors.front().second)
    {
    }

    void TileProxy::submit(const Streams& input, SinkId aSinkId)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        for (const Data& data : input)
        {
            StreamId streamId = data.getStreamId();
            if (input.isEndOfStream())
            {
                if (mTileStreamIds.count(streamId))
                {
                    assert(mFinishedTileStreams.count(streamId) == 0);
                    mFinishedTileStreams.insert(streamId);
                }

                // and also for the extractor
                while (extractorDataCollectionReady())
                {
                    processExtractors(mTiles);
                }
                if (mExtractorCache.count(streamId))
                {
                    mExtractorCache[streamId].push_back({data, input.isEndOfStream()});
                    if (extractorReadyForEoS())
                    {
                        // as tileproducers won't send any extractor after endOfStream, we should
                        // have now flushed the cache
                        createEoS(mTiles);
                        mExtractorFinished = true;
                    }
                }

                mTiles.push_back({data});
            }
            else
            {
                mSinkInfo[aSinkId].latestCodingIndex = data.getCodedFrameMeta().codingIndex;
                if (data.getCodedFrameMeta().format == CodedFormat::H265Extractor)
                {
                    // expecting only 1 extractor per data in input

                    mExtractorCache[streamId].push_back({data, input.isEndOfStream()});
                    // if we have extractor for all needed streams, output them as one new Data,
                    // with extractor streamId
                    while (extractorDataCollectionReady())
                    {
                        processExtractors(mTiles);
                    }
                }
                else
                {
                    // not an extractor, just forward as-is
                    mTiles.push_back({data});
                }
            }
        }
    }

    void TileProxy::addTileStreamIds(const std::set<StreamId> aStreamIds)
    {
        for (auto streamId : aStreamIds)
        {
            assert(!mTileStreamIds.count(streamId));
            mTileStreamIds.insert(streamId);
        }
    }

    std::vector<Streams> TileProxy::moveTiles(bool& aIsEndOfStream)
    {
        std::vector<Streams> tiles;
        std::unique_lock<std::mutex> lock(mMutex);
        std::swap(tiles, mTiles);
        aIsEndOfStream = mFinishedTileStreams.size() == mTileStreamIds.size() && mExtractorFinished;
        return tiles;
    }

    std::set<StreamId> TileProxy::fullTiles() const
    {
        std::set<StreamId> streamIds;
        std::unique_lock<std::mutex> lock(mMutex);
        for (auto& item : mExtractorCache)
        {
            if (item.second.size() > 0)
            {
                streamIds.insert(item.first);
            }
        }
        return streamIds;
    }

    bool TileProxy::isOutputFull() const
    {
        std::unique_lock<std::mutex> lock(mMutex);
        return mTiles.size() >= 100;
    }

    bool TileProxy::isAhead(SinkId aSinkId) const
    {
        std::unique_lock<std::mutex> lock(mMutex);
        if (mSinkInfo.count(aSinkId))
        {
            Index smallestIndex = mSinkInfo.at(aSinkId).latestCodingIndex;
            for (const auto& sinkIdSinkInfo : mSinkInfo)
            {
                smallestIndex = std::min(smallestIndex, sinkIdSinkInfo.second.latestCodingIndex);
            }
            return mSinkInfo.at(aSinkId).latestCodingIndex >= smallestIndex + 1;
        }
        else
        {
            return false;
        }
    }

    TileProxy::SinkId TileProxy::getNextSinkId()
    {
        auto sinkId = mNextSinkId;
        ++mNextSinkId;
        return sinkId;
    }

    void TileProxy::processExtractors(std::vector<Streams>& aTiles)
    {
        aTiles.push_back({ collectExtractors() });
    }

    void TileProxy::createEoS(std::vector<Streams>& aTiles)
    {
        Meta meta = defaultMeta;
        meta.attachTag<TrackIdTag>(mExtractorTrackId);
        aTiles.push_back({ Data(EndOfStream(), mExtractorStreamId, meta) });
        mExtractorCache.clear();
    }

    bool TileProxy::extractorDataCollectionReady() const
    {
        if (mExtractorCache.size() < mTileCount)
        {
            return false;
        }
        for (auto& item : mExtractorCache)
        {
            if (item.second.size() == 0)
            {
                return false;
            }
        }

        return true;
    }
    bool TileProxy::extractorReadyForEoS() const
    {
        if (mExtractorCache.size() < mTileCount)
        {
            return false;
        }
        for (auto& item : mExtractorCache)
        {
            if (item.second.size() == 0)
            {
                return false;
            }
            else if (!item.second.front().endOfStream)
            {
                return false;
            }
        }
        // all input should have EoS?
        return true;
    }

    Data TileProxy::collectExtractors()
    {
        CodedFrameMeta cMeta(mExtractorCache.begin()->second.front().data.getCodedFrameMeta());
        if (cMeta.regionPacking)
        {
            // need to rewrite the top level packed dimensions, as we are now talking about the combination of subpictures, not a single subpicture
            cMeta.regionPacking.get().packedPictureHeight = cMeta.height;
            cMeta.regionPacking.get().packedPictureWidth = cMeta.width;
            cMeta.regionPacking.get().projPictureHeight = cMeta.height;
            cMeta.regionPacking.get().projPictureWidth = cMeta.width;
            if (mOutputMode == PipelineOutputVideo::SideBySide ||
                mOutputMode == PipelineOutputVideo::TopBottom)
            {
                cMeta.regionPacking.get().constituentPictMatching = true;
            }
            cMeta.regionPacking.get().regions.clear();
        }
        Extractors extractors;
        for (auto& item : mExtractorCache)
        {
            // input has only 1 extractor per data, only output can have more
            extractors.push_back(std::move(item.second.front().data.getExtractors().front()));

            // Take region for the first packet only; input has only 1 region
            if (item.second.front().data.getCodedFrameMeta().regionPacking)
            {
                if (mOutputMode == PipelineOutputVideo::TopBottom)
                {
                    // use constituentPictMatching and copy only first half of the regions (symmetric stereo only supported for now)
                    // Note: the OMAF spec section 7.5.3.8 indicates that in case of framepacked stereo, the player should do the necessary adjustments to width or height
                    if (item.second.front().data.getCodedFrameMeta().regionPacking.get().regions.front().packedTop < cMeta.regionPacking.get().packedPictureHeight / 2)
                    {
                        cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().data.getCodedFrameMeta().regionPacking.get().regions.front()));
                    }
                }
                else if (mOutputMode == PipelineOutputVideo::SideBySide)
                {
                    // use constituentPictMatching and copy only first half of each of the region rows (symmetric stereo only supported for now)
                    if (item.second.front().data.getCodedFrameMeta().regionPacking.get().regions.front().packedLeft < cMeta.regionPacking.get().packedPictureWidth/2)
                    {
                        cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().data.getCodedFrameMeta().regionPacking.get().regions.front()));
                    }
                }
                else
                {
                    // copy all regions
                    cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().data.getCodedFrameMeta().regionPacking.get().regions.front()));
                }
            }

            item.second.pop_front();
        }

        Meta meta(cMeta);
        meta.attachTag<TrackIdTag>(mExtractorTrackId);
        if (!mExtractorSEICreated)
        {
            mExtractorSEICreated = true;
            return Data(std::move(createExtractorSEI(cMeta.regionPacking, extractors.front().nuhTemporalIdPlus1)), std::move(meta), std::move(extractors), mExtractorStreamId);
        }
        else
        {
            return Data(std::move(meta), std::move(extractors), mExtractorStreamId);
        }
    }

    CPUDataVector TileProxy::createExtractorSEI(VDD::Optional<RegionPacking> aRegionPacking, unsigned int aTemporalIdPlus1)
    {
        // add SEI NALs as data for the first extractor of each subpictures (may be needed only from one of them, but we don't know here which one)
        std::vector<std::uint8_t> seiBuffer;

        // we need temporal id for the SEI's
        Parser::BitStream seiNalStr;
        uint32_t length = OMAF::createProjectionSEI(seiNalStr, static_cast<int>(aTemporalIdPlus1));
        Parser::BitStream seiLengthField;
        seiLengthField.write32Bits(length);
        seiBuffer.insert(seiBuffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
        seiBuffer.insert(seiBuffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());

        if (aRegionPacking)
        {
            seiNalStr.clear();
            length = OMAF::createRwpkSEI(seiNalStr, *aRegionPacking, static_cast<int>(aTemporalIdPlus1));
            seiLengthField.clear();
            seiLengthField.write32Bits(length);
            seiBuffer.insert(seiBuffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
            seiBuffer.insert(seiBuffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());
        }
        std::vector<std::vector<std::uint8_t>> matrix(1, std::move(seiBuffer));
        return CPUDataVector(std::move(matrix));
    }

    TileProxySinkNode::TileProxySinkNode(GraphBase& aGraph, std::shared_ptr<TileProxy> aTileProxy, const std::set<StreamId>& aStreamIds)
        : AsyncProcessor(aGraph)
        , mTileProxy(aTileProxy)
        , mStreamIds(aStreamIds)
        , mActiveStreams(aStreamIds)
        , mSinkId(aTileProxy->getNextSinkId())
    {
        mTileProxy->addTileStreamIds(aStreamIds);
    }

    void TileProxySinkNode::hasInput(const Streams& input)
    {
        mTileProxy->submit(input, mSinkId);
        if (input.isEndOfStream())
        {
            for (auto& data: input)
            {
                mActiveStreams.erase(data.getStreamId());
            }
            if (!mActiveStreams.size())
            {
                setInactive();
            }
        }
    }

    bool TileProxySinkNode::isBlocked() const
    {
        if (mTileProxy->isAhead(mSinkId))
        {
            return true;
        }
        else if (mTileProxy->isOutputFull())
        {
            return true;
        }
        else
        {
            auto fillableStreams = mStreamIds;

            for (auto streamId : mTileProxy->fullTiles())
            {
                fillableStreams.erase(streamId);
            }

            return fillableStreams.size() < mStreamIds.size();
        }
    }

    TileProxySourceNode::TileProxySourceNode(GraphBase& aGraph,
                                             std::shared_ptr<TileProxy> aTileProxy)
        : AsyncSource(aGraph), mTileProxy(aTileProxy)
    {
        // nothing
    }

    void TileProxySourceNode::produce()
    {
        bool isEndOfStream = false;
        for (auto& tile : mTileProxy->moveTiles(isEndOfStream))
        {
            hasOutput(tile);
        }
        if (isEndOfStream)
        {
            setInactive();
        }
    }

    TileProxyConnector::TileProxyConnector(GraphBase& aGraph, std::unique_ptr<TileProxy> aTileProxy)
        : mGraph(aGraph)
        , mTileProxy(aTileProxy.release())
    {
        // nothing
    }

    TileProxySinkNode* TileProxyConnector::getSink(const std::set<StreamId>& aStreamIds)
    {
        auto sink = std::unique_ptr<TileProxySinkNode>(new TileProxySinkNode(mGraph, mTileProxy, aStreamIds));
        sink->setName("TileProxySink");
        mGraph.addGraphvizEdge(*sink, *getSource());
        return mGraph.giveOwnership(std::move(sink));
    }

    TileProxySourceNode* TileProxyConnector::getSource()
    {
        if (!mSource)
        {
            mSource = mGraph.giveOwnership(Utils::make_unique<TileProxySourceNode>(mGraph, mTileProxy));
            mSource->setName("TileProxySource");
        }
        return mSource;
    }
}  // namespace VDD
