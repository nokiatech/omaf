
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
#include "visualsampleentrybox.hpp"
#include "cleanaperturebox.hpp"
#include "log.hpp"

#include <cassert>
#include <cstring>
#include <stdexcept>

/// Length of compressorname string in VisualSampleEntry class
static const unsigned int COMPRESSORNAME_STRING_LENGTH = 31;

VisualSampleEntryBox::VisualSampleEntryBox(FourCCInt codingName, const String& compressorName)
    : SampleEntryBox(codingName)
    , mWidth(0)
    , mHeight(0)
    , mCompressorName(compressorName)
    , mClap(nullptr)
{
    assert(mCompressorName.length() <= COMPRESSORNAME_STRING_LENGTH);
    mCompressorName.resize(COMPRESSORNAME_STRING_LENGTH, '\0');  // Make fixed length
}

VisualSampleEntryBox::VisualSampleEntryBox(const VisualSampleEntryBox& box)
    : SampleEntryBox(box)
    , mWidth(box.mWidth)
    , mHeight(box.mHeight)
    , mCompressorName(box.mCompressorName)
    , mClap(box.mClap)
{
    assert(mCompressorName.length() <= COMPRESSORNAME_STRING_LENGTH);
    mCompressorName.resize(COMPRESSORNAME_STRING_LENGTH, '\0');  // Make fixed length
}

void VisualSampleEntryBox::setWidth(uint16_t width)
{
    mWidth = width;
}

uint32_t VisualSampleEntryBox::getWidth() const
{
    return mWidth;
}

void VisualSampleEntryBox::setHeight(uint16_t height)
{
    mHeight = height;
}
uint32_t VisualSampleEntryBox::getHeight() const
{
    return mHeight;
}

void VisualSampleEntryBox::createClap()
{
    if (mClap.get() == nullptr)
    {
        mClap = makeCustomShared<CleanApertureBox>();
    }
}

const CleanApertureBox* VisualSampleEntryBox::getClap() const
{
    return mClap.get();
}

CleanApertureBox* VisualSampleEntryBox::getClap()
{
    return mClap.get();
}

void VisualSampleEntryBox::writeBox(ISOBMFF::BitStream& bitstr)
{
    SampleEntryBox::writeBox(bitstr);

    bitstr.write16Bits(0);           // pre_defined = 0
    bitstr.write16Bits(0);           // reserved = 0
    bitstr.write32Bits(0);           // reserved = 0
    bitstr.write32Bits(0);           // reserved = 0
    bitstr.write32Bits(0);           // reserved = 0
    bitstr.write16Bits(mWidth);      // width
    bitstr.write16Bits(mHeight);     // height
    bitstr.write32Bits(0x00480000);  // horizresolution 72 dpi
    bitstr.write32Bits(0x00480000);  // vertresolution 72 dpi
    bitstr.write32Bits(0);           // reserved = 0
    bitstr.write16Bits(1);           // frame_count = 1

    assert(mCompressorName.length() <= COMPRESSORNAME_STRING_LENGTH);

    // Length-byte prefixed compressor name padded to 32 bytes total.
    bitstr.write8Bits(static_cast<std::uint8_t>(mCompressorName.length()));
    mCompressorName.resize(COMPRESSORNAME_STRING_LENGTH, '\0');  // Make fixed length
    bitstr.writeString(mCompressorName);                         // Write entire string buffer, including padding zeros

    bitstr.write16Bits(0x0018);                              // depth
    bitstr.write16Bits(static_cast<uint16_t>(int16_t(-1)));  // pre_defined

    auto rinfBox = getRestrictedSchemeInfoBox();
    if (rinfBox != nullptr)
    {
        bitstr.setByte(4, 'r');
        bitstr.setByte(5, 'e');
        bitstr.setByte(6, 's');
        bitstr.setByte(7, 'v');

        rinfBox->writeBox(bitstr);
    }

    // Update the size of the movie box
    updateSize(bitstr);
}

void VisualSampleEntryBox::parseBox(ISOBMFF::BitStream& bitstr)
{
    SampleEntryBox::parseBox(bitstr);

    bitstr.read16Bits();            // pre_defined
    bitstr.read16Bits();            // reserved
    bitstr.read32Bits();            // predefined
    bitstr.read32Bits();            // predefined
    bitstr.read32Bits();            // predefined
    mWidth  = bitstr.read16Bits();  // width
    mHeight = bitstr.read16Bits();  // height
    bitstr.read32Bits();            // horizontal resolution
    bitstr.read32Bits();            // vertical resolution
    bitstr.read32Bits();            // reserved
    bitstr.read16Bits();            // frame_count

    const uint8_t compressorNameLength = bitstr.read8Bits();
    if (compressorNameLength > COMPRESSORNAME_STRING_LENGTH)
    {
        throw RuntimeError("Too long compressorname string length read from VisualSampleEntry (>31 bytes).");
    }
    String codecName;
    bitstr.readStringWithLen(codecName, compressorNameLength);  // compressor name
    for (unsigned int i = compressorNameLength; i < COMPRESSORNAME_STRING_LENGTH; ++i)
    {
        bitstr.read8Bits();  // discard padding
    }

    bitstr.read16Bits();  // depth
    bitstr.read16Bits();  // pre_defined

    // Read the optional clap box, if present
    if (bitstr.numBytesLeft() > 8)
    {
        const uint64_t startOffset = bitstr.getPos();
        FourCCInt boxType;
        BitStream subBitStream = bitstr.readSubBoxBitStream(boxType);
        if (boxType == "clap")
        {
            const auto clap = makeCustomShared<CleanApertureBox>();
            clap->parseBox(subBitStream);
            mClap = clap;
        }
        else
        {
            // It was not 'clap', so the contained box probably belongs to the box box extending VisualSampleEntryBox.
            // Reset bitstream position so it will be possible to read the whole extending box.
            bitstr.setPosition(startOffset);
        }
    }
}

bool VisualSampleEntryBox::isVisual() const
{
    return true;
}
