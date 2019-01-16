
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
#ifndef METADATASAMPLEENTRYBOX_HPP
#define METADATASAMPLEENTRYBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "sampleentrybox.hpp"

/** @brief MetaDataSampleEntryBox class. Extends from SampleEntryBox.
 *  @details This box contains information related to the meta data samples of the track
 *  @details as defined in the ISOBMFF standard. **/

class MetaDataSampleEntryBox : public SampleEntryBox
{
public:
    MetaDataSampleEntryBox(FourCCInt codingname);
    virtual ~MetaDataSampleEntryBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a MetaDataSampleEntryBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

    /* @brief Returns the configuration record for this sample */
    virtual const DecoderConfigurationRecord* getConfigurationRecord() const override;

    /* @brief Returns the configuration box for this sample */
    virtual const Box* getConfigurationBox() const override;

private:
};

#endif  // METADATASAMPLEENTRYBOX_HPP
