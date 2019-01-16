
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
#include "brandandcompatiblebrandsbasebox.hpp"
#include <algorithm>
#include "fullbox.hpp"
#include "log.hpp"

template <>
BrandAndCompatibleBrandsBaseBox<Box>::BrandAndCompatibleBrandsBaseBox(FourCCInt boxType)
    : Box(boxType)
    , mMajorBrand()
    , mMinorVersion(0)
    , mCompatibleBrands()
{
}

template <>
BrandAndCompatibleBrandsBaseBox<FullBox>::BrandAndCompatibleBrandsBaseBox(FourCCInt boxType,
                                                                          std::uint8_t version,
                                                                          std::uint32_t flags)
    : FullBox(boxType, version, flags)
    , mMajorBrand()
    , mMinorVersion(0)
    , mCompatibleBrands()
{
}

template <class T>
void BrandAndCompatibleBrandsBaseBox<T>::setMajorBrand(const String& majorBrand)
{
    mMajorBrand = majorBrand;
}

template <class T>
const String& BrandAndCompatibleBrandsBaseBox<T>::getMajorBrand() const
{
    return mMajorBrand;
}

template <class T>
void BrandAndCompatibleBrandsBaseBox<T>::setMinorVersion(uint32_t minorVersion)
{
    mMinorVersion = minorVersion;
}

template <class T>
uint32_t BrandAndCompatibleBrandsBaseBox<T>::getMinorVersion() const
{
    return mMinorVersion;
}

template <class T>
void BrandAndCompatibleBrandsBaseBox<T>::addCompatibleBrand(const String& compatibleBrand)
{
    mCompatibleBrands.push_back(compatibleBrand);
}

template <class T>
Vector<String> BrandAndCompatibleBrandsBaseBox<T>::getCompatibleBrands() const
{
    return mCompatibleBrands;
}

template <class T>
bool BrandAndCompatibleBrandsBaseBox<T>::checkCompatibleBrand(const String& brand) const
{
    if (find(mCompatibleBrands.begin(), mCompatibleBrands.end(), brand) != mCompatibleBrands.end())
    {
        return true;
    }
    else
    {
        return false;
    }
}

template <>
void BrandAndCompatibleBrandsBaseBox<Box>::writeHeader(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
}

template <>
void BrandAndCompatibleBrandsBaseBox<FullBox>::writeHeader(ISOBMFF::BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
}

template <class T>
void BrandAndCompatibleBrandsBaseBox<T>::writeBox(ISOBMFF::BitStream& bitstr)
{
    // Write the box header
    writeHeader(bitstr);  // write parent box

    // major_brand
    bitstr.writeString(mMajorBrand);
    // minor_version
    bitstr.write32Bits(mMinorVersion);
    // compatible_brands[]
    for (auto const& brand : mCompatibleBrands)
    {
        bitstr.writeString(brand);
    }

    // Update the size field of this box in the bitstream
    T::updateSize(bitstr);
}

template <>
void BrandAndCompatibleBrandsBaseBox<Box>::parseHeader(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);
}

template <>
void BrandAndCompatibleBrandsBaseBox<FullBox>::parseHeader(ISOBMFF::BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
}

template <class T>
void BrandAndCompatibleBrandsBaseBox<T>::parseBox(ISOBMFF::BitStream& bitstr)
{
    // Parse the box or fullbox header
    parseHeader(bitstr);

    // major_brand
    bitstr.readStringWithLen(mMajorBrand, 4);
    // minor_version
    mMinorVersion = bitstr.read32Bits();
    // compatible_brands[]
    while (bitstr.numBytesLeft() >= 4)
    {
        String compatibleBrand;
        bitstr.readStringWithLen(compatibleBrand, 4);
        mCompatibleBrands.push_back(compatibleBrand);
    }
}

template class BrandAndCompatibleBrandsBaseBox<Box>;
template class BrandAndCompatibleBrandsBaseBox<FullBox>;
