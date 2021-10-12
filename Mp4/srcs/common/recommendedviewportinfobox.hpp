
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
#ifndef RECOMMENDEDVIEWPORTINFOBOX_HPP
#define RECOMMENDEDVIEWPORTINFOBOX_HPP

#include <cstdint>
#include "api/isobmff/commontypes.h"
#include "customallocator.hpp"
#include "fullbox.hpp"

namespace ISOBMFF
{
    class BitStream;
}

/**
 * Protection Scheme Information Box class
 * @details 'rvif' box structure is specified in the OMAF specification.
 */
class RecommendedViewportInfoBox : public FullBox
{
public:
    RecommendedViewportInfoBox();

    void setFrom(const ISOBMFF::RecommendedViewportInfoStruct& rcvpInfo);
    ISOBMFF::RecommendedViewportInfoStruct get() const;

    std::uint8_t getViewportType() const;
    const String getDescription() const;

    void setViewportType(std::uint8_t type);
    void setDescription(const String& description);


    /** Write box data to ISOBMFF::BitStream. */
    void writeBox(ISOBMFF::BitStream& bitstream);

    /** Read box data from ISOBMFF::BitStream. */
    void parseBox(ISOBMFF::BitStream& bitstream);

private:
    ISOBMFF::RecommendedViewportInfoStruct mRecommendedViewportInfo;
};

#endif
