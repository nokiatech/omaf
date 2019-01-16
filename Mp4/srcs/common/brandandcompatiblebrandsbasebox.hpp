
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
#ifndef BRANDSANDCOMPATIBLEBRANDSBASEBOX_HPP
#define BRANDSANDCOMPATIBLEBRANDSBASEBOX_HPP

#include "bbox.hpp"
#include "bitstream.hpp"
#include "customallocator.hpp"
#include "fullbox.hpp"

#include <string>

/** @brief Implementation + API for ftyp, styp and ttyp boxes (and other if there is more to come with same box payload)
 *  @details box implementation as specified in the ISOBMFF specification
 *  @details Major and Compatible Brands List are present in this box.
 */
template <class T>
class BrandAndCompatibleBrandsBaseBox : public T
{
public:
    BrandAndCompatibleBrandsBaseBox(FourCCInt boxType);
    BrandAndCompatibleBrandsBaseBox(FourCCInt boxType, std::uint8_t version, std::uint32_t flags = 0);

    virtual ~BrandAndCompatibleBrandsBaseBox() = default;

    /** @brief Set the Major Brand
     *  @param [in] majorBrand Major Brand as string value and as defined in relevant file format standards **/
    void setMajorBrand(const String& majorBrand);

    /** @brief Get the Major Brand
     *  @returns Major Brand as string value **/
    const String& getMajorBrand() const;

    /** @brief Set the minor version
     *  @param [in] minorVersion Minor Version as an unsigned integer value. **/
    void setMinorVersion(std::uint32_t minorVersion);

    /** @brief Get the minor version
     *  @returns Minor Version as an unsigned integer value. **/
    uint32_t getMinorVersion() const;

    /** @brief Adds a compatible brand to the list of compatible brands
     *  @param [in] compatibleBrand Compatible Brand as string value and as defined in relevant file format standards
     * **/
    void addCompatibleBrand(const String& compatibleBrand);

    /** @brief Get the Compatible Brands List
     *  @returns Compatible Brand as vector of string values **/
    Vector<String> getCompatibleBrands() const;

    /** @brief Check if a brand is among the list of compatible brands
     *  @param [in] brand Brand value to be checked
     *  @returns TRUE: if the brand is in the compatible brands list. FALSE otherwise. **/
    bool checkCompatibleBrand(const String& brand) const;

    /** @brief Creates the bitstream that represents the box in the ISOBMFF file
     *  @param [out] bitstr Bitstream that contains the box data. */
    virtual void writeBox(ISOBMFF::BitStream& bitstr);

    /** @brief Parses a File Type Box bitstream and fills in the necessary member variables
     *  @param [in]  bitstr Bitstream that contains the box data */
    virtual void parseBox(ISOBMFF::BitStream& bitstr);

private:
    // helpers for having separate implementations how to parse/write header for Box / FullBox
    virtual void writeHeader(ISOBMFF::BitStream& bitstr);
    virtual void parseHeader(ISOBMFF::BitStream& bitstr);

    String mMajorBrand;                ///< Major Brand as string value
    std::uint32_t mMinorVersion;       ///< Minor version as an unsigned integer
    Vector<String> mCompatibleBrands;  ///< Vector containing the Compatible Brands as strings
};

#endif /* end of include guard: BRANDSANDCOMPATIBLEBRANDSBASEBOX_HPP */
