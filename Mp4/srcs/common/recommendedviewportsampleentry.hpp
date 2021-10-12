
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
#ifndef RECOMMENDEDVIEWPORTSAMPLEENTRYBOX_HPP
#define RECOMMENDEDVIEWPORTSAMPLEENTRYBOX_HPP

#include "api/isobmff/commontypes.h"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "recommendedviewportinfobox.hpp"
#include "sphereregionsampleentrybox.hpp"


/**
 * OMAF recommended viewport sample description entry.
 *
 * sample type for this is SphereRegionSample
 */
class RecommendedViewportSampleEntryBox : public SphereRegionSampleEntryBox
{
public:
    RecommendedViewportSampleEntryBox();
    virtual ~RecommendedViewportSampleEntryBox() = default;

    virtual RecommendedViewportSampleEntryBox* clone() const override;

    RecommendedViewportInfoBox& getRecommendedViewportInfo();

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr) override;

private:
    // mandatory box
    RecommendedViewportInfoBox mRecommendedViewportInfoBox;
};

#endif
