
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
#include "uriinitbox.hpp"

UriInitBox::UriInitBox()
    : FullBox("uriI", 0, 0)
    , mInitBoxType(UriInitBox::InitBoxType::UNKNOWN)
    , mUriInitializationData()
{
}

UriInitBox::InitBoxType UriInitBox::getInitBoxType() const
{
    return mInitBoxType;
}

void UriInitBox::setInitBoxType(UriInitBox::InitBoxType dataType)
{
    mInitBoxType = dataType;
}

void UriInitBox::setUriInitializationData(const Vector<std::uint8_t>& uriInitData)
{
    mInitBoxType           = InitBoxType::UNKNOWN;
    mUriInitializationData = uriInitData;
}

Vector<std::uint8_t> UriInitBox::getUriInitializationData() const
{
    return mUriInitializationData;
}

void UriInitBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.write8BitsArray(mUriInitializationData, static_cast<unsigned int>(mUriInitializationData.size()));

    updateSize(bitstr);
}

void UriInitBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);

    const uint64_t numBytesLeft = bitstr.numBytesLeft();
    if (numBytesLeft >= 8)
    {
        FourCCInt boxType;
        BitStream subBitstr = bitstr.readSubBoxBitStream(boxType);
        mUriInitializationData.clear();
        mUriInitializationData.reserve(numBytesLeft);
        subBitstr.read8BitsArray(mUriInitializationData, numBytesLeft);
    }
}
