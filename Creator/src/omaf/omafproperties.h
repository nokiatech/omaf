
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
#pragma once

#include <memory>
#include <vector>

#include "parser/h265parser.hpp"
#include "processor/meta.h"


namespace VDD {
    namespace OMAF {
        std::uint32_t createProjectionSEI(Parser::BitStream& bitstr, int temporalId);
        //std::uint32_t createRwpkSEI(Parser::BitStream& bitstr, const TilePixelRegion& aTile, int temporalId);
        std::uint32_t createRwpkSEI(Parser::BitStream& bitstr, const RegionPacking& aRwpk, int temporalId);
    }

}
