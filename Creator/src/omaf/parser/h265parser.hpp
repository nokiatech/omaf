
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
#ifndef H265_PARSER_HPP
#define H265_PARSER_HPP

#include "bitstream.hpp"
#include "parserinterface.hpp"

#include <array>
#include <cstdint>
#include <fstream>
#include <list>
#include <string>
#include <vector>
#include <memory>
#include "h265datastructs.hpp"

#define EXTENDED_SAR 255

class H265InputStream
{
public:
    H265InputStream() = default;
    virtual ~H265InputStream() = default;

    virtual uint8_t getNextByte() = 0;
    virtual bool eof() = 0;
    virtual void rewind(size_t) = 0;
};

/** @brief H265 bitstream parser.
 *  @details Note: This is not a full-featured H.265 bitstream parser.
 *  @details It contains sufficient features in order to parse H.265 bitstreams compliant with OMAF standard.
 */
class H265Parser
{
public:
    H265Parser();
    virtual ~H265Parser();
    
    static void convertToRBSP(const std::vector<uint8_t>& src, std::vector<uint8_t>& dest, bool byteStreamMode = true);
    int  writeSliceHeader(Parser::BitStream& bitstr, H265::SliceHeader&, H265::H265NalUnitType naluType, const H265::SequenceParameterSet& aSps, const H265::PictureParameterSet& aPps);
    static std::uint32_t writeEquiProjectionSEINal(Parser::BitStream& bitstr, H265::EquirectangularProjectionSEI& projection, int temporalIdPlus1);
    static std::uint32_t writeCubeProjectionSEINal(Parser::BitStream& bitstr, H265::CubemapProjectionSEI& projection, int temporalIdPlus1);
    static std::uint32_t writeFramePackingSEINal(Parser::BitStream& bitstr, H265::FramePackingSEI& packing, int temporalIdPlus1);
    static std::uint32_t writeRwpkSEINal(Parser::BitStream& bitstr, H265::RegionWisePackingSEI& packing, int temporalIdPlus1);
    static std::uint32_t writeRotationSEINal(Parser::BitStream& bitstr, H265::SphereRotationSEI& packing, int temporalIdPlus1);
    static bool isFirstVclNaluInPic(const std::vector<uint8_t>& nalUnit);
    static bool checkAccessUnitBoundary(const std::vector<uint8_t>& nalUnit, bool isFirstNaluInAU, bool firstVclNaluFound);
    static int parseNalUnitHeader(Parser::BitStream& bitstr, H265::NalUnitHeader& naluHeader);
    static int parseSPS(Parser::BitStream& bitstr, H265::SequenceParameterSet& sps);
    static int  parsePPS(Parser::BitStream& bitstr, H265::PictureParameterSet& pps);

    bool parseParamSet(H265InputStream& aInputStream, ParserInterface::AccessUnit& accessUnit);
    bool parseNextAU(H265InputStream& aInputStream, ParserInterface::AccessUnit& accessUnit);

    int  parseNalUnit(const std::vector<uint8_t>& nalUnit, H265::NalUnitHeader& naluHeader);
    std::list<H265::SequenceParameterSet*>getSpsList();
    std::list<H265::PictureParameterSet*> getPpsList();
    void writeSPS(Parser::BitStream& mOutFile, H265::SequenceParameterSet& sps, bool rbsp);
    void writePPS(Parser::BitStream& bitstr, H265::PictureParameterSet& pps, bool rbsp);
    int parseSliceHeader(Parser::BitStream& bitstr, H265::SliceHeader& slice, H265::H265NalUnitType naluType, const std::list<H265::SequenceParameterSet*>& aSpsList, const std::list<H265::PictureParameterSet*>& aPpsList);
    void getSliceHeaderOffset(std::vector<H265::sliceHeaderOffset*>&);
    void getSliceHeaderList(std::vector <H265::SliceHeader>&);

private:
    int  writeShortTermRefPicSet(Parser::BitStream& bitstr, const std::vector<H265::ShortTermRefPicSetDerived>& rpsList, H265::ShortTermRefPicSet& rps, const unsigned int stRpsIdx, const unsigned int numShortTermRefPicSets);
    static void writeRbspTrailingBits(Parser::BitStream& bitstr);
    static bool isVclNaluType(H265::H265NalUnitType naluType);
    static void insertEPB(std::vector<uint8_t>& payload);
    H265::SequenceParameterSet* findSps(unsigned int spsId, const std::list<H265::SequenceParameterSet*>& aSpsList);
    H265::PictureParameterSet* findPps(unsigned int ppsId, const std::list<H265::PictureParameterSet*>& aPpsList);
    static void writeSEINalHeader(Parser::BitStream& bitstr, H265::H265SEIType payloadType, unsigned int payloadSize, int temporalIdPlus1);
    unsigned int ceilLog2(unsigned int x);
    static int parseShortTermRefPicSet(Parser::BitStream& bitstr, const std::vector<H265::ShortTermRefPicSetDerived>& rpsList, H265::ShortTermRefPicSet& rps, unsigned int stRpsIdx, unsigned int numShortTermRefPicSets);
    static int deriveRefPicSetParams(const std::vector<H265::ShortTermRefPicSetDerived>& rpsList, const H265::ShortTermRefPicSet& rps, H265::ShortTermRefPicSetDerived& rpsDerived, unsigned int stRpsIdx);
    int parseRefPicListsModification(Parser::BitStream& bitstr, const H265::SliceHeader& slice, H265::RefPicListsModification& refPicListsMod);
    int parsePredWeightTable(Parser::BitStream& bitstr, H265::SequenceParameterSet& sps, H265::SliceHeader& slice, H265::PredWeightTable& pwTable);

private:
    int  writeScalingListData(Parser::BitStream& bitstr, H265::ScalingListData& scalingList);
    int  writeProfileTierLevel(Parser::BitStream& bitstr, H265::ProfileTierLevel& ptl, const unsigned int maxNumSubLayersMinus1);
    int  writePredWeightTable(Parser::BitStream&, H265::SequenceParameterSet&, H265::SliceHeader&, H265::PredWeightTable&);
    void printPicStats(const H265::Picture& pic);
    int decodePoc(const H265::SliceHeader& slice, H265::NalUnitHeader& naluHeader);
    int decodeRefPicSet(H265::SliceHeader& slice, H265::RefPicSet& rps, int poc);
    int deltaPocMsbCycleLt(H265::SliceHeader& slice, int i);
    H265::Picture* findPicInDpbPocLsb(unsigned int pocLsb);
    H265::Picture* findPicInDpbPoc(int poc);
    int generateRefPicLists(H265::SliceHeader& slice, H265::RefPicSet& rps);
    void removeSps(unsigned int spsId);
    void removePps(unsigned int ppsId);
    void getRefPicIndices(std::vector<unsigned int>& refPicIndices, const std::vector<H265::Picture*>& mRefPicList0, const std::vector<H265::Picture*>& mRefPicList1);
    bool isUniquePicIndex(const std::vector<unsigned int>& refPicIndices, unsigned int picIndex);
    static int parseProfileTierLevel(Parser::BitStream& bitstr, H265::ProfileTierLevel& ptl, unsigned int maxNumSubLayersMinus1);
    static int parseScalingListData(Parser::BitStream& bitstr, H265::ScalingListData& scalingList);
    static void setVuiDefaults(H265::ProfileTierLevel& ptl, H265::VuiParameters& vui);
    static int parseVuiParameters(Parser::BitStream& bitstr, H265::VuiParameters& vui);
    int parseHrdParameters(Parser::BitStream& bitstr, H265::HrdParameters& hrd, unsigned int commonInfPresentFlag, unsigned int maxNumSubLayersMinus1);
    int parseSubLayerHrd(Parser::BitStream& bitstr, H265::SubLayerHrdParameters& hrd, int cpbCnt, unsigned int subPicHrdParamsPresentFlag);
    static H265::H265NalUnitType getH265NalUnitType(const std::vector<uint8_t>& nalUnit);

public:
    static H265::H265NalUnitType readNextNalUnit(H265InputStream& aInputStream, std::vector<uint8_t>& nalUnit);

private:
    std::vector<H265::SliceHeader> mSliceHeaderList;
    std::vector<H265::Picture*> mRefPicList0;
    std::vector<H265::Picture*> mRefPicList1;
    std::list<H265::PictureParameterSet*> mPpsList;
    std::list<H265::SequenceParameterSet*> mSpsList;
    std::list<H265::Picture> mDecodedPicBuffer;
    int mPicIndex;
    unsigned int mPrevPicOrderCntLsb;
    int mPrevPicOrderCntMsb;
    std::vector<H265::SliceHeader*> mSliceList;
    std::vector <H265::sliceHeaderOffset*> mSliceHeaderOffsetList;

};

#endif
