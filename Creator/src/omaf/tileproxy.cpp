
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
        : mTileCount(aConfig.tileCount)
        , mTileMergingConfig(aConfig.tileMergingConfig)
        , mExtractorTrackId(aConfig.extractors.front().second)
        , mExtractorStreamId(aConfig.extractors.front().first)
        , mOutputMode(aConfig.outputMode)
    {
    }

    StorageType TileProxy::getPreferredStorageType() const
    {
        return StorageType::CPU;
    }

    std::vector<Views> TileProxy::process(const Views& data)
    {
        std::vector<Views> tiles;
        if (data[0].isEndOfStream())
        {
            tiles.push_back(data);

            // and also for the extractor
            while (extractorDataCollectionReady())
            {
                processExtractors(tiles);
            }
            if (mExtractorCache.find(data.at(0).getStreamId()) != mExtractorCache.end())
            {
                mExtractorCache[data.at(0).getStreamId()].push_back(std::move(data.at(0)));
                if (extractorReadyForEoS())
                {
                    // as tileproducers won't send any extractor after endOfStream, we should have now flushed the cache
                    createEoS(tiles);
                }
            }
        }
        else
        {
            if (data.at(0).getCodedFrameMeta().format == CodedFormat::H265Extractor)
            {
                // expecting only 1 extractor per data in input

                mExtractorCache[data.at(0).getStreamId()].push_back(std::move(data.at(0)));
                // if we have extractor for all needed streams, output them as one new Data, with extractor streamId
                while (extractorDataCollectionReady())
                {
                    processExtractors(tiles);
                }
            }
            else
            {
                // not an extractor, just forward as-is
                tiles.push_back(data);
            }
        }
        return tiles;
    }

    void TileProxy::processExtractors(std::vector<Views>& aTiles)
    {
        aTiles.push_back({ std::move(collectExtractors()) });
    }

    void TileProxy::createEoS(std::vector<Views>& aTiles)
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
            else if (!item.second.front().isEndOfStream())
            {
                return false;
            }
        }
        // all input should have EoS?
        return true;
    }

    Data TileProxy::collectExtractors()
    {
        CodedFrameMeta cMeta(mExtractorCache.begin()->second.front().getCodedFrameMeta());
        if (cMeta.regionPacking)
        {
            // need to rewrite the top level packed dimensions, as we are now talking about the combination of subpictures, not a single subpicture
            cMeta.regionPacking.get().packedPictureHeight = cMeta.height;
            cMeta.regionPacking.get().packedPictureWidth = cMeta.width;
            cMeta.regionPacking.get().projPictureHeight = cMeta.height;
            cMeta.regionPacking.get().projPictureWidth = cMeta.width;
            if (mOutputMode == PipelineOutput::VideoSideBySide || mOutputMode == PipelineOutput::VideoTopBottom)
            {
                cMeta.regionPacking.get().constituentPictMatching = true;
            }
            cMeta.regionPacking.get().regions.clear();
        }
        Extractors extractors;
        for (auto& item : mExtractorCache)
        {
            // input has only 1 extractor per data, only output can have more
            extractors.push_back(std::move(item.second.front().getExtractors().at(0)));

            // Take region for the first packet only; input has only 1 region
            if (item.second.front().getCodedFrameMeta().regionPacking)
            {
                if (mOutputMode == PipelineOutput::VideoTopBottom)
                {
                    // use constituentPictMatching and copy only first half of the regions (symmetric stereo only supported for now)
                    // Note: the OMAF spec section 7.5.3.8 indicates that in case of framepacked stereo, the player should do the necessary adjustments to width or height
                    if (item.second.front().getCodedFrameMeta().regionPacking.get().regions.front().packedTop < cMeta.regionPacking.get().packedPictureHeight / 2)
                    {
                        cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().getCodedFrameMeta().regionPacking.get().regions.front()));
                    }
                }
                else if (mOutputMode == PipelineOutput::VideoSideBySide)
                {
                    // use constituentPictMatching and copy only first half of each of the region rows (symmetric stereo only supported for now)
                    if (item.second.front().getCodedFrameMeta().regionPacking.get().regions.front().packedLeft < cMeta.regionPacking.get().packedPictureWidth/2)
                    {
                        cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().getCodedFrameMeta().regionPacking.get().regions.front()));
                    }
                }
                else
                {
                    // copy all regions
                    cMeta.regionPacking.get().regions.push_back(std::move(item.second.front().getCodedFrameMeta().regionPacking.get().regions.front()));
                }
            }

            item.second.pop_front();
        }

        Meta meta(cMeta);
        meta.attachTag<TrackIdTag>(mExtractorTrackId);
        if (!mExtractorSEICreated)
        {
            mExtractorSEICreated = true;
            return Data(std::move(createExtractorSEI(cMeta.regionPacking, extractors.at(0).nuhTemporalIdPlus1)), std::move(meta), std::move(extractors), mExtractorStreamId);
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
        uint32_t length = OMAF::createProjectionSEI(seiNalStr, aTemporalIdPlus1);
        Parser::BitStream seiLengthField;
        seiLengthField.write32Bits(length);
        seiBuffer.insert(seiBuffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
        seiBuffer.insert(seiBuffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());

        if (aRegionPacking)
        {
            seiNalStr.clear();
            length = OMAF::createRwpkSEI(seiNalStr, *aRegionPacking, aTemporalIdPlus1);
            seiLengthField.clear();
            seiLengthField.write32Bits(length);
            seiBuffer.insert(seiBuffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
            seiBuffer.insert(seiBuffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());
        }
        std::vector<std::vector<std::uint8_t>> matrix(1, std::move(seiBuffer));
        return CPUDataVector(std::move(matrix));
    }
}  // namespace VDD
