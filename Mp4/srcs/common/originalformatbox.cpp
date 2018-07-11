
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
#include "originalformatbox.hpp"
#include "bitstream.hpp"

OriginalFormatBox::OriginalFormatBox()
    : Box("frma")
{
}

FourCCInt OriginalFormatBox::getOriginalFormat() const
{
    return mOriginalFormat;
}

void OriginalFormatBox::setOriginalFormat(FourCCInt origFormat)
{
    mOriginalFormat = origFormat;
}

void OriginalFormatBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    writeBoxHeader(bitstr);
    bitstr.write32Bits(mOriginalFormat.getUInt32());
    updateSize(bitstr);
}

void OriginalFormatBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    parseBoxHeader(bitstr);
    mOriginalFormat = bitstr.read32Bits();
}
