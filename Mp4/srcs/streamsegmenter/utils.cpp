
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "bitstream.hpp"

#include "utils.hpp"

namespace StreamSegmenter
{
    namespace Utils
    {
        std::string stringOfAvcNalUnitType(AvcNalUnitType naluType)
        {
            std::string type;
            const char* types[] = {"UNSPECIFIED_0",
                                   "CODED_SLICE_NON_IDR",
                                   "CODED_SLICE_DPAR_A",
                                   "CODED_SLICE_DPAR_B",
                                   "CODED_SLICE_DPAR_C",
                                   "CODED_SLICE_IDR",
                                   "SEI",
                                   "SPS",
                                   "PPS",
                                   "ACCESS_UNIT_DELIMITER",
                                   "EOS",
                                   "EOB",
                                   "FILLER_DATA",
                                   "SPS_EXT",
                                   "PREFIX_NALU",
                                   "SUB_SPS",
                                   "DPS",
                                   "RESERVED_17",
                                   "RESERVED_18",
                                   "SLICE_AUX_NOPAR",
                                   "SLICE_EXT",
                                   "SLICE_EXT_3D",
                                   "RESERVED_22",
                                   "RESERVED_23",
                                   "UNSPECIFIED_24",
                                   "UNSPECIFIED_25",
                                   "UNSPECIFIED_26",
                                   "UNSPECIFIED_27",
                                   "UNSPECIFIED_28",
                                   "UNSPECIFIED_29",
                                   "UNSPECIFIED_30",
                                   "UNSPECIFIED_31"};
            if (size_t(naluType) < sizeof(types) / sizeof(*types))
            {
                type = types[size_t(naluType)];
            }
            else
            {
                type = "INVALID";
            }
            return type;
        }

        void dumpBox(std::ostream& stream, ::Box& box)
        {
            BitStream bs;
            box.writeBox(bs);
            auto& storage = bs.getStorage();
            hexDump(stream, storage.begin(), storage.end());
        }


    }  // namepsace Utils
}  // namespace StreamSegmenter
