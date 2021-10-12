
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
#pragma once

#include <vector>
#include <cstdint>
#include "parser/h265datastructs.hpp"

namespace VDD
{
    /* aLevel = 51 -> set level to 5.1 */
    void setSpsLevelIdc(H265::SequenceParameterSet& aSps, unsigned aLevel);

    std::vector<uint8_t> spsNalWithLevelIdc(const std::vector<uint8_t>& aSpsNal, unsigned aLevel);
}  // namespace VDD
