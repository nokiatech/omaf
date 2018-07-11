
/** 
 * This file is part of Nokia OMAF implementation
 *
 * Copyright (c) 2018 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
 *
 * Contact: omaf@nokia.com
 *
 * This software, including documentation, is protected by copyright controlled by Nokia Corporation and/ or its
 * subsidiaries. All rights are reserved.
 *
 * Copying, including reproducing, storing, adapting or translating, any or all of this material requires the prior
 * written consent of Nokia.
 */
#include "schemetypebox.hpp"
#include "bitstream.hpp"

using std::endl;

SchemeTypeBox::SchemeTypeBox()
    : FullBox("schm", 0, 0)
    , mSchemeType(0)
    , mSchemeVersion(0)
    , mSchemeUri("")
{
}

void SchemeTypeBox::setSchemeType(FourCCInt schemeType)
{
    mSchemeType = schemeType;
}

FourCCInt SchemeTypeBox::getSchemeType() const
{
    return mSchemeType;
}

void SchemeTypeBox::setSchemeVersion(std::uint32_t schemeVersion)
{
    mSchemeVersion = schemeVersion;
}

std::uint32_t SchemeTypeBox::getSchemeVersion() const
{
    return mSchemeVersion;
}

void SchemeTypeBox::setSchemeUri(const String& uri)
{
    std::uint32_t hasUriFieldFlags = uri.empty() ? getFlags() & (~0x1) : getFlags() | 0x1;
    setFlags(hasUriFieldFlags);
    mSchemeUri = uri;
}

const String& SchemeTypeBox::getSchemeUri() const
{
    return mSchemeUri;
}

void SchemeTypeBox::writeBox(BitStream& bitstr)
{
    writeFullBoxHeader(bitstr);
    bitstr.write32Bits(mSchemeType.getUInt32());
    bitstr.write32Bits(mSchemeVersion);
    if (getFlags() & 0x1)
    {
        bitstr.writeZeroTerminatedString(mSchemeUri);
    }
    updateSize(bitstr);
}

void SchemeTypeBox::parseBox(BitStream& bitstr)
{
    parseFullBoxHeader(bitstr);
    mSchemeType    = bitstr.read32Bits();
    mSchemeVersion = bitstr.read32Bits();
    if (getFlags() & 0x1)
    {
        bitstr.readZeroTerminatedString(mSchemeUri);
    }
}
