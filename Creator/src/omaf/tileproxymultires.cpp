
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
#include <math.h>
#include <sstream>
#include "./tileproxymultires.h"
#include "processor/data.h"
#include "log/logstream.h"
#include "./tilefilter.h"


namespace VDD {

    TileProxyMultiRes::TileProxyMultiRes(Config aConfig)
        : TileProxy(aConfig)
    {
        if (!mH265Parser)
        {
            // Create a new H265Parser and move it into mH265Parser
            mH265Parser.reset(new H265Parser);
        }
    }

    void TileProxyMultiRes::processExtractors(std::vector<Streams>& aTiles)
    {
        for (auto& direction : mTileMergingConfig.directions)
        {
            aTiles.push_back({ collectExtractors(direction, mFirstExtractorRound) });
        }
        mFirstExtractorRound = false;
        // now we are ready to pop the extractors at front of each stream
        for (auto& item : mExtractorCache)
        {
            item.second.pop_front();
        }
    }

    void TileProxyMultiRes::createEoS(std::vector<Streams>& aTiles)
    {
        for (auto& direction : mTileMergingConfig.directions)
        {
            Meta meta = defaultMeta;
            meta.attachTag<TrackIdTag>(direction.extractorId.second);
            aTiles.push_back({ Data(EndOfStream(), direction.extractorId.first, meta) });
        }
        mExtractorCache.clear();
    }

    Data TileProxyMultiRes::collectExtractors(TileDirectionConfig& aDirection, bool aFirstPacket)
    {
        StreamId idOfFirstTile = aDirection.tiles.at(0).at(0).tile.at(0).ids.first;

        CodedFrameMeta cMeta(mExtractorCache[idOfFirstTile].front().data.getCodedFrameMeta());

        if (aFirstPacket)
        {
            // 
            StreamId idOfVeryFirstTile = mTileMergingConfig.directions.at(0).tiles.at(0).at(0).tile.at(0).ids.first;

            CodedFrameMeta cFirstMeta(mExtractorCache[idOfVeryFirstTile].front().data.getCodedFrameMeta());

            // the very first extractor data should contain SPS of the input picture
            // and the very first tile is expected to be a high-res video (but it may not matter even if it is not as we rewrite the resolution)
            if (cFirstMeta.decoderConfig[ConfigType::SPS].size())
            {
                std::vector<uint8_t> origSps = cFirstMeta.decoderConfig[ConfigType::SPS];
                std::vector<uint8_t> nalUnitRBSP;
                mH265Parser->convertToRBSP(origSps, nalUnitRBSP, true);
                Parser::BitStream bitstr(nalUnitRBSP);
                H265::NalUnitHeader naluHeader;
                mH265Parser->parseNalUnitHeader(bitstr, naluHeader);

                mH265Parser->parseSPS(bitstr, mTileMergingConfig.extractorSPS);
                mTileMergingConfig.extractorSPS.mPicHeightInLumaSamples = mTileMergingConfig.packedHeight;
                mTileMergingConfig.extractorSPS.mPicWidthInLumaSamples = mTileMergingConfig.packedWidth;
                mTileMergingConfig.extractorSPS.mVuiParametersPresentFlag = 0;//??

                // Replace SPS
                cMeta.decoderConfig.erase(ConfigType::SPS);
                Parser::BitStream newSps;
                // first write start code (4 bytes) + NAL unit header
                for (uint8_t i = 0; i < 6; i++)
                {
                    newSps.write8Bits(origSps.at(i));
                }
                // then encode the merged stream sps
                mH265Parser->writeSPS(newSps, mTileMergingConfig.extractorSPS, false);
                cMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::SPS, { newSps.getStorage() }));


            }
            if (cFirstMeta.decoderConfig[ConfigType::PPS].size())
            {
                std::vector<uint8_t> origPps = cFirstMeta.decoderConfig[ConfigType::PPS];

                uint64_t ctuSize = pow(2, (mTileMergingConfig.extractorSPS.mLog2MinLumaCodingBlockSizeMinus3 + 3) + (mTileMergingConfig.extractorSPS.mLog2DiffMaxMinLumaCodingBlockSize));

                std::vector<uint8_t> nalUnitRBSP;
                mH265Parser->convertToRBSP(origPps, nalUnitRBSP, true);
                Parser::BitStream bitstr(nalUnitRBSP);
                H265::NalUnitHeader naluHeader;
                mH265Parser->parseNalUnitHeader(bitstr, naluHeader);

                mH265Parser->parsePPS(bitstr, mTileMergingConfig.extractorPPS);
                mTileMergingConfig.extractorPPS.mNumTileColumnsMinus1 = mTileMergingConfig.grid.columnWidths.size() - 1;
                mTileMergingConfig.extractorPPS.mNumTileRowsMinus1 = mTileMergingConfig.grid.rowHeights.size() - 1;
                mTileMergingConfig.extractorPPS.mUniformSpacingFlag = 0;
                mTileMergingConfig.extractorPPS.mColumnWidthMinus1.clear();
                mTileMergingConfig.extractorPPS.mRowHeightMinus1.clear();
                for (size_t x = 0; x < mTileMergingConfig.extractorPPS.mNumTileColumnsMinus1; x++)
                {
                    mTileMergingConfig.extractorPPS.mColumnWidthMinus1.push_back(mTileMergingConfig.grid.columnWidths[x] / ctuSize - 1);
                }
                for (size_t y = 0; y < mTileMergingConfig.extractorPPS.mNumTileRowsMinus1; y++)
                {
                    mTileMergingConfig.extractorPPS.mRowHeightMinus1.push_back(mTileMergingConfig.grid.rowHeights[y] / ctuSize - 1);
                }

                // Replace PPS
                cMeta.decoderConfig.erase(ConfigType::PPS);

                Parser::BitStream newPps;
                // first write start code (4 bytes) + NAL unit header
                for (uint8_t i = 0; i < 6; i++)
                {
                    newPps.write8Bits(origPps.at(i));
                }

                // then encode the merged stream pps
                mH265Parser->writePPS(newPps, mTileMergingConfig.extractorPPS, false);
                cMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::PPS, { newPps.getStorage() }));
            }
            // add quality ranking for the first one
            cMeta.qualityRankCoverage = aDirection.qualityRank;
            cMeta.sphericalCoverage = aDirection.qualityRank.qualityInfo.front().sphere;

        }


        if (cMeta.regionPacking)
        {
            // need to rewrite the top level packed dimensions, as we are now talking about the combination of subpictures, not a single subpicture
            cMeta.regionPacking.get().packedPictureHeight = mTileMergingConfig.packedHeight;
            cMeta.regionPacking.get().packedPictureWidth = mTileMergingConfig.packedWidth;
            cMeta.regionPacking.get().projPictureHeight = mTileMergingConfig.projectedHeight;
            cMeta.regionPacking.get().projPictureWidth = mTileMergingConfig.projectedWidth;
            if (mOutputMode == PipelineOutputVideo::SideBySide ||
                mOutputMode == PipelineOutputVideo::TopBottom)
            {
                cMeta.regionPacking.get().constituentPictMatching = true;
            }

            cMeta.regionPacking.get().regions.clear();
        }
        cMeta.width = mTileMergingConfig.packedWidth;
        cMeta.height = mTileMergingConfig.packedHeight;

        Extractors extractors;
        // rewrite the inline constructor and region packing to follow the tile packing scheme
        for (auto& row : aDirection.tiles)
        {
            for (auto& gridTile : row)
            {
                for (auto& tile : gridTile.tile)
                {
                    StreamId id = tile.ids.first;

                    Data& data = mExtractorCache[id].front().data;
                    Extractor extractor = data.getExtractors().front();

                    // rewrite the inline constructor for this tile configuration
                    // the same extractor may be used with other tile configuration, so don't pop it yet
                    Parser::BitStream newSliceHeaderBitStr;
                    convertSliceHeader(extractor.sampleConstruct.at(0).sliceInfo.naluHeader, extractor.sampleConstruct.at(0).sliceInfo.origSliceHeader, newSliceHeaderBitStr, static_cast<std::uint64_t>(tile.ctuIndex), mTileMergingConfig.extractorSPS, mTileMergingConfig.extractorPPS);

                    Extractor::InlineConstruct inlineCtor;
                    // add placeholder for NAL unit length field; reader is expected to rewrite it
                    inlineCtor.inlineData.insert(inlineCtor.inlineData.begin(), 4, 0xff);
                    // add the updated slice header
                    inlineCtor.inlineData.insert(inlineCtor.inlineData.end(), newSliceHeaderBitStr.getStorage().begin(), newSliceHeaderBitStr.getStorage().end());

                    if (extractor.inlineConstruct.size())
                    {
                        // replace the existing inline constructor
                        inlineCtor.idx = extractor.inlineConstruct.front().idx;
                        extractor.inlineConstruct.pop_back();
                    }
                    else
                    {
                        // there was no inline constructor (the tile was originally the first slice of a picture), so create one and insert it before the sample constructor
                        inlineCtor.idx = extractor.sampleConstruct.at(0).idx;
                        extractor.sampleConstruct.at(0).idx++;
                        extractor.idx++;

                        // then modify the sample constructor too, as it is no longer the first slice of a picture
                        extractor.sampleConstruct.at(0).dataOffset = extractor.sampleConstruct.at(0).sliceInfo.payloadOffset;
                        extractor.sampleConstruct.at(0).dataLength -= extractor.sampleConstruct.at(0).sliceInfo.origSliceHeaderLength;
                    }
                    extractor.inlineConstruct.push_back(inlineCtor);

                    extractors.push_back(extractor);
                }
                // Take region for the first packet only; input has only 1 region
                if (cMeta.regionPacking)
                {
                    // copy RWPK from the config
                    cMeta.regionPacking.get().regions.push_back(gridTile.regionPacking);
                }
            }
        }

        Meta meta(cMeta);
        meta.attachTag<TrackIdTag>(aDirection.extractorId.second);
        if (!mExtractorSEICreated)
        {
            mExtractorSEICreated = true;
            return Data(std::move(createExtractorSEI(*cMeta.regionPacking, extractors.at(0).nuhTemporalIdPlus1)), std::move(meta), std::move(extractors), aDirection.extractorId.first);
        }
        else
        {
            return Data(std::move(meta), std::move(extractors), aDirection.extractorId.first);
        }
    }

    void TileProxyMultiRes::convertSliceHeader(std::vector<std::uint8_t>& aNaluHeader, H265::SliceHeader& aOldHeaderParsed, Parser::BitStream& aNewSliceHeader, uint64_t aCtuId,
        const H265::SequenceParameterSet& aSps, const H265::PictureParameterSet& aPps)
    {
        H265::NalUnitHeader naluHeader;
        Parser::BitStream bitstr(aNaluHeader);
        mH265Parser->parseNalUnitHeader(bitstr, naluHeader);
        // then copy the header
        for (uint8_t j = 0; j < aNaluHeader.size(); j++)
        {
            aNewSliceHeader.write8Bits(aNaluHeader.at(j));
        }

        if (aCtuId == 0)
        {
            aOldHeaderParsed.mFirstSliceSegmentInPicFlag = 1;
        }
        else
        {
            aOldHeaderParsed.mFirstSliceSegmentInPicFlag = 0;
            aOldHeaderParsed.mSliceSegmentAddress = (unsigned int)aCtuId;
        }

        mH265Parser->writeSliceHeader(aNewSliceHeader, aOldHeaderParsed, naluHeader.mH265NalUnitType, aSps, aPps);

    }

}  // namespace VDD
