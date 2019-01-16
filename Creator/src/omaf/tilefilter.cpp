
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
#include "./tilefilter.h"
#include "./parser/h265parser.hpp"
#include "extractor.h"
#include "omafproperties.h"
#include <math.h>


namespace VDD {

    static const uint8_t NALHeaderLength = 2;

    TileFilter::TileFilter(std::uint8_t aQualityRank, Projection& aProjection)
        : mQualityRank(aQualityRank)
        , mProjection(aProjection)
    {
    }

    TileFilter::~TileFilter()
    {
        // TODO these would be cleaner to handle as unique_ptr, but H265Parser should also return the lists in that form too, otherwise we have many types to convert between, and that is no longer clean
        while (mOriginalSpsData.size() > 0)
        {
            H265::SequenceParameterSet* sps = mOriginalSpsData.back();
            mOriginalSpsData.pop_back();
            delete sps;
        }
        while (mOriginalPpsData.size() > 0)
        {
            H265::PictureParameterSet* pps = mOriginalPpsData.back();
            mOriginalPpsData.pop_back();
            delete pps;
        }
        while (mCbsSpsData.size() > 0)
        {
            CbsSpsData spsList = mCbsSpsData.back();
            mCbsSpsData.pop_back();
            while (spsList.size() > 0)
            {
                H265::SequenceParameterSet* sps = spsList.back();
                spsList.pop_back();
                delete sps;
            }
        }
        while (mCbsPpsData.size() > 0)
        {
            CbsPpsData ppsList = mCbsPpsData.back();
            mCbsPpsData.pop_back();
            while (ppsList.size() > 0)
            {
                H265::PictureParameterSet* pps = ppsList.back();
                ppsList.pop_back();
                delete pps;
            }
        }

    }

    bool TileFilter::parseParamSet(H265InputStream& aInputStream, const OmafTileSets& aTileConfig, const CodedFrameMeta& aInputMeta)
    {
        if (!mH265Parser)
        {
            // Create a new H265Parser and move it into mH265Parser
            mH265Parser.reset(new H265Parser);
        }

        // Construct a new AccessUnit structure and move it into mAccessUnit
        mAccessUnit.reset(new AccessUnit);

        bool validAUFound = mH265Parser->parseParamSet(aInputStream, *mAccessUnit.get());

        // Push parameter sets and the access unit into their appropriate
        // storages
        if (mAccessUnit->mVpsNalUnits.size() > 0)
        {
            mNonVclNals.mVpsNals = mAccessUnit->mVpsNalUnits.front();
        }
        if (mAccessUnit->mSpsNalUnits.size() > 0)
        {
            mNonVclNals.mSpsNals = mAccessUnit->mSpsNalUnits.front();
        }
        if (mAccessUnit->mPpsNalUnits.size() > 0)
        {
            mNonVclNals.mPpsNals = mAccessUnit->mPpsNalUnits.front();
        }
        if (mNonVclNals.mSpsNals.size() > 0 && mNonVclNals.mPpsNals.size() > 0)
        {
            //TODO is not prepared to handle multiple parameter sets
            prepareParamSets(aTileConfig, aInputMeta);
        }
        return validAUFound;
    }

    bool TileFilter::parseAU(H265InputStream& aInputStream, int aExpectedTileCount)
    {
        if (!mH265Parser)
        {
            // Create a new H265Parser and move it into mH265Parser
            mH265Parser.reset(new H265Parser);
        }

        // Constuct a new AccessUnit structure and move it into mAccessUnit
        mAccessUnit.reset(new AccessUnit);

        bool validAUFound = mH265Parser->parseNextAU(aInputStream, *mAccessUnit.get());

        mH265Parser->getSliceHeaderOffset(mSlcHdrOset);

        // validate the number of tiles / NAL units
        if (validAUFound && mAccessUnit->mNalUnits.size() != aExpectedTileCount)
        {
            return false;
        }

        return validAUFound;
    }

    void TileFilter::prepareParamSets(const OmafTileSets& aTileConfig, const CodedFrameMeta& aInputMeta)
    {
        mCbsSpsData.reserve(aTileConfig.size());
        mCbsPpsData.reserve(aTileConfig.size());
        
        // store the original SPS/PPS
        // TODO these would be cleaner to handle as unique_ptr, but H265Parser should also return the lists in that form too, otherwise we have many types to convert between, and that is no longer clean
        CbsSpsData spsList = mH265Parser->getSpsList();
        for (CbsSpsData::iterator it = spsList.begin(); it != spsList.end(); ++it)
        {
            H265::SequenceParameterSet* sps = new H265::SequenceParameterSet(**it);
            mOriginalSpsData.push_back(sps);
        }
        unsigned int fullVideoPixelArea = mOriginalSpsData.front()->mPicWidthInLumaSamples * mOriginalSpsData.front()->mPicHeightInLumaSamples;
        CbsPpsData ppsList = mH265Parser->getPpsList();
        for (CbsPpsData::iterator it = ppsList.begin(); it != ppsList.end(); ++it)
        {
            H265::PictureParameterSet* pps = new H265::PictureParameterSet(**it);
            mOriginalPpsData.push_back(pps);
        }
        uint64_t ctuSize = pow(2, (mOriginalSpsData.front()->mLog2MinLumaCodingBlockSizeMinus3 + 3) + (mOriginalSpsData.front()->mLog2DiffMaxMinLumaCodingBlockSize));
        unsigned int tileWidth = 0;
        unsigned int tileHeight = 0;
        if (mOriginalPpsData.front()->mUniformSpacingFlag)
        {
            tileWidth = mOriginalSpsData.front()->mPicWidthInLumaSamples / (mOriginalPpsData.front()->mNumTileColumnsMinus1 + 1);
            tileHeight = mOriginalSpsData.front()->mPicHeightInLumaSamples / (mOriginalPpsData.front()->mNumTileRowsMinus1 + 1);

            for (int y = 0; y <= mOriginalPpsData.front()->mNumTileRowsMinus1; y++)
            {
                for (int x = 0; x <= mOriginalPpsData.front()->mNumTileColumnsMinus1; x++)
                {
                    TilePixelRegion tile;
                    tile.top = y * tileHeight;
                    tile.left = x * tileWidth;
                    tile.width = tileWidth;
                    tile.height = tileHeight;

                    mTileRegions.push_back(tile);
                }
            }
        }
        else
        {
            uint64_t top = 0;
            uint64_t left = 0;
            uint64_t lastColWidth = mOriginalSpsData.front()->mPicWidthInLumaSamples;
            uint64_t lastRowHeight = mOriginalSpsData.front()->mPicHeightInLumaSamples;
            for (int y = 0; y < mOriginalPpsData.front()->mNumTileRowsMinus1; y++)
            {
                left = 0;
                uint64_t rowHeight = (mOriginalPpsData.front()->mRowHeightMinus1.at(y) + 1)*ctuSize;
                lastColWidth = mOriginalSpsData.front()->mPicWidthInLumaSamples;
                for (int x = 0; x < mOriginalPpsData.front()->mNumTileColumnsMinus1; x++)
                {
                    TilePixelRegion tile;
                    tile.top = top;
                    tile.left = left;
                    tile.width = (mOriginalPpsData.front()->mColumnWidthMinus1.at(x) + 1)*ctuSize;
                    tile.height = rowHeight;
                    lastColWidth -= tile.width;
                    left += tile.width;
                    mTileRegions.push_back(tile);
                }
                TilePixelRegion tile;
                tile.top = top;
                tile.left = left;
                tile.width = lastColWidth;
                tile.height = rowHeight;
                mTileRegions.push_back(tile);

                lastRowHeight -= rowHeight;
                top += rowHeight;
            }
            left = 0;
            for (int x = 0; x < mOriginalPpsData.front()->mNumTileColumnsMinus1; x++)
            {
                TilePixelRegion tile;
                tile.top = top;
                tile.left = left;
                tile.width = (mOriginalPpsData.front()->mColumnWidthMinus1.at(x) + 1)*ctuSize;
                tile.height = lastRowHeight;
                mTileRegions.push_back(tile);
                left += (mOriginalPpsData.front()->mColumnWidthMinus1.at(x) + 1)*ctuSize;
            }
            TilePixelRegion tile;
            tile.top = top;
            tile.left = left;
            tile.width = lastColWidth;
            tile.height = lastRowHeight;
            mTileRegions.push_back(tile);
        }

        for (int i = 0; i < aTileConfig.size(); i++)
        {
            H265::NalUnitHeader naluHeader;
            mH265Parser->parseNalUnit(mNonVclNals.mSpsNals, naluHeader);
            spsList = mH265Parser->getSpsList();
            // make copy of the objects in the list, since they will be overwritten in the next round of this loop
            CbsSpsData spsListSubPicture;
            for (CbsSpsData::iterator it = spsList.begin(); it != spsList.end(); ++it)
            {
                H265::SequenceParameterSet* sps = new H265::SequenceParameterSet(**it);
                spsListSubPicture.push_back(sps);
            }
            // TODO we are kind of supporting many SPS but still only use the very first one

            spsListSubPicture.front()->mPicWidthInLumaSamples = mTileRegions.at(aTileConfig.at(i).tileIndex).width;
            spsListSubPicture.front()->mPicHeightInLumaSamples = mTileRegions.at(aTileConfig.at(i).tileIndex).height;

            // TODO the H265Parser is not able to write VUI to SPS. Disable VUI for now
            spsListSubPicture.front()->mVuiParametersPresentFlag = 0;
            mCbsSpsData.push_back(spsListSubPicture);

            mH265Parser->parseNalUnit(mNonVclNals.mPpsNals, naluHeader);
            ppsList = mH265Parser->getPpsList();
            // make copy of the objects in the list, since they will be overwritten in the next round of this loop
            CbsPpsData ppsListSubPicture;
            for (CbsPpsData::iterator it = ppsList.begin(); it != ppsList.end(); ++it)
            {
                H265::PictureParameterSet* pps = new H265::PictureParameterSet(**it);
                ppsListSubPicture.push_back(pps);
            }
            // TODO we are kind of supporting many PPS but still only use the very first one
            ppsListSubPicture.front()->mNumTileColumnsMinus1 = 0;
            ppsListSubPicture.front()->mNumTileRowsMinus1 = 0;
            // we are using 1 tile per subpicture, hence tiles are not really enabled
            ppsListSubPicture.front()->mTilesEnabledFlag = 0;

            mCbsPpsData.push_back(ppsListSubPicture);

            // estimate also the bitrate per tile set
            int bitrate = aInputMeta.bitrate.avgBitrate * mTileRegions[i].width*mTileRegions[i].height / fullVideoPixelArea;
            mBitratePerSubpicture.push_back(bitrate);
        }
    }

    // rewrite slice headers, and output them in NAL Access Unit format (length + NAL in RBSP)
    void TileFilter::convertToSubpicture(std::uint32_t aConfigIndex, const OmafTileSetConfiguration& aConfig, size_t aAUIndex, std::vector<Views>& aSubPictures, FrameTime aPresTime, int64_t aCodingIndex, FrameDuration aDuration, ExtractorMode aExtractorMode)
    {

        CbsSpsData& sps = mCbsSpsData.at(aConfigIndex);
        CbsPpsData& pps = mCbsPpsData.at(aConfigIndex);

        std::list<std::vector<uint8_t>>::iterator nalPtr;
        size_t origSliceHeaderLen = 1;

        std::vector<std::uint8_t> buffer;
        size_t payloadOffset = 0;

        Extractor extractor;
        uint8_t nuhTemporalIdPlus1 = 0;
        if (aAUIndex == 0)
        {
            // The SEI NAL units will be in the same MP4 sample as the tile bitstream. However, they are relevant only if the subpicture is decoded independently.
            // Hence the extractor should be able to skip the SEI NAL's, and have offset pointing to the video data NAL. This should work with the payloadOffset = buffer.size();
            // As they won't change, add them to the first frame only
            
            // we need temporal id for the SEI's

            std::vector<uint8_t> nalUnitRBSP;
            mH265Parser->convertToRBSP(mAccessUnit->mNalUnits.front(), nalUnitRBSP);
            Parser::BitStream bitstr(nalUnitRBSP);
            H265::NalUnitHeader naluHeader;
            H265Parser::parseNalUnitHeader(bitstr, naluHeader);
            int temporalIdPlus1 = naluHeader.mNuhTemporalIdPlus1;

            Parser::BitStream seiNalStr;
            uint32_t length = OMAF::createProjectionSEI(seiNalStr, temporalIdPlus1);
            Parser::BitStream seiLengthField;
            seiLengthField.write32Bits(length);
            buffer.insert(buffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
            buffer.insert(buffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());

            seiNalStr.clear();
            length = OMAF::createRwpkSEI(seiNalStr, std::move(createRwpk(mTileRegions.at(aConfig.tileIndex))), temporalIdPlus1);
            seiLengthField.clear();
            seiLengthField.write32Bits(length);
            buffer.insert(buffer.end(), seiLengthField.getStorage().begin(), seiLengthField.getStorage().end());
            buffer.insert(buffer.end(), seiNalStr.getStorage().begin(), seiNalStr.getStorage().end());
            
        }
        
        nalPtr = mAccessUnit->mNalUnits.begin();
        advance(nalPtr, aConfig.tileIndex);

        size_t startCodeLength = 0;
        // skip start code; 3 or 4 bytes
        for (startCodeLength = 0; startCodeLength < 4; startCodeLength++)
        {
            if (nalPtr->at(startCodeLength) == 1)
            {
                // last byte of start code found
                startCodeLength++;
                break;
            }
        }
            
        Parser::BitStream sliceHeader;
        H265::SliceHeader oldHeaderParsed;
        createSubPictureSliceHeader(*nalPtr, startCodeLength, sliceHeader, mOriginalSpsData, mOriginalPpsData, mCbsSpsData.front().front(), mCbsPpsData.front().front(), oldHeaderParsed, nuhTemporalIdPlus1);

        origSliceHeaderLen = mSlcHdrOset.at(aConfig.tileIndex)->byteOffset + 1; // There is always rbsp_trailing_bit of 1-8 bits, which is not included in the mSlcHdrOset; hence +1
        origSliceHeaderLen += startCodeLength;
        // origSliceHeaderLen now includes start code

        // add NAL length field
        Parser::BitStream lengthField;
        uint32_t nalLength = sliceHeader.getSize() + (std::uint32_t)(nalPtr->size() - origSliceHeaderLen);
        lengthField.write32Bits(nalLength);
        buffer.insert(buffer.end(), lengthField.getStorage().begin(), lengthField.getStorage().end());
        lengthField.clear();

        // add modified slice header
        buffer.insert(buffer.end(), sliceHeader.getStorage().begin(), sliceHeader.getStorage().end());
        payloadOffset = buffer.size();
        // add the payload (in bytestream format)
        buffer.insert(buffer.end(), nalPtr->begin() + origSliceHeaderLen, nalPtr->end());
        if (aExtractorMode != NO_EXTRACTOR)
        {
            // create constructors and save to the Extractor
            // as we always re-create the very first slice header too (we have tiles enabled = false in subpicture streams), we need to have inline constructor for the very first subpicture too
            Extractor::InlineConstruct inlineCtor;
            inlineCtor.idx = extractor.idx++;
            // add placeholder for NAL unit length field; reader is expected to rewrite it
            inlineCtor.inlineData.insert(inlineCtor.inlineData.begin(), 4, 0xff);
            // add the original header
            inlineCtor.inlineData.insert(inlineCtor.inlineData.end(), nalPtr->begin() + startCodeLength, nalPtr->begin() + origSliceHeaderLen);//origSliceHeaderLen includes startCodeLength

            extractor.inlineConstruct.push_back(inlineCtor);

            Extractor::SampleConstruct sampleCtor;
            sampleCtor.idx = extractor.idx++;
            sampleCtor.dataOffset = payloadOffset;
            if (aExtractorMode == DEDICATED_EXTRACTOR)
            {
                sampleCtor.dataLength = nalPtr->size() - origSliceHeaderLen;
            }
            else if (aExtractorMode == COMMON_EXTRACTOR)
            {
                // when datalength is larger than actual data, reader is expected to copy the full sample from offset to the end
                sampleCtor.dataLength = UINT32_MAX;
            }
            // When the dataLength == 0, reader is expected to copy a single full NAL unit; currently not useful for us 

            sampleCtor.trackId = std::uint8_t(aConfig.trackId.get());

            sampleCtor.sliceInfo.origSliceHeader = oldHeaderParsed;
            sampleCtor.sliceInfo.origSliceHeaderLength = origSliceHeaderLen;//including start code/length field
            // NAL length field included 
            sampleCtor.sliceInfo.payloadOffset = payloadOffset;
            sampleCtor.sliceInfo.naluHeader.insert(sampleCtor.sliceInfo.naluHeader.end(), nalPtr->begin() + startCodeLength, nalPtr->begin() + startCodeLength + NALHeaderLength);

            extractor.sampleConstruct.push_back(sampleCtor);
        }

        sliceHeader.clear();

        // assign buffer to subPicture
        std::vector<std::vector<std::uint8_t>> matrix(1, std::move(buffer));
        Meta meta(createMetadata(mTileRegions.at(aConfig.tileIndex), aConfig.trackId, aPresTime, aCodingIndex, aDuration, mAccessUnit->mIsIdr, aAUIndex, sps, pps, mBitratePerSubpicture.at(aConfigIndex)));
        meta.attachTag<TrackIdTag>(aConfig.trackId);
        aSubPictures.push_back({ Data(CPUDataVector(std::move(matrix)), 
            meta, 
            aConfig.streamId) });

        if (aExtractorMode != NO_EXTRACTOR)
        {
            extractor.nuhTemporalIdPlus1 = nuhTemporalIdPlus1;
            extractor.streamId = aConfig.streamId;
            aSubPictures.push_back({ Data(createExtractorMetadata(mTileRegions.at(aConfig.tileIndex), aPresTime, aCodingIndex, aDuration, aAUIndex, mAccessUnit->mIsIdr), {extractor}, aConfig.streamId) });
        }
    }

    // private, aOldHeader shall not have start codes
    void TileFilter::createSubPictureSliceHeader(std::vector<std::uint8_t>& aOldHeader, size_t aOldHeaderOffset, Parser::BitStream& aNewSliceHeader,
        const std::list<H265::SequenceParameterSet*>& aOldSpsList, const std::list<H265::PictureParameterSet*>& aOldPpsList,
        H265::SequenceParameterSet* aNewSps, H265::PictureParameterSet* aNewPps, H265::SliceHeader& aOldHeaderParsed, uint8_t& aNuhTemporalIdPlus1)
    {
        H265::NalUnitHeader naluHeader;
        std::vector<uint8_t> nalUnitRBSP;

        mH265Parser->convertToRBSP(aOldHeader, nalUnitRBSP, true);
        Parser::BitStream bitstr(nalUnitRBSP);
        mH265Parser->parseNalUnitHeader(bitstr, naluHeader);
        aNuhTemporalIdPlus1 = naluHeader.mNuhTemporalIdPlus1;
        mH265Parser->parseSliceHeader(bitstr, aOldHeaderParsed, naluHeader.mH265NalUnitType, aOldSpsList, aOldPpsList);
        // then copy the header
        for (uint8_t j = 0; j < NALHeaderLength; j++) 
        {
            aNewSliceHeader.write8Bits(aOldHeader.at(j + aOldHeaderOffset));
        }
        H265::SliceHeader slcHdrData = aOldHeaderParsed;

        // in subpictures, the slice is always the first slice (only 1 tile per subpicture supported). The address is reset only for clarity, it is not written to bitstream for first slice
        slcHdrData.mFirstSliceSegmentInPicFlag = 1;
        slcHdrData.mSliceSegmentAddress = 0;

        mH265Parser->writeSliceHeader(aNewSliceHeader, slcHdrData, naluHeader.mH265NalUnitType, *aNewSps, *aNewPps);

    }


    CodedFrameMeta TileFilter::createMetadata(const TilePixelRegion& aTile, TrackId aTrackId, FrameTime aPresTime, int64_t aCodingIndex, FrameDuration aDuration, bool aIsIDR, size_t aAUIndex, CbsSpsData& aSps, CbsPpsData& aPps, int aBitrate)
    {
        CodedFrameMeta codedMeta;
        codedMeta.inCodingOrder = true;
        codedMeta.presTime = aPresTime;
        codedMeta.codingIndex = CodingIndex(aCodingIndex);
        codedMeta.presIndex = CodingIndex(aCodingIndex);
        codedMeta.width = (std::uint32_t)aSps.front()->mPicWidthInLumaSamples;
        codedMeta.height = (std::uint32_t)aSps.front()->mPicHeightInLumaSamples;
        codedMeta.duration = aDuration;
        codedMeta.format = CodedFormat::H265;
        codedMeta.type = (aIsIDR ? VDD::FrameType::IDR : VDD::FrameType::NONIDR);
        codedMeta.trackId = aTrackId;
        codedMeta.projection = mProjection.projection;
        codedMeta.bitrate.avgBitrate = aBitrate;

        // needed only for the very first access unit for each subpicture
        if (aAUIndex == 0)
        {
            codedMeta.regionPacking = std::move(createRwpk(aTile));

            createSpherical(codedMeta);

            if (aSps.size() > 0)
            {
                Parser::BitStream bitstr;
                // first write start code (4 bytes) + NAL unit header
                for (uint8_t i = 0; i < 6; i++) 
                {
                    bitstr.write8Bits(mNonVclNals.mSpsNals.at(i));
                }
                // then encode the subpicture sps
                mH265Parser->writeSPS(bitstr, *aSps.front(), false);

                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::SPS, { bitstr.getStorage() }));
                bitstr.clear();
            }
            if (aPps.size() > 0)
            {
                Parser::BitStream bitstr;
                // first write start code (4 bytes) + NAL unit header
                for (uint8_t i = 0; i < 6; i++) 
                {
                    bitstr.write8Bits(mNonVclNals.mPpsNals.at(i));
                }
                // then encode the subpicture pps
                mH265Parser->writePPS(bitstr, *aPps.front(), false);

                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::PPS, { bitstr.getStorage() }));
                bitstr.clear();
            }
            if (mNonVclNals.mVpsNals.size() > 0)
            {
                // VPS can be written unchanged
                // the strange thing is that the mp4/dash writer expects SPS, PPS, VPS in byte stream format, but video data must be in NAL access unit format
                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::VPS, { mNonVclNals.mVpsNals }));
            }
        }

        return codedMeta;
    }

    CodedFrameMeta TileFilter::createExtractorMetadata(const TilePixelRegion& aTile, FrameTime aPts, int64_t aCodingIndex, FrameDuration aDuration, size_t aAUIndex, bool aIsIDR)
    {
        CodedFrameMeta codedMeta;
        codedMeta.format = CodedFormat::H265Extractor;
        codedMeta.presTime = aPts;
        codedMeta.codingIndex = CodingIndex(aCodingIndex);
        codedMeta.presIndex = CodingIndex(aCodingIndex);
        codedMeta.width = mOriginalSpsData.front()->mPicWidthInLumaSamples;
        codedMeta.height = mOriginalSpsData.front()->mPicHeightInLumaSamples;
        codedMeta.duration = aDuration;
        codedMeta.projection = mProjection.projection;
        codedMeta.type = (aIsIDR ? VDD::FrameType::IDR : VDD::FrameType::NONIDR);

        // insert the original SPS/PPS/VPS. This works with single resolution VD case
        if (aAUIndex == 0)
        {
            codedMeta.regionPacking = std::move(createRwpk(aTile));

            if (mNonVclNals.mSpsNals.size() > 0)
            {
                // Write the original SPS
                // the strange thing is that the mp4/dash writer expects SPS, PPS, VPS in byte stream format, but video data must be in NAL access unit format
                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::SPS, { mNonVclNals.mSpsNals }));
            }
            if (mNonVclNals.mPpsNals.size() > 0)
            {
                // Write the original PPS
                // the strange thing is that the mp4/dash writer expects SPS, PPS, VPS in byte stream format, but video data must be in NAL access unit format
                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::PPS, { mNonVclNals.mPpsNals }));
            }
            if (mNonVclNals.mVpsNals.size() > 0)
            {
                // Write the original PPS
                // the strange thing is that the mp4/dash writer expects SPS, PPS, VPS in byte stream format, but video data must be in NAL access unit format
                codedMeta.decoderConfig.insert(std::pair<ConfigType, std::vector<uint8_t>>(ConfigType::VPS, { mNonVclNals.mVpsNals }));
            }

        }
        return codedMeta;
    }

    RegionPacking TileFilter::createRwpk(const TilePixelRegion& aTile)
    {
        unsigned int fullWidth = mOriginalSpsData.front()->mPicWidthInLumaSamples;
        unsigned int fullHeight = mOriginalSpsData.front()->mPicHeightInLumaSamples;

        RegionPacking regionPacking;
        regionPacking.constituentPictMatching = false;//in this level, subpictures are assumed to be mono, i.e. in framepacked stereo tiles are always either in left or right eye, never crossing the eye boundary
        regionPacking.projPictureWidth = fullWidth;
        regionPacking.projPictureHeight = fullHeight;

        Region region;

        if (mProjection.projection == OmafProjectionType::CUBEMAP)
        {
            region.projTop = aTile.top;
            region.packedTop = aTile.top;
            region.projLeft = aTile.left;
            region.packedLeft = aTile.left;
            region.transform = 0; // no transform compared to OMAF cubemap definition
        }
        else
        {
            // equirect, projected as such without any moves
            region.projTop = aTile.top;
            region.packedTop = aTile.top;
            region.projLeft = aTile.left;
            region.packedLeft = aTile.left;
            region.transform = 0;
        }
        // At this point packed and projected dimensions the same. They may be rewritten while repacking the tiles
        region.projWidth = aTile.width;
        regionPacking.packedPictureWidth = region.packedWidth = aTile.width;
        region.projHeight = aTile.height;
        regionPacking.packedPictureHeight = region.packedHeight = aTile.height;

        regionPacking.regions.push_back(std::move(region));

        return regionPacking;
    }

    void TileFilter::createSpherical(CodedFrameMeta& aCodedMeta)
    {
        assert(aCodedMeta.regionPacking);
        unsigned int fullWidth = aCodedMeta.regionPacking.get().projPictureWidth;
        unsigned int fullHeight = aCodedMeta.regionPacking.get().projPictureHeight;

        Region& region = aCodedMeta.regionPacking.get().regions.front();

        // convert RWPK to spherical coverage, used in COVI and quality3d
        Spherical sphericalCoverage;
        sphericalCoverage.cAzimuth = (std::int32_t)((((fullWidth / 2) - (float)(region.projLeft + region.projWidth / 2)) * 360 * 65536) / fullWidth);
        sphericalCoverage.cElevation = (std::int32_t)((((fullHeight / 2) - (float)(region.projTop + region.projHeight / 2)) * 180 * 65536) / fullHeight);
        sphericalCoverage.cTilt = 0;

        sphericalCoverage.rAzimuth = (std::uint32_t)((region.projWidth * 360.f * 65536) / fullWidth);
        sphericalCoverage.rElevation = (std::uint32_t)((region.projHeight * 180.f * 65536) / fullHeight);

        aCodedMeta.sphericalCoverage = sphericalCoverage;

        // refer to sphericalCoverage
        Quality3d qualityRankCoverage;
        QualityInfo info;
        info.sphere = sphericalCoverage;
        info.qualityRank = mQualityRank;
        qualityRankCoverage.qualityInfo.push_back(info);

        aCodedMeta.qualityRankCoverage = qualityRankCoverage;
    }

}  // namespace VDD
