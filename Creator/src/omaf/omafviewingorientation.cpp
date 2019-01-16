
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
#include "omaf/omafviewingorientation.h"
#include "omaf/parser/bitstream.hpp"


namespace VDD
{
    std::vector<uint8_t> InitialViewingOrientationSample::toBitstream()
    {
        Parser::BitStream sampleStream;
        sampleStream.write32Bits(cAzimuth * 65536);
        sampleStream.write32Bits(cElevation * 65536);
        sampleStream.write32Bits(cTilt * 65536);
        sampleStream.write8Bits(0);  // interpolate is always 0
        sampleStream.write8Bits(refreshFlag ? 0x1 : 0x0);

        return sampleStream.getStorage();
    }

}  // namespace VDD
