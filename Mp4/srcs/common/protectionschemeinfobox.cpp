
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
#include "protectionschemeinfobox.hpp"
#include "bitstream.hpp"

Vector<std::uint8_t> ProtectionSchemeInfoBox::getData() const
{
    return mData;
}

void ProtectionSchemeInfoBox::setData(const Vector<std::uint8_t>& data)
{
    mData = data;
}

void ProtectionSchemeInfoBox::writeBox(BitStream& bitstream)
{
    bitstream.write8BitsArray(mData, mData.size());
}

void ProtectionSchemeInfoBox::parseBox(BitStream& bitstream)
{
    bitstream.read8BitsArray(mData, bitstream.getSize());
}
