
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
#ifndef URIMETASAMPLEENTRY_HPP_
#define URIMETASAMPLEENTRY_HPP_

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "metadatasampleentrybox.hpp"
#include "uribox.hpp"
#include "uriinitbox.hpp"

/** @brief UriMetaSampleEntryBox class. Extends from MetaDataSampleEntryBox.
 *  @details This box contains information related to the URI meta sample of the track
 *  @details as defined in the ISOBMFF standard. **/

class UriMetaSampleEntryBox : public MetaDataSampleEntryBox
{
public:
    UriMetaSampleEntryBox();
    virtual ~UriMetaSampleEntryBox() = default;

    enum class VRTMDType
    {
        UNKNOWN = 0
    };

    /** @brief Gets the reference of the URI Box.
     *  @return Reference to the  URI Box. **/
    UriBox& getUriBox();

    /** @brief Gets whether box has URI Init Box.
     *  @return bool true if box exists **/
    bool hasUriInitBox();

    /** @brief Gets the reference of the URI Init Box.
     *  @return Reference to the  URI Init Box. **/
    UriInitBox& getUriInitBox();

    /** @brief Gets the type of timed meta data.
     *  @return UriMetaSampleEntryBox::TMDType enumeration**/
    VRTMDType getVRTMDType() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a MetaDataSampleEntryBox bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

    /* @brief Make a copy of this box that has dynamically the same type as this */
    virtual UriMetaSampleEntryBox* clone() const;

    /* @brief Returns the configuration record for this sample */
    virtual const DecoderConfigurationRecord* getConfigurationRecord() const override;

    /* @brief Returns the configuration box for this sample */
    virtual const Box* getConfigurationBox() const override;

private:
    UriBox mUriBox;
    VRTMDType mVRMetaDataType;
    bool mHasUriInitBox;
    UriInitBox mUriInitBox;
};

#endif /* URIMETASAMPLEENTRY_HPP_ */
