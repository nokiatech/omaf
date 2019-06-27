
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
#pragma once

#include <memory>
#include <vector>

#include "parser/h265parser.hpp"
#include "tileconfig.h"

namespace VDD {

    enum ExtractorMode
    {
        INVALID = -1,
        NO_EXTRACTOR,
        COMMON_EXTRACTOR,
        DEDICATED_EXTRACTOR,

        COUNT
    };
    class TileFilter
    {
    public:
        TileFilter(std::uint8_t aQualityRank, Projection& aProjection, bool aResetExtractorLevelIDCTo51);
        ~TileFilter();

    public:
        typedef std::vector<OmafTileSetConfiguration> OmafTileSets;

        bool parseParamSet(H265InputStream& aInputStream, const OmafTileSets& aTileConfig, const CodedFrameMeta& aInputMeta);
        bool parseAU(H265InputStream& aInputStream, int aExpectedTileCount);
        void convertToSubpicture(std::uint32_t aConfigIndex, const OmafTileSetConfiguration& aConfig, size_t aAUIndex, std::vector<Views>& aSubPictures, FrameTime aPresTime, int64_t aCodingIndex, FrameDuration aDuration, ExtractorMode aExtractorMode);

    private:
        struct TilePixelRegion
        {
            uint64_t top;
            uint64_t left;
            uint64_t width;
            uint64_t height;
        };

        using CbsSpsData = std::list<H265::SequenceParameterSet*>;
        using CbsPpsData = std::list<H265::PictureParameterSet*>;
        using SlcHdrOset = std::vector<H265::sliceHeaderOffset*>;
        using AccessUnit = ParserInterface::AccessUnit;
		
        void createSubPictureSliceHeader(std::vector<std::uint8_t>& aOldHeader, size_t aOldHeaderOffset, Parser::BitStream& aNewSliceHeader,
            const std::list<H265::SequenceParameterSet*>& aOldSpsList, const std::list<H265::PictureParameterSet*>& aOldPpsList,
            H265::SequenceParameterSet* aNewSps, H265::PictureParameterSet* aNewPps, H265::SliceHeader& aOldHeaderParsed, uint8_t& aNuhTemporalIdPlus1);
        void prepareParamSets(const OmafTileSets& aTileConfig, const CodedFrameMeta& aInputMeta);
        CodedFrameMeta createMetadata(const TilePixelRegion& aTile, TrackId aTrackId,
                                      FrameTime aPresTime,
                                      int64_t aCodingIndex,
                                      FrameDuration aDuration, bool aIsIDR, size_t aAUIndex,
                                      const CbsSpsData& aSps, const CbsPpsData& aPps,
                                      uint32_t aBitrate);
        CodedFrameMeta createExtractorMetadata(const TilePixelRegion& aTile,
                                               FrameTime aPresTime,
                                               int64_t aCodingIndex,
                                               FrameDuration aDuration, size_t aAUIndex,
                                               bool aIsIDR);
        RegionPacking createRwpk(const TilePixelRegion& aTile);
        void createSpherical(CodedFrameMeta& aCodedMeta);

    private:
        // parameters sets in coded format
        struct NonVclNals {
            std::vector<uint8_t>  mVpsNals; /* A vector of Video Parameter Set NAL units.    */
            std::vector<uint8_t>  mSpsNals; /* A vector of Sequence Parameter Set NAL units. */
            std::vector<uint8_t>  mPpsNals; /* A vector of Picture Parameter Set NAL units.  */
        };
        NonVclNals  mNonVclNals;

        // parameter sets in parsed format
        // Original ones (full-size)
        CbsSpsData mOriginalSpsData;
        CbsPpsData mOriginalPpsData;
        // Processed ones, one for each subpicture
        std::vector<CbsSpsData> mCbsSpsData;
        std::vector<CbsPpsData> mCbsPpsData;


        std::unique_ptr<H265Parser>  mH265Parser = nullptr;
        std::unique_ptr<AccessUnit>  mAccessUnit = nullptr;
        SlcHdrOset mSlcHdrOset = {};

        std::uint8_t mQualityRank;
        std::vector<int> mBitratePerSubpicture;
        Projection mProjection;
        bool mResetExtractorLevelIDCTo51;

        std::vector<TilePixelRegion> mTileRegions;
    };


}
