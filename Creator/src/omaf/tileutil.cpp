
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
#include "parser/h265parser.hpp"

namespace VDD
{
    /* aLevel = 51 -> set level to 5.1 */
    void setSpsLevelIdc(H265::SequenceParameterSet& aSps, unsigned aLevel)
    {
        // "general_level_idc and sub_layer_level_idc[ i ] shall be set equal to a value of 30
        // times. 3 and aLevel used here instead of 30 and 5.1 to avoid useless floating point
        // arithmetics
        aSps.mProfileTierLevel.mGeneralLevelIdc = aLevel * 3;
        for (auto& x : aSps.mProfileTierLevel.mSubLayerProfileTierLevels)
        {
            x.mSubLayerLevelIdc = aLevel * 3;
        }
    }

    std::vector<uint8_t> spsNalWithLevelIdc(const std::vector<uint8_t>& aSpsNal, unsigned aLevel)
    {
        std::vector<uint8_t> customSps;

        std::vector<uint8_t> rbsp;
        H265::NalUnitHeader nalUnitHeader;
        H265Parser::convertToRBSP(aSpsNal, rbsp, true);
        Parser::BitStream bitstrIn(rbsp);
        H265::SequenceParameterSet sps;
        H265Parser::parseNalUnitHeader(bitstrIn, nalUnitHeader);
        H265Parser::parseSPS(bitstrIn, sps);

        setSpsLevelIdc(sps, aLevel);

        Parser::BitStream bitstrOut;
        H265Parser::writeNalUnitHeader(bitstrOut, nalUnitHeader);
        H265Parser::writeSPS(bitstrOut, sps, true);
        auto customSpsRbsp = bitstrOut.getStorage();
        H265Parser::convertFromRBSP(customSpsRbsp, customSps);

        return customSps;
    }
}  // namespace VDD
