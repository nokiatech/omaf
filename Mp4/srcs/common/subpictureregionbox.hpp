
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
#ifndef SUBPICTUREREGIONBOX_HPP
#define SUBPICTUREREGIONBOX_HPP

#include "fullbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "api/isobmff/commontypes.h"

#include <cstdint>

/** @brief SubPictureRegionBox Box class. Extends from FullBox.
 */
class SubPictureRegionBox : public FullBox
{
public:
    SubPictureRegionBox();
    SubPictureRegionBox(const ISOBMFF::SubPictureRegionData& aData);
    ~SubPictureRegionBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses a Full box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Access the parsed data */
    const ISOBMFF::SubPictureRegionData& get() const;

    /** @brief Access the parsed data */
    ISOBMFF::SubPictureRegionData& get();

private:
    ISOBMFF::SubPictureRegionData mData;
};

#endif /* end of include guard: SUBPICTUREREGIONBOX_HPP */
