
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
#ifndef SPATIALRELATIONSHIP2DDESCRIPTIONBOX_HPP
#define SPATIALRELATIONSHIP2DDESCRIPTIONBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "trackgrouptypebox.hpp"
#include "api/isobmff/commontypes.h"

#include <cstdint>

/** @brief SpatialRelationship2DDescriptionBox Box class. Extends from FullBox.
 */
class SpatialRelationship2DDescriptionBox : public TrackGroupTypeBox
{
public:
    SpatialRelationship2DDescriptionBox();
    ~SpatialRelationship2DDescriptionBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses a Full box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr) override;

    SpatialRelationship2DDescriptionBox* clone() const override;

    /** @brief Access the parsed data */
    const ISOBMFF::SpatialRelationship2DDescriptionData& get() const;

    /** @brief Access the parsed data */
    ISOBMFF::SpatialRelationship2DDescriptionData& get();

private:
    ISOBMFF::SpatialRelationship2DDescriptionData mData;
};

#endif /* end of include guard: SPATIALRELATIONSHIP2DDESCRIPTIONBOX_HPP */
