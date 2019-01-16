
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
#ifndef SAMPLEGROUPENTRY_HPP
#define SAMPLEGROUPENTRY_HPP

#include "bitstream.hpp"
#include "customallocator.hpp"

/** @brief SampleGroupEntry class.
 *  @details Provides abstract methods to generate a sample group entry.
 *  @details Other classes can inherit from SampleGroupEntry to implement different sample groupings **/
class SampleGroupEntry
{
public:
    SampleGroupEntry()          = default;
    virtual ~SampleGroupEntry() = default;

    /** @brief Get the size of the Sample Group Entry.
     *  @details Abstract class to be implemented by the extending class.
     *  @returns Serialized byte size of the sample group entry **/
    virtual std::uint32_t getSize() const = 0;

    /** @brief Serialize the sample group entry data structure.
     *  @details Abstract class to be implemented by the extending class.
     *  @param [out] bitstr Bitstream containing the serialized sample group entry data structure **/
    virtual void writeEntry(ISOBMFF::BitStream& bitstr) = 0;

    /** @brief Parse a serialized sample group entry data structure.
     *  @details Abstract class to be implemented by the extending class.
     *  @param [in] bitstr Bitstream containing the serialized sample group entry data structure **/
    virtual void parseEntry(ISOBMFF::BitStream& bitstr) = 0;
};

/** @brief DirectReferenceSampleListEntry class. Inherits from SampleGroupEntry.
 *  @details Implements DirectReferenceSampleListEntry as defined in ISOBMFF standard.**/
class DirectReferenceSampleListEntry : public SampleGroupEntry
{
public:
    DirectReferenceSampleListEntry();
    virtual ~DirectReferenceSampleListEntry() = default;

    /** @brief Set the sample identifier computed for a reference or a non reference sample.
     *  @param [in] sampleId Sample ID **/
    void setSampleId(std::uint32_t sampleId);

    /** @brief Get the sample identifier computed for a reference or a non reference sample.
     *  @returns Sample ID **/
    std::uint32_t getSampleId() const;

    /** @brief Set identifiers for referenced samples.
     *  @param [in] refSampleId Referenced sample Ids **/
    void setDirectReferenceSampleIds(const Vector<std::uint32_t>& refSampleId);

    /** @brief Get identifiers for referenced samples.
     *  @return Identifiers for referenced samples.**/
    Vector<std::uint32_t> getDirectReferenceSampleIds() const;

    /** @see SampleGroupEntry::getSize() */

    /** @brief Get the serialized byte size of DirectReferenceSampleListEntry.
     *  @return Byte size of the Entry**/
    virtual std::uint32_t getSize() const;

    /** @brief Serialize the DirectReferenceSampleListEntry data structure.
     *  @details Implemented by the extending class.
     *  @param [out] bitstr Bitstream containing the serialized DirectReferenceSampleListEntry data structure **/
    virtual void writeEntry(ISOBMFF::BitStream& bitstr);

    /** @brief Parse a serialized DirectReferenceSampleListEntry data structure.
     *  @details Implemented by the extending class.
     *  @param [in] bitstr Bitstream containing the serialized DirectReferenceSampleListEntry data structure **/
    virtual void parseEntry(ISOBMFF::BitStream& bitstr);

private:
    std::uint32_t mSampleId;                          ///< Sample Id whose referenced sample Id will be listed
    Vector<std::uint32_t> mDirectReferenceSampleIds;  ///< Vector of direct reference sample Ids
};

#endif /* end of include guard: SAMPLEGROUPENTRY_HPP */
