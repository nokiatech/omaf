
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
#ifndef DYNAMICVIEWPOINTSAMPLEENTRY_HPP
#define DYNAMICVIEWPOINTSAMPLEENTRY_HPP

#include "api/isobmff/commontypes.h"
#include "api/isobmff/optional.h"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "metadatasampleentrybox.hpp"
#include "overlayconfigbox.hpp"

/**
 * @brief OMAF DynamicViewpointSampleEntry class.
 *
 * Reference material: OMAFv2-WD6-Draft01-SM.pdf
 */

class DynamicViewpointSampleEntryBox : public MetaDataSampleEntryBox
{
public:
    DynamicViewpointSampleEntryBox();
    virtual ~DynamicViewpointSampleEntryBox();

    DynamicViewpointSampleEntryBox* clone() const override;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    void writeBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Parses bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    void parseBox(ISOBMFF::BitStream& bitstr) override;

    /** @brief Set the sample entry */
    void setSampleEntry(const ISOBMFF::DynamicViewpointSampleEntry& aSampleEntry);

    /** @brief Get the sample entry */
    const ISOBMFF::DynamicViewpointSampleEntry& getSampleEntry() const;

private:
    ISOBMFF::DynamicViewpointSampleEntry mSampleEntry;
};

#endif  // DYNAMICVIEWPOINTSAMPLEENTRY_HPP
