
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
#ifndef DATAREFERENCEBOX_HPP
#define DATAREFERENCEBOX_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

#include <cstdint>

/** @brief Data Entry Box class. Extends from FullBox.
 *  @details This is the base class for DataEntryUrlBox and DataEntryUrnBox
 *  @details Can be present in DataInformationBox. There can be multiple data entries.
 */
class DataEntryBox : public FullBox
{
public:
    DataEntryBox(FourCCInt boxType, std::uint8_t version, std::uint32_t flags);
    virtual ~DataEntryBox() = default;

    /** @brief Set the location field as specificed in ISOBMFF specification.
     *  @param [in] location value of the location field as a string**/
    void setLocation(const String& location);

    /** @brief Get the location field as specificed in ISOBMFF.
     *  @return value of the location field as a string**/
    const String getLocation() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr) = 0;

    /** @brief Parses a Data Entry Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr) = 0;

private:
    String mLocation;  ///< location field as specificed in ISOBMFF specification.
};

/** @brief Data Entry URL Box class. Extends from DataEntryBox.
 */
class DataEntryUrlBox : public DataEntryBox
{
public:
    enum IsSelfContained
    {
        NotSelfContained,
        SelfContained
    };

    DataEntryUrlBox(IsSelfContained isSelfContained = NotSelfContained);
    virtual ~DataEntryUrlBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a Data Entry URL Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);
};

/** @brief Data Entry URN Box class. Extends from DataEntryBox.
 */
class DataEntryUrnBox : public DataEntryBox
{
public:
    DataEntryUrnBox();
    virtual ~DataEntryUrnBox() = default;

    /** @brief Sets the name field as specified in ISOBMFF specification
     *  @param [in]  name name field as a string */
    void setName(const String& name);

    /** @brief Returns the name field as specified in ISOBMFF specification
     *  @return  name field as a string */
    const String getName() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a Data Entry URN Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    String mName;  ///< name field as specificed in ISOBMFF specification
};

/** @brief Data Entry Imda Box class. Extends from DataEntryBox.
 */
class DataEntryImdaBox : public DataEntryBox
{
public:
    DataEntryImdaBox();
    virtual ~DataEntryImdaBox() = default;

    /** @brief Sets the name field as specified in ISOBMFF specification
     *  @param [in]  name name field as a string */
    void setImdaRefIdentifier(std::uint32_t imdaRefIdentifier);

    /** @brief Returns the imdaRefIdentifier field as specified in ISOBMFF specification
     *  @return  imdaRefIdentifier field as a string */
    std::uint32_t getImdaRefIdentifier() const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a Data Entry Imda Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    std::uint32_t mImdaRefIdentifier;  ///< imda_ref_identifier as specificed in ISOBMFF specification
};

/** @brief Data Entry Seq Num Imda Box class. Extends from DataEntryBox.
 */
class DataEntrySeqNumImdaBox : public DataEntryBox
{
public:
    DataEntrySeqNumImdaBox();
    virtual ~DataEntrySeqNumImdaBox() = default;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a Data Entry Seq Num Imda Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
};

/** @brief Data Reference Box class. Extends from FullBox.
 */
class DataReferenceBox : public FullBox
{
public:
    DataReferenceBox();
    virtual ~DataReferenceBox() = default;

    /// @return 1-based entry index
    unsigned int addEntry(std::shared_ptr<DataEntryBox> dataEntryBox);

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a Data Reference Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data
     *  @throws Runtime Error if there is an unknown box inside Data Reference Box */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

    /** @brief Access the DataEntryBoxes */
    Vector<std::shared_ptr<const DataEntryBox>> getDataEntries() const;

private:
    Vector<std::shared_ptr<DataEntryBox>> mDataEntries;  ///< data reference entries present in the box
};

#endif /* end of include guard: DATAREFERENCEBOX_HPP */
