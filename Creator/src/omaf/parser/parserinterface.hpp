
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
#ifndef PARSER_INTERFACE_HPP
#define PARSER_INTERFACE_HPP

#include <cstdint>
#include <list>
#include <string>
#include <vector>

/** Interface for video bitstream parsers */
namespace ParserInterface
{
    /** Information about a video bitstream AccessUnit */
    struct AccessUnit
    {
        std::list<std::vector<uint8_t>> mVpsNalUnits;
        std::list<std::vector<uint8_t>> mSpsNalUnits;
        std::list<std::vector<uint8_t>> mPpsNalUnits;
        std::list<std::vector<uint8_t>> mNalUnits;     // Non-parameter set NAL units
        std::vector<unsigned int> mRefPicIndices;
        unsigned int mPicIndex;      // Picture number in decoding order
        int mPoc;                    // Picture Order Count
        unsigned int mPicWidth;
        unsigned int mPicHeight;
        bool mIsIntra;
        bool mIsIdr;
        bool mIsCra;
        bool mIsBla;
        bool mPicOutputFlag;
    };

};

#endif
